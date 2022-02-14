#include "streams.hpp"

char* itoa(int value, char* result, int base) {
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
  if (tmp_value < 0) *ptr++ = '-';
  *ptr-- = '\0';
  while (ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr-- = *ptr1;
    *ptr1++ = tmp_char;
  }
  return result;
}

void default_file_error_handler(char const* FILE, char const* FUNCTION) {
  cerr.write(FILE);
  cerr.write(": error in function ");
  cerr.write(FUNCTION);
  cerr.write(": ");
  cerr.write(strerror(errno));
  cerr.write("(");
  array<char> buffer{11};
  for (auto& item : buffer) item = '\0';
  itoa(errno, buffer.data(), 10);
  cerr.write(buffer);
  cerr.write(")\n");
  cerr.flush();
}
void (*file_error_handler)(char const*,
                           char const*) = &default_file_error_handler;

ofstream cerr{1, false};
ofstream cout{1, false};
ifstream cin{0, false};
