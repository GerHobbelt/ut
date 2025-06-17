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
#define main boost_ut_example_hello_world_main
#endif

extern "C"
int main(void) {
  using namespace boost::ut;

  "hello world"_test = [] {
    expect(0_i == sum());
    expect(1_i == sum(1));
    expect(3_i == sum(1, 2));
  };

  return 0;
}
