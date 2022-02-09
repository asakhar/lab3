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
  virtual ~BasicFile() = default;

 protected:
  IOMode fmode;
  int fd = -1;
  array<char> fn{1, '\0'};
};

class File : public BasicFile {
 public:
  File(char const* filename, IOMode mode = READWRITE);
  File(int fd, IOMode mode = READWRITE);
  bool open(char const* filename, IOMode mode);
  void open(int fd, IOMode mode);
  int release();
  void close();
  array<char> readline();
  ssize_t read(array<char>& buffer, ssize_t size);
  ssize_t write(char const* buffer, size_t size);
  ssize_t write(array<char> const& buffer, ssize_t size);
  void wseek(long offset, IOPos pos);
  void rseek(long offset, IOPos pos);
  ssize_t flush();

  virtual ~File();

 private:
  static ssize_t constexpr BUFFER_SIZE = 5;
  struct {
    long off;
    int pos;
  } woffset = {0, SEEK_SET};
  struct {
    long off;
    int pos;
  } roffset = {0, SEEK_SET};
  struct {
    array<char> buf{BUFFER_SIZE, '\0'};
    ssize_t pos = 0;
    ssize_t size = 0;
  } rbuffer;
  struct {
    array<char> buf{BUFFER_SIZE, '\0'};
    ssize_t size = 0;
  } wbuffer;
};

class OFile : public File {
  OFile(char const* filename) : File(filename, WRITE) {}
  OFile(int fd) : File(fd, WRITE) {}
};

class IFile : public File {
  IFile(char const* filename) : File(filename, READ) {}
  IFile(int fd) : File(fd, READ) {}
};

extern File fin;
extern File fout;
extern File ferr;

#endif  // FILE_HPP