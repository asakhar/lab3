#include <sys/types.h>

#include <cstddef>
#include <cstdint>

#include "smartp.hpp"

enum class IOMode : int { READ = 1, WRITE = 2, READWRITE = 3 };
enum class IOPos : int { SET = 0, CUR = 1, END = 2 };
#ifdef __cpp_concepts

template <character_type T>
#else
template <typename T>
#endif
class basic_stream_traits {
public:
  virtual ~basic_stream_traits() = default;

protected:
  static constexpr size_t BUFFER_SIZE = 80;
  struct {
    array<T> buf{BUFFER_SIZE};
    size_t pos = 0;
    size_t size = 0;
  } m_rbuffer;
  struct {
    array<T> buf{BUFFER_SIZE};
    size_t size = 0;
  } m_wbuffer;
  ssize_t m_roffset = 0;
  ssize_t m_woffset = 0;
  array<T> m_fn;
};
#define fold(x) (__builtin_constant_p(x) ? (x) : (x))

#ifdef __cpp_concepts
template <character_type T, typename HANDLE_T>
#else
template <typename T, typename HANDLE_T>
#endif
class basic_fstream_traits : public basic_stream_traits<T> {
public:
  void openstream() {
    for (auto it = m_open_actions.begin(); it != m_open_actions.end(); ++it) {
      invoke(*it, this);
    }
  };
  IOMode getOpenMode() const { return m_mode; }
  HANDLE_T getHandle() const { return m_handle; }
  void setSeekable(bool val) { m_isSeekable = val; }
  void setHandle(HANDLE_T handle) {
    m_handle = handle;
    this->m_roffset = 0;
    this->m_woffset = 0;
    this->m_wbuffer.size = 0;
    this->m_rbuffer.pos = 0;
    this->m_rbuffer.size = 0;
  }
  T const *getFileName() const { return m_fn.data(); }
  void setFileName(T const *filename) {
    m_fn = array<T>(filename, stringlen(filename) + 1);
  }
  virtual ~basic_fstream_traits() = default;

protected:
  inline static HANDLE_T INVALID_HANDLE = reinterpret_cast<HANDLE_T>(-1);
  vector<void (*)(basic_fstream_traits *)> m_open_actions;
  bool m_isSeekable = true;

  array<T> m_fn;
  IOMode m_mode = IOMode::READ;
  HANDLE_T m_handle = INVALID_HANDLE;
};

extern void (*file_error_handler)(char const *FILE, char const *FUNCTION);
void default_file_error_handler(char const *FILE, char const *FUNCTION);

#ifdef __linux__
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

template <character_type T>
class basic_fstream_unix : public basic_fstream_traits<T, int> {
public:
  basic_fstream_unix() { this->m_open_actions.append(&open_unix); }

  ~basic_fstream_unix() {
    if (close(this->getHandle()) == -1) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
    }
    this->setHandle(basic_fstream_traits<T, int>::INVALID_HANDLE);
    this->setFileName("(no file)");
  }

protected:
  static void open_unix(basic_fstream_traits<T, int> *self) {
    int flags = O_RDONLY;
    if (self->getOpenMode() == IOMode::WRITE) {
      flags = O_WRONLY | O_CREAT;
    }
    if (static_cast<IOMode>(static_cast<int>(self->getOpenMode()) &
                            static_cast<int>(IOMode::READWRITE)) ==
        IOMode::READWRITE) {
      flags = O_RDWR | O_CREAT;
    }
    constexpr int mode = 0666;
    self->setHandle(open(self->getFileName(), flags, mode));
    if (self->getHandle() == basic_fstream_traits<T, int>::INVALID_HANDLE) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      self->setFileName("(no file)");
    }
  }
};

template <character_type T>
class basic_ifstream : public basic_fstream_unix<T> {
public:
  basic_ifstream(array<T> const &filename) {
    this->m_fn = filename;
    this->m_mode = IOMode::READ;
    this->openstream();
  }
  basic_ifstream(int handle, bool isSeekable = true) {
    this->setSeekable(isSeekable);
    this->setFileName("(opened by handle)");
    this->m_mode = IOMode::READ;
    this->setHandle(handle);
  }

  virtual ssize_t rseek(ssize_t offset, IOPos position) {
    if (!this->m_isSeekable)
      return -1l;
    if (lseek(this->m_handle,
              this->m_roffset + static_cast<ssize_t>(this->m_rbuffer.pos) -
                  static_cast<ssize_t>(this->m_rbuffer.size),
              SEEK_SET) == -1) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return -1l;
    }
    auto cur = lseek(this->m_handle, offset, static_cast<int>(position));
    if (cur == -1) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return -1l;
    }
    this->m_rbuffer.size = 0;
    this->m_rbuffer.pos = 0;
    return this->m_roffset = cur;
  }
  virtual ssize_t tellr() const {
    return this->m_roffset + static_cast<ssize_t>(this->m_rbuffer.pos) -
           static_cast<ssize_t>(this->m_rbuffer.size);
  }
  ssize_t tellend() {
    if (this->m_isSeekable)
      return ::lseek(this->m_handle, 0, SEEK_END);
    return 0l;
  }
  bool eof() {
    if (this->m_isSeekable)
      return tellend() == tellr();
    int n;

    auto res = ioctl(0, FIONREAD, &n);
    if (res == -1) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return true;
    }
    if (n == 0)
      return true;
    return false;
  }
  static bool is_nl(T ch) { return ch == '\n'; }
  virtual pair<array<T>, size_t> readline() { return readUntil(&is_nl); }
  virtual pair<array<T>, size_t> readUntil(bool (*predicate)(T)) {
    array<T> result{1};
    size_t totalRead = 0;
    bool firstReq = true;
    for (;;) {
      auto const prevsize = result.capacity() >> 1;
      auto [readsize, res] = readUntil(
          result, prevsize, result.capacity() - prevsize, predicate, firstReq);
      firstReq = false;
      totalRead += readsize;
      if (res)
        break;
      if (readsize + prevsize != result.capacity())
        break;
      array<T> newarray{result.capacity() << 1};
      for (size_t i = 0; i < result.capacity(); ++i)
        newarray[i] = result[i];
      result = forward<array<T>>(newarray);
    }
    return {result, totalRead};
  }
  virtual pair<size_t, bool> readUntil(array<T> &buffer, bool (*predicate)(T),
                                       bool firstReq = true) {
    return readUntil(buffer, 0, buffer.capacity(), predicate, firstReq);
  }
  virtual pair<size_t, bool> readUntil(array<T> &buffer, size_t start,
                                       size_t size, bool (*predicate)(T),
                                       bool firstReq = true) {
    size_t actualRead = 0;
    bool found = false;
    while (actualRead != size) {
      if (checkNeedsFill()) {
        auto filled = fillBuffer(firstReq);
        firstReq = false;
        if (filled == 0)
          break;
      }
      auto result = consumeUntil(buffer, actualRead + start, size - actualRead,
                                 predicate);
      actualRead += result.first;
      if (result.second) {
        found = true;
        break;
      }
    }
    return {actualRead, found};
  }
  virtual size_t read(array<T> &buffer, size_t size, bool firstReq = true) {
    size_t actualRead = 0;
    while (actualRead != size) {
      if (checkNeedsFill()) {
        auto filled = fillBuffer(firstReq);
        firstReq = false;
        if (filled == 0)
          break;
      }
      actualRead += consumeBuffer(buffer, actualRead, size - actualRead);
    }
    return actualRead;
  }

protected:
  bool checkNeedsFill() const { return this->m_rbuffer.size == 0; }
  virtual size_t fillBuffer(bool firstRequest = true) {
    if (this->m_isSeekable &&
        lseek(this->m_handle, this->m_roffset, SEEK_SET) == -1) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return 0ul;
    }
    if (!firstRequest && !this->m_isSeekable && eof())
      return 0l;
    errno = 0;
    auto rsize = ::read(
        this->m_handle, this->m_rbuffer.buf.data() + this->m_rbuffer.size,
        (this->m_rbuffer.buf.capacity() - this->m_rbuffer.size) * sizeof(T));
    if (rsize == -1l) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return 0ul;
    }
    if (errno != 0) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return 0ul;
    }
    auto actualSize = rsize / sizeof(T);
    this->m_rbuffer.size += actualSize;
    this->m_roffset += rsize;
    return static_cast<size_t>(actualSize);
  }
  virtual size_t consumeBuffer(array<T> &buffer, size_t start, size_t size) {
    auto consumed = min(this->m_rbuffer.size - this->m_rbuffer.pos, size);
    memcpy(buffer.data() + start,
           this->m_rbuffer.buf.data() + this->m_rbuffer.pos,
           consumed * sizeof(T));
    this->m_rbuffer.pos += consumed;
    if (this->m_rbuffer.pos == this->m_rbuffer.size) {
      this->m_rbuffer.pos = 0;
      this->m_rbuffer.size = 0;
    }
    return consumed;
  }
  virtual pair<size_t, bool> consumeUntil(array<T> &buffer, size_t start,
                                          size_t size, bool (*predicate)(T)) {
    size_t consumed = this->m_rbuffer.pos;
    auto const predicted =
        min(this->m_rbuffer.size, size + this->m_rbuffer.pos);
    bool result = false;
    for (; consumed < predicted; ++consumed) {
      if (invoke(predicate, this->m_rbuffer.buf[consumed])) {
        result = true;
        ++consumed;
        break;
      }
    }
    consumed -= this->m_rbuffer.pos;
    memcpy(buffer.data() + start,
           this->m_rbuffer.buf.data() + this->m_rbuffer.pos,
           consumed * sizeof(T));
    this->m_rbuffer.pos += consumed;
    if (this->m_rbuffer.pos == this->m_rbuffer.size) {
      this->m_rbuffer.pos = 0;
      this->m_rbuffer.size = 0;
    }
    return {consumed, result};
  }
};

template <character_type T>
class basic_ofstream : public basic_fstream_unix<T> {
public:
  basic_ofstream(array<T> const &filename) {
    this->m_fn = filename;
    this->m_mode = IOMode::WRITE;
    this->openstream();
  }
  basic_ofstream(int handle, bool isSeekable = true) {
    this->setSeekable(isSeekable);
    this->setFileName("(opened by handle)");
    this->m_mode = IOMode::WRITE;
    this->setHandle(handle);
  }

  virtual ssize_t wseek(ssize_t offset, IOPos position) {
    if (!this->m_isSeekable)
      return -1l;
    flush();
    if (lseek(this->m_handle, this->m_woffset, SEEK_SET) == -1) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return -1l;
    }
    auto cur = lseek(this->m_handle, offset, static_cast<int>(position));
    if (cur == -1) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return -1l;
    }
    return this->m_woffset = cur;
  }
  virtual ssize_t tellw() const {
    return this->m_woffset + static_cast<ssize_t>(this->m_wbuffer.size);
  }
  virtual size_t write(T const *buffer) {
    array<T> arr{buffer, stringlen(buffer)};
    return write(arr);
  }
  virtual size_t write(array<T> const &buffer) {
    return write(buffer, buffer.capacity());
  }
  virtual size_t write(array<T> const &buffer, size_t size) {
    size_t actualWritten = 0;
    while (actualWritten != size) {
      auto filled = fillBuffer(buffer, actualWritten, size - actualWritten);
      actualWritten += filled;
      if (this->m_wbuffer.size == this->m_wbuffer.buf.capacity()) {
        if (flush() == 0)
          return actualWritten - filled;
      }
    }
    return actualWritten;
  }
  virtual size_t flush() {
    if (this->m_wbuffer.size == 0)
      return 0ul;
    if (this->m_isSeekable &&
        lseek(this->m_handle, this->m_woffset, SEEK_SET) == -1) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return 0ul;
    }
    auto wsize = ::write(this->m_handle, this->m_wbuffer.buf.data(),
                         this->m_wbuffer.size * sizeof(T));
    if (wsize != static_cast<ssize_t>(this->m_wbuffer.size * sizeof(T))) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return 0ul;
    }
    auto actualSize = wsize / sizeof(T);
    this->m_wbuffer.size = 0;
    this->m_woffset += wsize;
    return static_cast<size_t>(actualSize);
  };

  ~basic_ofstream() { flush(); }

protected:
  virtual size_t fillBuffer(array<T> const &buffer, size_t start, size_t size) {
    size_t toFill =
        min(size, this->m_wbuffer.buf.capacity() - this->m_wbuffer.size);
    memcpy(this->m_wbuffer.buf.data() + this->m_wbuffer.size,
           buffer.data() + start, toFill * sizeof(T));
    this->m_wbuffer.size += toFill;
    return toFill;
  }
};

#elif _WIN32

#include <windows.h>
#ifdef __cpp_concepts
template <character_type T>
#else
template <typename T>
#endif
class basic_fstream_windows : public basic_fstream_traits<T, HANDLE> {
public:
  basic_fstream_windows() { this->m_open_actions.append(&open_windows); }

  ~basic_fstream_windows() {
    if (CloseHandle(this->getHandle()) == -1) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
    }
    this->setHandle(basic_fstream_traits<T, HANDLE>::INVALID_HANDLE);
    this->setFileName("(no file)");
  }

protected:
  static void open_windows(basic_fstream_traits<T, HANDLE> *self) {
    DWORD creationDisposition = OPEN_EXISTING;
    DWORD desiredAccess = 0;
    if (self->getOpenMode() == IOMode::WRITE) {
      desiredAccess |= GENERIC_WRITE;
      creationDisposition = OPEN_ALWAYS;
    }
    if (self->getOpenMode() == IOMode::READ) {
      desiredAccess |= GENERIC_READ;
    }
    DWORD const shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    self->setHandle(CreateFile(self->getFileName(), desiredAccess, shareMode,
                               nullptr, creationDisposition,
                               FILE_ATTRIBUTE_NORMAL, NULL));
    if (self->getHandle() == basic_fstream_traits<T, HANDLE>::INVALID_HANDLE) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      self->setFileName("(no file)");
    }
  }
};
#ifdef __cpp_concepts
template <character_type T>
#else
template <typename T>
#endif
class basic_ifstream : public basic_fstream_windows<T> {
public:
  basic_ifstream(array<T> const &filename) {
    this->m_fn = filename;
    this->m_mode = IOMode::READ;
    this->openstream();
  }
  basic_ifstream(HANDLE handle, bool isSeekable = true) {
    this->setSeekable(isSeekable);
    this->setFileName("(opened by handle)");
    this->m_mode = IOMode::READ;
    this->setHandle(handle);
  }

  virtual ssize_t rseek(ssize_t offset, IOPos position) {
    if (!this->m_isSeekable)
      return -1l;
    if (SetFilePointer(this->m_handle,
                       static_cast<LONG>(this->m_roffset) +
                           static_cast<LONG>(this->m_rbuffer.pos) -
                           static_cast<LONG>(this->m_rbuffer.size),
                       nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return -1l;
    }
    auto cur = SetFilePointer(this->m_handle, static_cast<LONG>(offset),
                              nullptr, static_cast<int>(position));
    if (cur == INVALID_SET_FILE_POINTER) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return -1l;
    }
    this->m_rbuffer.size = 0;
    this->m_rbuffer.pos = 0;
    return this->m_roffset = cur;
  }
  virtual ssize_t tellr() const {
    return this->m_roffset + static_cast<ssize_t>(this->m_rbuffer.pos) -
           static_cast<ssize_t>(this->m_rbuffer.size);
  }
  ssize_t tellend() {
    if (this->m_isSeekable)
      return SetFilePointer(this->m_handle, 0, nullptr, FILE_END);
    return 0l;
  }
  bool eof() {
    // if (this->m_isSeekable)
    //   return tellend() == tellr();
    // int n;

    // auto res = ioctl(0, FIONREAD, &n);
    // if (res == -1) {
    //   invoke(file_error_handler, __FILE__, __FUNCTION__);
    //   return true;
    // }
    // if (n == 0)
    //   return true;
    return GetFileSize(this->m_handle, NULL) == tellr();
  }
  static bool is_nl(T ch) { return ch == '\n'; }
  virtual pair<array<T>, size_t> readline() {
    auto result = readUntil(&is_nl);
    if (result.second > 1 && result.first[result.second - 1] == '\n') {
      result.first[result.second - 2] = '\n';
      --result.second;
    }
    return result;
  }
  virtual pair<array<T>, size_t> readUntil(bool (*predicate)(T)) {
    array<T> result{1};
    size_t totalRead = 0;
    bool firstReq = true;
    for (;;) {
      auto const prevsize = result.capacity() >> 1;
      auto [readsize, res] = readUntil(
          result, prevsize, result.capacity() - prevsize, predicate, firstReq);
      firstReq = false;
      totalRead += readsize;
      if (res)
        break;
      if (readsize + prevsize != result.capacity())
        break;
      array<T> newarray{result.capacity() << 1};
      for (size_t i = 0; i < result.capacity(); ++i)
        newarray[i] = result[i];
      result = forward<array<T>>(newarray);
    }
    return {result, totalRead};
  }
  virtual pair<size_t, bool> readUntil(array<T> &buffer, bool (*predicate)(T),
                                       bool firstReq = true) {
    return readUntil(buffer, 0, buffer.capacity(), predicate, firstReq);
  }
  virtual pair<size_t, bool> readUntil(array<T> &buffer, size_t start,
                                       size_t size, bool (*predicate)(T),
                                       bool firstReq = true) {
    size_t actualRead = 0;
    bool found = false;
    while (actualRead != size) {
      if (checkNeedsFill()) {
        auto filled = fillBuffer(firstReq);
        firstReq = false;
        if (filled == 0)
          break;
      }
      auto result = consumeUntil(buffer, actualRead + start, size - actualRead,
                                 predicate);
      actualRead += result.first;
      if (result.second) {
        found = true;
        break;
      }
    }
    return {actualRead, found};
  }
  virtual size_t read(array<T> &buffer, size_t size, bool firstReq = true) {
    size_t actualRead = 0;
    while (actualRead != size) {
      if (checkNeedsFill()) {
        auto filled = fillBuffer(firstReq);
        firstReq = false;
        if (filled == 0)
          break;
      }
      actualRead += consumeBuffer(buffer, actualRead, size - actualRead);
    }
    return actualRead;
  }

protected:
  bool checkNeedsFill() const { return this->m_rbuffer.size == 0; }
  virtual size_t fillBuffer(bool firstRequest = true) {
    if (this->m_isSeekable &&
        SetFilePointer(this->m_handle, static_cast<LONG>(this->m_roffset),
                       nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return 0ul;
    }
    if (!firstRequest && !this->m_isSeekable && eof())
      return 0l;
    errno = 0;
    DWORD rsize;
    if (!ReadFile(this->m_handle,
                  this->m_rbuffer.buf.data() + this->m_rbuffer.size,
                  static_cast<DWORD>(
                      (this->m_rbuffer.buf.capacity() - this->m_rbuffer.size) *
                      sizeof(T)),
                  &rsize, nullptr)) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return 0ul;
    }
    if (errno != 0) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return 0ul;
    }
    auto actualSize = rsize / sizeof(T);
    this->m_rbuffer.size += actualSize;
    this->m_roffset += rsize;
    return static_cast<size_t>(actualSize);
  }
  virtual size_t consumeBuffer(array<T> &buffer, size_t start, size_t size) {
    auto consumed = min(this->m_rbuffer.size - this->m_rbuffer.pos, size);
    memcpy(buffer.data() + start,
           this->m_rbuffer.buf.data() + this->m_rbuffer.pos,
           consumed * sizeof(T));
    this->m_rbuffer.pos += consumed;
    if (this->m_rbuffer.pos == this->m_rbuffer.size) {
      this->m_rbuffer.pos = 0;
      this->m_rbuffer.size = 0;
    }
    return consumed;
  }
  virtual pair<size_t, bool> consumeUntil(array<T> &buffer, size_t start,
                                          size_t size, bool (*predicate)(T)) {
    size_t consumed = this->m_rbuffer.pos;
    auto const predicted =
        min(this->m_rbuffer.size, size + this->m_rbuffer.pos);
    bool result = false;
    for (; consumed < predicted; ++consumed) {
      if (invoke(predicate, this->m_rbuffer.buf[consumed])) {
        result = true;
        ++consumed;
        break;
      }
    }
    consumed -= this->m_rbuffer.pos;
    memcpy(buffer.data() + start,
           this->m_rbuffer.buf.data() + this->m_rbuffer.pos,
           consumed * sizeof(T));
    this->m_rbuffer.pos += consumed;
    if (this->m_rbuffer.pos == this->m_rbuffer.size) {
      this->m_rbuffer.pos = 0;
      this->m_rbuffer.size = 0;
    }
    return {consumed, result};
  }
};
#ifdef __cpp_concepts
template <character_type T>
#else
template <typename T>
#endif
class basic_ofstream : public basic_fstream_windows<T> {
public:
  basic_ofstream(array<T> const &filename) {
    this->m_fn = filename;
    this->m_mode = IOMode::WRITE;
    this->openstream();
  }
  basic_ofstream(HANDLE handle, bool isSeekable = true) {
    this->setSeekable(isSeekable);
    this->setFileName("(opened by handle)");
    this->m_mode = IOMode::WRITE;
    this->setHandle(handle);
  }

  virtual ssize_t wseek(ssize_t offset, IOPos position) {
    if (!this->m_isSeekable)
      return -1l;
    flush();
    if (SetFilePointer(this->m_handle, static_cast<LONG>(this->m_woffset),
                       nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return -1l;
    }
    auto cur = SetFilePointer(this->m_handle, static_cast<LONG>(offset),
                              nullptr, static_cast<int>(position));
    if (cur == INVALID_SET_FILE_POINTER) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return -1l;
    }
    return this->m_woffset = cur;
  }
  virtual ssize_t tellw() const {
    return this->m_woffset + static_cast<ssize_t>(this->m_wbuffer.size);
  }
  virtual size_t write(T const *buffer) {
    array<T> arr{buffer, stringlen(buffer)};
    return this->write(arr);
  }
  virtual size_t write(array<T> const &buffer) {
    return this->write(buffer, buffer.capacity());
  }
  virtual size_t write(array<T> const &buffer, size_t size) {
    size_t actualWritten = 0;
    while (actualWritten != size) {
      auto filled = fillBuffer(buffer, actualWritten, size - actualWritten);
      actualWritten += filled;
      if (this->m_wbuffer.size == this->m_wbuffer.buf.capacity()) {
        if (flush() == 0)
          return actualWritten - filled;
      }
    }
    return actualWritten;
  }
  virtual size_t flush() {
    if (this->m_wbuffer.size == 0)
      return 0ul;
    if (this->m_isSeekable &&
        SetFilePointer(this->m_handle, static_cast<LONG>(this->m_woffset),
                       nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return 0ul;
    }
    DWORD wsize;
    if (!WriteFile(this->m_handle, this->m_wbuffer.buf.data(),
                   static_cast<DWORD>(this->m_wbuffer.size * sizeof(T)), &wsize,
                   nullptr) ||
        wsize != static_cast<DWORD>(this->m_wbuffer.size * sizeof(T))) {
      invoke(file_error_handler, __FILE__, __FUNCTION__);
      return 0ul;
    }
    auto actualSize = wsize / sizeof(T);
    this->m_wbuffer.size = 0;
    this->m_woffset += wsize;
    return static_cast<size_t>(actualSize);
  };

  ~basic_ofstream() { flush(); }

protected:
  virtual size_t fillBuffer(array<T> const &buffer, size_t start, size_t size) {
    size_t toFill =
        min(size, this->m_wbuffer.buf.capacity() - this->m_wbuffer.size);
    memcpy(this->m_wbuffer.buf.data() + this->m_wbuffer.size,
           buffer.data() + start, toFill * sizeof(T));
    this->m_wbuffer.size += toFill;
    return toFill;
  }
};

#endif

using ifstream = basic_ifstream<char>;
using ofstream = basic_ofstream<char>;
using iwfstream = basic_ifstream<short>;
using owfstream = basic_ofstream<short>;

extern ofstream cerr;
extern ifstream cin;
extern ofstream cout;
