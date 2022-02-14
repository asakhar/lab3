#include <unistd.h>

#include "streams.hpp"

// #include <cstdio>

// #include "file.hpp"
// #include <algorithm>
// #include <sstream>

int main(int argc, char const* argv[])
{
  if(argc != 3) {
    cerr.write("Invalid number of arguments!\n");
    return 1;
  }
  char* endp;
  auto n = std::strtol(argv[1], &endp, 10);
  if(endp != argv[1]+strlen(argv[1])) {
    cerr.write("Invalid argument for number of symbols!\n");
    return 1;
  }
  ofstream output{array<char>(argv[2])};
  output.wseek(0, IOPos::END);
  auto line = cin.readline();
  ssize_t len = line.second;
  size_t len2cpy = min(len, n+1);
  array<char> text{len2cpy};
  memcpy(text.data(), line.first.data()+len-len2cpy, len2cpy);
  output.write(text, len2cpy);
  return 0;
}
