//
// Copyright (c) 2019-2020 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#undef NDEBUG
#include <boost/ut.hpp>
#include <cassert>


#if defined(BUILD_MONOLITHIC)
#define main boost_ut_example_abort_main
#endif

extern "C"
int main(void) {
  using namespace boost::ut;

#if __has_include(<unistd.h>) and __has_include(<sys/wait.h>)

  "abort"_test = [] {
    expect(not aborts([] {}));
    expect(aborts([] { assert(false); }));
    expect(aborts([] { throw; }));
  };

#endif

  return 0;
}
