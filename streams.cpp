#include "streams.hpp"
#include <climits>
#include <cmath>
#include <math.h>
#include <unistd.h>

char *itoa(int value, char *result, int base) {
  // check that the base if valid
  if (base < 2 || base > 36) {
    *result = '\0';
    return result;
  }

  char *ptr = result, *ptr1 = result, tmp_char;
  int tmp_value;

  do {
    tmp_value = value;
    value /= base;
    *ptr++ =
        "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxy"
        "z"[35 + (tmp_value - value * base)];
  } while (value);

  // Apply negative sign
  if (tmp_value < 0)
    *ptr++ = '-';
  *ptr-- = '\0';
  while (ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr-- = *ptr1;
    *ptr1++ = tmp_char;
  }
  return result;
}

void default_file_error_handler(char const *FILE, char const *FUNCTION) {
  unsigned int constexpr base = 10;
  double constexpr log2_10 = 3.3219280948874;
  cerr.write(FILE);
  cerr.write(": error in function ");
  cerr.write(FUNCTION);
  cerr.write(": ");
  cerr.write(strerror(errno));
  cerr.write("(");
  int constexpr int_width =
      static_cast<int>(static_cast<double>(UINT_WIDTH) / log2_10) + 2;
  array<char> buffer{static_cast<size_t>(int_width)};
  for (auto &item : buffer)
    item = '\0';
  itoa(errno, buffer.data(), base);
  cerr.write(buffer);
  cerr.write(")\n");
  cerr.flush();
  exit(errno);
}
void (*file_error_handler)(char const *,
                           char const *) = &default_file_error_handler;

ofstream cerr{STDERR_FILENO, false};
ofstream cout{STDOUT_FILENO, false};
ifstream cin{STDIN_FILENO, false};
