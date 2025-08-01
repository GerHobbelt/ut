//
// Copyright (c) 2019-2020 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/ut.hpp>


#if defined(BUILD_MONOLITHIC)
#define main boost_ut_example_auto_main
#endif

// auto main(int argc, const char** argv) -> int { ... }     --> cannot overload functions by return type alone.
extern "C"
int main(int argc, const char** argv) {
  using namespace boost::ut;
  expect((argc == 2_i) >> fatal) << "Not enough parameters!";
  cfg<override> = {.filter = argv[1]};
  return cfg<override>.run();
}
