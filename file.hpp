#ifndef FILE_HPP
#define FILE_HPP
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "smartptrs.hpp"
extern void (*errorHandler)(int);

class BasicFile {
 public:
  enum IOMode { READ = 1, WRITE = 2, READWRITE = 3 };
  enum IOPos : int { BEGIN, CURRENT, END };
  BasicFile(char const* filename, IOMode mode) { open(filename, mode); }
  BasicFile(int fd, IOMode mode) { open(fd, mode); }
  virtual ~BasicFile() {
    close();
  };
  bool open(char const* filename, IOMode mode);
  void open(int fd, IOMode mode);
  virtual int release();
  virtual void close();

 protected:
  array<void (*)(BasicFile*)> _open_internal;
  IOMode fmode;
  int fd = -1;
  array<char> fn{1, '\0'};
};

class OFile : public virtual BasicFile {
 public:
  OFile(char const* filename) : BasicFile(filename, WRITE) {
    _open_internal = {1, &OFile::_open_internal_w};
  }
  OFile(int fd) : BasicFile(fd, WRITE) {
    _open_internal = {1, &OFile::_open_internal_w};
  }
  void close() override {
    flush();
    BasicFile::close();
  }
  int release() override {
    flush();
    return BasicFile::release();
  }
  virtual ssize_t write(char const* buffer, size_t size);
  virtual ssize_t write(char const* buffer);
  virtual ssize_t write(array<char> const& buffer, ssize_t size);
  virtual void wseek(long offset, IOPos pos);
  virtual ssize_t flush();

 protected:
  static void _open_internal_w(BasicFile* ptr) {
    dynamic_cast<OFile*>(ptr)->wbuffer.size = 0;
  }

 private:
  static ssize_t constexpr BUFFER_SIZE = 5;
  struct {
    long off;
    int pos;
  } woffset = {0, SEEK_SET};
  struct {
    array<char> buf{BUFFER_SIZE, '\0'};
    ssize_t size = 0;
  } wbuffer;
};

class IFile : public virtual BasicFile {
 public:
  IFile(char const* filename) : BasicFile(filename, READ) {
    _open_internal = {1, &IFile::_open_internal_r};
  }
  IFile(int fd) : BasicFile(fd, READ) {
    _open_internal = {1, &IFile::_open_internal_r};
  }

  virtual array<char> readline();
  virtual ssize_t read(array<char>& buffer, ssize_t size);
  virtual void rseek(long offset, IOPos pos);

 protected:
  static void _open_internal_r(BasicFile* ptr) {
    dynamic_cast<IFile*>(ptr)->rbuffer.size = 0;
    dynamic_cast<IFile*>(ptr)->rbuffer.pos = 0;
  }

 private:
  static ssize_t constexpr BUFFER_SIZE = 5;

  struct {
    long off;
    int pos;
  } roffset = {0, SEEK_SET};
  struct {
    array<char> buf{BUFFER_SIZE, '\0'};
    ssize_t pos = 0;
    ssize_t size = 0;
  } rbuffer;
};

class File : public virtual IFile, public virtual OFile {
 public:
  File(char const* filename, IOMode mode = READWRITE)
      : BasicFile(filename, mode), IFile(filename), OFile(filename) {
    fmode = mode;
    _open_internal = {2, nullptr};
    _open_internal[0] = &_open_internal_w;
    _open_internal[1] = &_open_internal_r;
  }
  File(int fileDescryptor, IOMode mode)
      : BasicFile(fileDescryptor, mode),
        IFile(fileDescryptor),
        OFile(fileDescryptor) {
    _open_internal = {2, nullptr};
    _open_internal[0] = &_open_internal_w;
    _open_internal[1] = &_open_internal_r;
  }

  ssize_t read(array<char>& buffer, ssize_t size) override {
    flush();
    return IFile::read(buffer, size);
  }
  void rseek(long offset, IOPos pos) override {
    flush();
    IFile::rseek(offset, pos);
  }
  array<char> readline() override {
    flush();
    return IFile::readline();
  }

  virtual ~File();
};

extern File fin;
extern File fout;
extern File ferr;

#endif  // FILE_HPP