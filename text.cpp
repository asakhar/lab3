#include "smartp.hpp"
#include "type_traits.hpp"
// #include "smartptrs2.hpp"

#include <iostream>

class S {
  long f;
  int a;

 public:
  long getF() const { return f; }
  long getA() const { return a; }
  S(long ff, int aa) : f{ff}, a{aa} {
    std::cout << "S::S(long(" << f << "), int(" << a << "))\n";
  }
  ~S() { std::cout << "S::~S()  f(" << f << "), a(" << a << ")\n"; }
  S(S const& s) : f{s.f}, a{s.a} {
    std::cout << "S::S(S const&(" << f << ", " << a << "))\n";
  }
  S(S&& s) : f{s.f}, a{s.a} {
    std::cout << "S::S(S&&(" << f << ", " << a << "))\n";
  }
  S& operator=(S const&) = default;
  S& operator=(S&&) = default;
};

class SW {
 public:
  S s;
  SW(S&& ss = S{0l, 1}) : s{ss} {
    std::cout << "SW::SW(S(" << s.getF() << ", " << s.getA() << "))\n";
  }
  ~SW() {
    std::cout << "SW::~SW()  S(" << s.getF() << ", " << s.getA() << ")\n";
  }
  SW(SW const& sw) : s{sw.s} {
    std::cout << "SW::SW(SW const&(" << s.getF() << ", " << s.getA() << "))\n";
  }
  SW(SW&& sw) : s{sw.s} {
    std::cout << "SW::SW(SW&&(" << s.getF() << ", " << s.getA() << "))\n";
  }
  SW& operator=(SW const&) = default;
  SW& operator=(SW&&) = default;
};

class NoDefault {
  int i;

 public:
  NoDefault() = delete;
  NoDefault(int ii) : i{ii} {
    std::cout << "NoDefault::NoDefault(int(" << i << "))\n";
  };
  NoDefault(NoDefault const& ii) : i{ii.i} {
    std::cout << "NoDefault::NoDefault(NoDefault const&(" << i << "))\n";
  };
  NoDefault(NoDefault&& ii) : i{ii.i} {
    std::cout << "NoDefault::NoDefault(NoDefault&&(" << i << "))\n";
  };
  ~NoDefault() { std::cout << "NoDefault::~NoDefault()  i(" << i << ")\n"; }
};

#include <cstring>

int main(int argc, char const* argv[]) {
  {
    if (argc < 3) return -1;
    char* end = nullptr;
    size_t n = std::strtol(argv[1], &end, 10);
    if (end != argv[1] + strlen(argv[1])) return -1;
    auto ptr = make_uniq<SW[]>(n);
    puts("");
    auto ptr2 = make_uniq<S>(-1, -2);
    puts("");
    auto ptr3 = make_uniq<S>(-3, -4);
    puts("");
    auto res = ptr3 == ptr2;
    std::cout << (res ? "true" : "false") << "\n";
    puts("\n");
    for (size_t i = 0; i < n; ++i) {
      ptr[i] = SW{{std::strtol(argv[i + 2], &end, 10), static_cast<int>(i)}};
      puts("");
    }
    puts("");
    ptr[1] = SW{};
    puts("");
    for (size_t i = 0; i < n; ++i) std::cout << ptr[i].s.getF() << " ";
    puts("\n");
    auto ptrs = make_uniq<NoDefault[]>(4, 2);
    puts("\n");
    auto ptrl = make_uniql<NoDefault[]>(NoDefault{4}, NoDefault{2});
    puts("\n");
  }
  {
    puts("\n\n\n");
    array<SW> arr{6};
  }
  return 0;
}
