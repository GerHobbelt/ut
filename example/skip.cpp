//
// Copyright (c) 2019-2020 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/ut.hpp>


#if defined(BUILD_MONOLITHIC)
#define main boost_ut_example_skip_main
#endif

extern "C"
int main(void) {
  using namespace boost::ut;

  // clang-format off
  skip /
  "don't run"_test = [] {
    expect(42_i == 43) << "should not fire!";
    expect(false) << "should fail!";
  };

  skip / test("don't run") = [] {
    expect(42_i == 43) << "should not fire!";
    expect(false) << "should fail!";
  };
  // clang-format on

  return 0;
}
