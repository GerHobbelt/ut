//
// Copyright (c) 2019-2020 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/ut.hpp>


#if defined(BUILD_MONOLITHIC)
#define main boost_ut_example_minimal_main
#endif

extern "C" int main(void) {
  boost::ut::expect(true);

  return 0;
}
