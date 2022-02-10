#include "file.hpp"

#include <fcntl.h>
#include <unistd.h>

void doNothing(int) {}

void (*errorHandler)(int) = doNothing;

void BasicFile::open(int fileDescryptor, IOMode mode) {
  close();
  for(auto oi : _open_internal)
    std::invoke(oi, this);
  fd = fileDescryptor;
  fmode = mode;
  fn.reset();
}
bool BasicFile::open(char const *filename, IOMode mode) {
  close();
  for(auto oi : _open_internal)
    std::invoke(oi, this);
  int flags = O_RDONLY;
  if (mode == READWRITE)
    flags = O_RDWR;
  else if ((mode & WRITE) != 0)
    flags = O_WRONLY;
  else
    flags = O_RDONLY;
  fd = ::open(
      filename, flags | O_CREAT,
      ((mode & WRITE) != 0) ? (S_IWUSR | S_IWGRP | S_IRUSR | S_IRGRP) : 0);
  if (fd == -1) {
    perror("Opening file failed");
    errorHandler(errno);
    errno = 0;
    ::close(fd);
    fd = -1;
    return false;
  }
  fmode = mode;
  fn = array<char>(strlen(filename) + 1, '\0');
  strcpy(fn.get(), filename);
  return true;
}
void BasicFile::close() {
  if (fd != -1) {
    if (::close(fd) == -1) {
      perror("Error closing file");
      errorHandler(errno);
      errno = 0;
    }
    ::close(fd);
    fd = -1;
  }
  fn.reset();
}
File::~File() { close(); }
int BasicFile::release() {
  auto tmp = fd;
  fn.reset();
  fd = -1;
  return tmp;
}

File fin{STDIN_FILENO, File::IOMode::READ};
File fout{STDOUT_FILENO, File::IOMode::WRITE};
File ferr{STDERR_FILENO, File::IOMode::WRITE};
array<char> IFile::readline() {
  if ((fmode & READ) == 0) {
    // ##########
    return {1, '\0'};
  }
  lseek(fd, roffset.off, roffset.pos);
  array<char> buffer(BUFFER_SIZE, '\0');
  ssize_t size = BUFFER_SIZE;
  bool readNL = false;
  ssize_t totalRead = 0;
  while (!readNL) {
    ssize_t end = rbuffer.size;
    for (ssize_t i = rbuffer.pos; i < rbuffer.size; ++i) {
      if (rbuffer.buf[i] == '\n') {
        end = i + 1;
        readNL = true;
        break;
      }
    }
    auto actualRead = std::min(end - rbuffer.pos, size);
    if (actualRead != 0) {
      memcpy(&buffer[totalRead], rbuffer.buf.get() + rbuffer.pos, actualRead);
      rbuffer.pos += actualRead;
      totalRead += actualRead;
      size -= actualRead;
      if (rbuffer.pos == rbuffer.size) {
        rbuffer.size = 0;
        rbuffer.pos = 0;
      }
      if (totalRead == static_cast<long>(buffer.size())) {
        array<char> tmp{'\0', buffer.size() << 1};
        memcpy(tmp.get(), buffer.get(), buffer.size());
        size += buffer.size();
        buffer = std::move(tmp);
      }
    }
    if (readNL)
      break;
    auto res = ::read(fd, rbuffer.buf.get() + rbuffer.size,
                      BUFFER_SIZE - rbuffer.size);
    if (res == -1) {
      perror("Reading from file failed");
      errorHandler(errno);
      errno = 0;
      rbuffer.pos = 0;
      rbuffer.size = 0;
      return 0l;
    }
    if (res == 0) {
      return buffer;
    }
    rbuffer.size += res;
    end = rbuffer.size;
    for (ssize_t i = rbuffer.pos; i < rbuffer.size; ++i) {
      if (rbuffer.buf[i] == '\n') {
        end = i + 1;
        if (size >= end - rbuffer.pos)
          readNL = true;
        break;
      }
    }
    actualRead = std::min(size, end - rbuffer.pos);
    memcpy(&buffer[totalRead], rbuffer.buf.get() + rbuffer.pos, actualRead);
    rbuffer.pos += actualRead;
    roffset.off += rbuffer.size;
    totalRead += actualRead;
    size -= actualRead;
    if (rbuffer.pos == rbuffer.size) {
      rbuffer.size = 0;
      rbuffer.pos = 0;
    }
    if (totalRead == static_cast<long>(buffer.size())) {
      array<char> tmp{buffer.size() << 1, '\0'};
      memcpy(tmp.get(), buffer.get(), buffer.size());
      size += buffer.size();
      buffer = std::move(tmp);
    }
  }
  return buffer;
}
ssize_t IFile::read(array<char> &buffer, ssize_t size) {
  if ((fmode & READ) == 0) {
    // ##########
    return 0;
  }
  lseek(fd, roffset.off, roffset.pos);
  ssize_t totalRead = 0;
  while (size != 0) {
    auto actualRead = std::min(rbuffer.size - rbuffer.pos, size);
    if (actualRead != 0) {
      memcpy(&buffer[totalRead], rbuffer.buf.get() + rbuffer.pos, actualRead);
      rbuffer.pos += actualRead;
      totalRead += actualRead;
      size -= actualRead;
      if (rbuffer.pos == rbuffer.size) {
        rbuffer.size = 0;
        rbuffer.pos = 0;
      }
    }
    auto res = ::read(fd, rbuffer.buf.get() + rbuffer.size,
                      BUFFER_SIZE - rbuffer.size);
    if (res == -1) {
      perror("Reading from file failed");
      errorHandler(errno);
      errno = 0;
      rbuffer.pos = 0;
      rbuffer.size = 0;
      return 0l;
    }
    if (res == 0) {
      return totalRead;
    }
    rbuffer.size += res;
    actualRead = std::min(size, res);
    memcpy(&buffer[totalRead], rbuffer.buf.get() + rbuffer.pos, actualRead);
    rbuffer.pos += actualRead;
    roffset.off += rbuffer.size;
    totalRead += actualRead;
    size -= actualRead;
    if (rbuffer.pos == rbuffer.size) {
      rbuffer.size = 0;
      rbuffer.pos = 0;
    }
  }
  return totalRead;
}
ssize_t OFile::write(array<char> const &buffer, ssize_t size) {
  if ((fmode & WRITE) == 0) {
    // ##########
    return 0;
  }
  ssize_t actualWritten = 0;
  while (size > 0) {
    if (wbuffer.size == BUFFER_SIZE) {
      if (flush() == 0)
        return actualWritten - wbuffer.size;
    }
    auto toWrite = std::min(BUFFER_SIZE - wbuffer.size, size);
    memcpy(wbuffer.buf.get() + wbuffer.size, buffer.get() + actualWritten,
           toWrite);
    wbuffer.size += toWrite;
    size -= toWrite;
    actualWritten += toWrite;
  }
  return actualWritten;
}
void OFile::wseek(long offset, IOPos pos) {
  flush();
  woffset = {offset, pos};
  wbuffer.size = 0;
}
void IFile::rseek(long offset, IOPos pos) {
  roffset = {offset, pos};
  rbuffer.pos = 0;
  rbuffer.size = 0;
}
ssize_t OFile::flush() {
  if (wbuffer.size != 0) {
    lseek(fd, woffset.off, woffset.pos);
    auto res = ::write(fd, wbuffer.buf.get(), wbuffer.size);
    if (res == -1) {
      perror("Writing to file failed");
      errorHandler(errno);
      errno = 0;
      res = 0;
    } else {
      if (woffset.pos == BEGIN)
        woffset.off += res;
      wbuffer.size = 0;
    }
    return res;
  }
  return 0;
}
ssize_t OFile::write(char const *buffer, size_t size) {
  array<char> buf{size, '\0'};
  memcpy(buf.get(), buffer, size);
  return write(buf, size);
}
ssize_t OFile::write(char const *buffer) {
  size_t size = strlen(buffer);
  array<char> buf{size, '\0'};
  memcpy(buf.get(), buffer, size);
  return write(buf, size);
}
