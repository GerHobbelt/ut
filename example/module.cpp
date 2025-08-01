//
// Copyright (c) 2019-2020 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#if not defined(_MSC_VER)

import boost.ut;


#if defined(BUILD_MONOLITHIC)
#define main boost_ut_example_module_main
#endif

extern "C"
int main(void) {
  using namespace boost::ut;

  "module"_test = [] {
    // clang-format off
    expect(42_i == 42 and constant<3 == 3_i>);
    // clang-format on
    expect(std::vector{1, 2, 3} == std::vector{1, 2, 3});
  };

  return 0;
}

#endif
