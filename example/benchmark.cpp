//
// Copyright (c) 2019-2020 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/ut.hpp>
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>

namespace benchmark {
struct benchmark : boost::ut::detail::test {
  explicit benchmark(std::string_view _name)
      : boost::ut::detail::test{"benchmark", _name} {}

  template <class Test>
  auto operator=(Test _test) {
    static_cast<boost::ut::detail::test&>(*this) = [&_test, this] {
      const auto start = std::chrono::high_resolution_clock::now();
      _test();
      const auto stop = std::chrono::high_resolution_clock::now();
      const auto ns =
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
      std::clog << "[" << name << "] " << ns.count() << " ns\n";
    };
  }
};

[[nodiscard]] auto operator""_benchmark(const char* _name,
                                        decltype(sizeof("")) size) {
  return ::benchmark::benchmark{{_name, size}};
}

#if defined(__GNUC__) or defined(__clang__)
template <class T>
void do_not_optimize(T&& t) {
  asm volatile("" ::"m"(t) : "memory");
}
#else
#pragma optimize("", off)
template <class T>
void do_not_optimize(T&& t) {
  reinterpret_cast<char volatile&>(t) =
      reinterpret_cast<char const volatile&>(t);
}
#pragma optimize("", on)
#endif
}  // namespace benchmark


#if defined(BUILD_MONOLITHIC)
#define main boost_ut_example_benchmark_main
#endif

extern "C"
int main(void) {
  using namespace boost::ut;
  using namespace benchmark;

  "string creation"_benchmark = [] {
    std::string created_string{"hello"};
    do_not_optimize(created_string);
  };

  return 0;
}
