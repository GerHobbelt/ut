//
// Copyright (c) 2019-2020 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

// ensure no conflict between `Windows.h` and `ut.hpp`
#include <Windows.h>

#ifndef __MINGW32__
#if not defined(min) || not defined(max)
#error 'min' and 'max' should be defined
#endif
#endif

#include "boost/ut.hpp"

#ifndef __MINGW32__
#if not defined(min) || not defined(max)
#error 'min' and 'max' should still be defined
#endif
#endif

namespace ut = boost::ut;


#if defined(BUILD_MONOLITHIC)
#define main     boost_ut_test_win_compat_main
#endif

extern "C"
int main(void) {
  using namespace ut;
  expect(true);

  return 0;
}
