#include "streams.hpp"
#include <climits>
#include <cmath>
#ifdef _WIN32
#include <winbase.h>
#include <wtypesbase.h>
#elif __linux__
#include <unistd.h>
#endif

char *inttoa(int value, char *result, int base) {
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
#ifdef __linux__
  auto error = errno;
#elif _WIN32
  auto error = GetLastError();
#endif
  int constexpr error_width =
      static_cast<int>(static_cast<double>(sizeof(decltype(error)) * 8) /
                       log2_10) +
      2;
  array<char> buffer{static_cast<size_t>(error_width)};
  for (auto &item : buffer)
    item = '\0';
  cerr.write(FILE);
  cerr.write(": error in function ");
  cerr.write(FUNCTION);
  cerr.write(": ");
#ifdef __linux__
  cerr.write(strerror(error));
  cerr.write("(");
  inttoa(error, buffer.data(), base);
  cerr.write(buffer);
#elif _WIN32
  LPSTR message;
  DWORD dwMessageLen = FormatMessageA(
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, error,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPSTR>(&message), 0, NULL);
  cerr.write(array<char>(message, dwMessageLen));
  HeapFree(GetProcessHeap(), 0, message);
  cerr.write("(");
  inttoa(errno, buffer.data(), base);
  cerr.write(buffer);
#endif
  cerr.write(")\n");
  cerr.flush();
  exit(errno);
}
void (*file_error_handler)(char const *,
                           char const *) = &default_file_error_handler;

#ifdef __linux__
ofstream cerr{STDERR_FILENO, false};
ofstream cout{STDOUT_FILENO, false};
ifstream cin{STDIN_FILENO, false};
#elif _WIN32
ofstream cerr{GetStdHandle(STD_ERROR_HANDLE), false};
ofstream cout{GetStdHandle(STD_OUTPUT_HANDLE), false};
ifstream cin{GetStdHandle(STD_INPUT_HANDLE), false};
#endif