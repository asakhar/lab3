#include <unistd.h>

#include <cstdio>

#include "file.hpp"
#include <algorithm>
#include <sstream>

#define SSTR( x )  \
        ( std::ostringstream() << std::dec << x ).str()

void fileErrorHandler(int error) {
  ferr.write("Errno is set to ");
  auto errstr = SSTR(error);
  ferr.write(errstr.c_str());
  ferr.write("\nExiting...\n");
  exit(error);
}

int main(int argc, char const* argv[])
{
  errorHandler = fileErrorHandler;
  if(argc != 3) {
    ferr.write("Invalid number of arguments!\n");
    return 1;
  }
  char* endp;
  auto n = std::strtol(argv[1], &endp, 10);
  if(endp != argv[1]+strlen(argv[1])) {
    ferr.write("Invalid argument for number of symbols!\n");
    return 1;
  }
  File output{argv[2], File::IOMode::WRITE};
  output.wseek(0, File::IOPos::END);
  auto line = fin.readline();
  ssize_t len = strlen(line.get());
  size_t len2cpy = std::min(len, n+1);
  array<char> text{len2cpy, '\0'};
  memcpy(text.get(), line.get()+len-len2cpy, len2cpy);
  output.write(text, len2cpy);
  return 0;
}
