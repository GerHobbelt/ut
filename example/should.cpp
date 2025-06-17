//
// Copyright (c) 2019-2020 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/ut.hpp>


#if defined(BUILD_MONOLITHIC)
#define main boost_ut_example_should_main
#endif

extern "C"
int main(void) {
  using namespace boost::ut;

  auto i = 0;
  expect((i == 0_i) >> fatal);

  should("return increased number for ++") = [i] { expect(++mut(i) == 1_i); };
  should("return decreased number for --") = [i]() mutable {
    expect(--i == -1_i);
  };

  return 0;
}
