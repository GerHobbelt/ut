//
// Copyright (c) 2019-2020 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/ut.hpp>

constexpr auto sum = [](auto... args) { return (0 + ... + args); };


#if defined(BUILD_MONOLITHIC)
#define main boost_ut_example_spec_main
#endif

extern "C"
int main(void) {
  using namespace boost::ut::operators::terse;
  using namespace boost::ut::literals;
  using namespace boost::ut::spec;

  describe("sum") = [] {
    it("should be 0") = [] { sum() == 0_i; };
    it("should add all args") = [] { sum(1, 2, 3) == 6_i; };
  };

  return 0;
}
