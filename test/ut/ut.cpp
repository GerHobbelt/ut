//
// Copyright (c) 2019-2020 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#include "boost/ut.hpp"

#include <algorithm>
#include <any>
#include <array>
#include <complex>
#include <cstdlib>
#include <iostream>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ut = boost::ut;

namespace test_has_member_object {

struct not_defined {};

class private_member_object_value {
  int value;

 public:
  private_member_object_value(int value_) : value(value_) {}
};

struct public_member_object_value {
  int value;
};

struct public_static_member_object_value {
  static int value;
};

struct public_member_function_value {
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  int value() const { return 0; }
};

struct public_static_member_function_value {
  static int value() { return 0; }
};

class private_member_object_epsilon {
  int epsilon;

 public:
  private_member_object_epsilon(int epsilon_) : epsilon(epsilon_) {}
};

struct public_member_object_epsilon {
  int epsilon;
};

struct public_static_member_object_epsilon {
  static int epsilon;
};

struct public_member_function_epsilon {
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  int epsilon() const { return 0; }
};

struct public_static_member_function_epsilon {
  static int epsilon() { return 0; }
};

}  // namespace test_has_member_object

constexpr auto to_string = [](const auto& expr) {
  ut::printer printer{{.none = "", .pass = "", .fail = ""}};
  printer << std::boolalpha << expr;
  return printer.str();
};

namespace {
auto test_assert = [](const bool result,
                      const ut::reflection::source_location& sl =
                          ut::reflection::source_location::current()) {
  if (not result) {
    std::cerr << sl.file_name() << ':' << sl.line() << ":FAILED" << std::endl;
    std::abort();
  }
};
}  // namespace

struct fake_cfg {
  struct assertion_call {
    std::string expr{};
    ut::reflection::source_location location{};
    bool result{};
  };

  struct test_call {
    std::string_view type{};
    std::string name{};
    ut::reflection::source_location location{};
    std::any arg{};
  };

  template <class... Ts>
  auto on(ut::events::test<Ts...> test) -> void {
    if (std::find_if(std::cbegin(test.tag), std::cend(test.tag),
                     [](const auto& name) {
                       return ut::utility::is_match(name, "skip");
                     }) != std::cend(test.tag)) {
      on(ut::events::skip<>{.type = test.type, .name = test.name});
      return;
    }
    if (std::empty(test_filter) or std::string_view{test.name} == test_filter) {
      run_calls.push_back({.type = test.type,
                           .name = test.name,
                           .location = test.location,
                           .arg = test.arg});
      try {
        test();
      } catch (const ut::events::fatal_assertion&) {
      } catch (const ut::events::exception& exception) {
        exception_calls.push_back(exception.what());
      } catch (const std::exception& exception) {
        exception_calls.push_back(exception.what());
      } catch (...) {
        exception_calls.push_back("Unknown exception");
      }
    }
  }

  template <class... Ts>
  auto on(ut::events::skip<Ts...> test) -> void {
    skip_calls.push_back(
        {.type = test.type, .name = std::string{test.name}, .arg = test.arg});
  }

  template <class TExpr>
  auto on(const ut::events::assertion<TExpr>& assertion) -> bool {
    assertion_calls.push_back({.expr = to_string(assertion.expr),
                               .location = assertion.location,
                               .result = assertion.expr});
    return assertion.expr;
  }

  auto on(ut::events::fatal_assertion fatal_assertion) -> void {
    ++fatal_assertion_calls;
    throw fatal_assertion;
  }

  template <class TMsg>
  auto on(ut::events::log<TMsg> log) -> void {
    log_calls.push_back(log.msg);
  }

  std::vector<test_call> run_calls{};
  std::vector<test_call> skip_calls{};
  std::vector<std::any> log_calls{};
  std::vector<std::string> exception_calls{};
  std::vector<assertion_call> assertion_calls{};
  std::size_t fatal_assertion_calls{};
  std::string test_filter{};
};

template <class T>
class fake_calculator {
 public:
  auto enter(const T& value) -> void { values_.push_back(value); }
  auto name(std::string value) -> void { name_ = std::move(value); }
  auto add() -> void {
    result_ = std::accumulate(std::cbegin(values_), std::cend(values_), 0);
  }
  auto sub() -> void {
    result_ = std::accumulate(std::cbegin(values_) + 1, std::cend(values_),
                              values_.front(), std::minus{});
  }
  [[nodiscard]] auto get() const -> const T& { return result_; }
  [[nodiscard]] auto name() const -> std::string { return name_; }

 private:
  std::vector<T> values_{};
  T result_{};
  std::string name_{};
};

struct test_reporter : ut::reporter<ut::printer> {
  using reporter::asserts_;
  using reporter::tests_;
  using reporter::operator=;
};

struct test_runner : ut::runner<test_reporter> {
  using runner::reporter_;
  using runner::run_;
  using runner::operator=;
};

struct test_empty {
  auto operator()() -> void {}
};

struct test_throw {
  // NOLINTNEXTLINE(hicpp-exception-baseclass)
  auto operator()() -> void { throw 42; }
};

struct test_throw_runtime_error {
  auto operator()() -> void { throw std::runtime_error("exception"); }
};

struct test_assertion_true {
  explicit test_assertion_true(test_runner& _run) : run{_run} {}

  auto operator()() -> void {
    void(run.on(ut::events::assertion<bool>{.expr = true, .location = {}}));
  }

 private:
  test_runner& run;
};

struct test_assertion_false {
  explicit test_assertion_false(test_runner& _run) : run{_run} {}

  auto operator()() -> void {
    void(run.on(ut::events::assertion<bool>{.expr = false, .location = {}}));
  }

 private:
  test_runner& run;
};

struct test_assertions {
  explicit test_assertions(test_runner& _run) : run{_run} {}

  auto operator()() -> void {
    void(run.on(ut::events::assertion<bool>{.expr = false, .location = {}}));
    run.on(ut::events::fatal_assertion{});
    void(run.on(ut::events::assertion<bool>{.expr = true, .location = {}}));
  }

 private:
  test_runner& run;
};

struct test_sub_section {
  explicit test_sub_section(test_runner& _run) : run{_run} {}

  auto operator()() -> void {
    run.on(ut::events::test<test_empty>{.type = "test",
                                        .name = "sub-section",
                                        .location = {},
                                        .arg = ut::none{},
                                        .run = test_empty{}});
  }

 private:
  test_runner& run;
};

struct test_sub_sections {
  explicit test_sub_sections(test_runner& _run) : run{_run} {}

  auto operator()() -> void {
    run.on(ut::events::test<test_empty>{.type = "test",
                                        .name = "sub-section-1",
                                        .location = {},
                                        .arg = ut::none{},
                                        .run = test_empty{}});
    run.on(ut::events::test<test_throw>{.type = "test",
                                        .name = "sub-section-2",
                                        .location = {},
                                        .arg = ut::none{},
                                        .run = test_throw{}});
  }

 private:
  test_runner& run;
};

struct test_summary_reporter : ut::reporter<ut::printer> {
  auto count_summaries(std::size_t& counter) -> void {
    summary_counter_ = &counter;
  }

  auto count_runs(std::size_t& counter) -> void { runs_counter_ = &counter; }

  auto on(ut::events::summary) const -> void {
    if (summary_counter_) {
      ++*summary_counter_;
    }
  }

  auto on(ut::events::run_begin) const -> void {
    if (runs_counter_) {
      ++*runs_counter_;
    }
  }

  std::size_t* summary_counter_{};
  std::size_t* runs_counter_{};
};

struct test_summary_runner : ut::runner<test_summary_reporter> {
  using runner::reporter_;
};

namespace ns {
namespace {
template <char... Cs>
constexpr auto operator""_i() -> int {
  return sizeof...(Cs);
}
auto f() -> int { return 0_i; }
}  // namespace
}  //  namespace ns
namespace {
auto f() -> int {
  using namespace ns;
  return 42_i;
}
}  // namespace

struct custom {
  int i{};

  friend auto operator==(const custom& lhs, const custom& rhs) {
    return lhs.i == rhs.i;
  }

  friend auto operator!=(const custom& lhs, const custom& rhs) {
    return lhs.i != rhs.i;
  }

  friend auto operator<<(std::ostream& os, const custom& c) -> std::ostream& {
    return os << "custom{" << c.i << '}';
  }
};

struct custom_vec : std::vector<int> {
  using std::vector<int>::vector;

  friend auto operator<<(std::ostream& os, const custom_vec& c)
      -> std::ostream& {
    os << "custom_vec{";
    if (!c.empty()) {
      os << c.front();
      std::for_each(std::next(c.begin()), c.end(),
                    [&os](int const v) { os << ", " << v; });
    }
    os << '}';
    return os;
  }
};

struct custom_non_printable_type {
  int value;
};

struct custom_printable_type {
  int value;
};

// NOLINTBEGIN(misc-use-anonymous-namespace)
static std::string format_test_parameter(const custom_printable_type& value,
                                         [[maybe_unused]] const int counter) {
  return "custom_printable_type(" + std::to_string(value.value) + ")";
}

template <class... Ts>
static auto ut::cfg<ut::override, Ts...> = fake_cfg{};
// NOLINTEND(misc-use-anonymous-namespace)

// used for type_list check
#define DEFINE_NON_INSTANTIABLE_TYPE(Name, Value) \
  struct Name {                                   \
    Name() = delete;                              \
    Name(Name&&) = delete;                        \
    Name(const Name&) = delete;                   \
    Name& operator=(Name&&) = delete;             \
    Name& operator=(const Name&) = delete;        \
                                                  \
    static bool value() { return Value; }         \
  }

DEFINE_NON_INSTANTIABLE_TYPE(non_instantiable_A, true);
DEFINE_NON_INSTANTIABLE_TYPE(non_instantiable_B, true);
DEFINE_NON_INSTANTIABLE_TYPE(non_instantiable_C, false);
DEFINE_NON_INSTANTIABLE_TYPE(non_instantiable_D, true);

#undef DEFINE_NON_INSTANTIABLE_TYPE


#if defined(BUILD_MONOLITHIC)
#define main     boost_ut_test_ut_main
#endif

extern "C"
int main(void) {  // NOLINT(readability-function-size)
  {
    using namespace ut;
    using namespace std::literals::string_view_literals;

    {
      static_assert(std::is_same_v<type_traits::list<>,
                                   type_traits::function_traits<void()>::args>);
      static_assert(
          std::is_same_v<void,
                         type_traits::function_traits<void()>::result_type>);
      static_assert(
          std::is_same_v<type_traits::list<int>,
                         type_traits::function_traits<void(int)>::args>);
      static_assert(
          std::is_same_v<
              type_traits::list<int, const float&>,
              type_traits::function_traits<void(int, const float&)>::args>);
      static_assert(
          std::is_same_v<int,
                         type_traits::function_traits<int()>::result_type>);
    }

    {
      static_assert(!type_traits::has_static_member_object_value_v<
                    test_has_member_object::not_defined>);
      static_assert(!type_traits::has_static_member_object_value_v<
                    test_has_member_object::private_member_object_value>);
      static_assert(!type_traits::has_static_member_object_value_v<
                    test_has_member_object::public_member_object_value>);
      static_assert(type_traits::has_static_member_object_value_v<
                    test_has_member_object::public_static_member_object_value>);
      static_assert(!type_traits::has_static_member_object_value_v<
                    test_has_member_object::public_member_function_value>);
      static_assert(
          !type_traits::has_static_member_object_value_v<
              test_has_member_object::public_static_member_function_value>);

      static_assert(!type_traits::has_static_member_object_epsilon_v<
                    test_has_member_object::not_defined>);
      static_assert(!type_traits::has_static_member_object_epsilon_v<
                    test_has_member_object::private_member_object_epsilon>);
      static_assert(!type_traits::has_static_member_object_epsilon_v<
                    test_has_member_object::public_member_object_epsilon>);
      static_assert(
          type_traits::has_static_member_object_epsilon_v<
              test_has_member_object::public_static_member_object_epsilon>);
      static_assert(!type_traits::has_static_member_object_epsilon_v<
                    test_has_member_object::public_member_function_epsilon>);
      static_assert(
          !type_traits::has_static_member_object_epsilon_v<
              test_has_member_object::public_static_member_function_epsilon>);
    }

    {
      static_assert("void" == reflection::type_name<void>());
      static_assert("int" == reflection::type_name<int>());

#if defined(_MSC_VER) and not defined(__clang__)
      static_assert("struct fake_cfg" == reflection::type_name<fake_cfg>());
#else
      static_assert("fake_cfg" == reflection::type_name<fake_cfg>());
#endif
    }

    {
      static_assert(utility::regex_match("", ""));
      static_assert(utility::regex_match("hello", "hello"));
      static_assert(utility::regex_match("hello", "h.llo"));
      static_assert(utility::regex_match("hello", "he..o"));
      static_assert(not utility::regex_match("hello", "hella"));
      static_assert(not utility::regex_match("hello", "helao"));
      static_assert(not utility::regex_match("hello", "hlllo"));
      static_assert(not utility::regex_match("hello", ""));
      static_assert(not utility::regex_match("", "hello"));
      static_assert(not utility::regex_match("hi", "hello"));
      static_assert(not utility::regex_match("hello there", "hello"));
    }

    {
      test_assert(utility::is_match("", ""));
      test_assert(utility::is_match("", "*"));
      test_assert(utility::is_match("abc", "abc"));
      test_assert(utility::is_match("abc", "a?c"));
      test_assert(utility::is_match("abc", "a*"));
      test_assert(utility::is_match("abc", "a*c"));

      test_assert(not utility::is_match("", "abc"));
      test_assert(not utility::is_match("abc", "b??"));
      test_assert(not utility::is_match("abc", "a*d"));
      test_assert(not utility::is_match("abc", "*C"));
    }

    {
      test_assert(std::vector<std::string_view>{} ==
                  utility::split<std::string_view>("", "."));
      test_assert(std::vector<std::string_view>{"a"} ==
                  utility::split<std::string_view>("a.", "."));
      test_assert(std::vector<std::string_view>{"a", "b"} ==
                  utility::split<std::string_view>("a.b", "."));
      test_assert(std::vector<std::string_view>{"a", "b", "cde"} ==
                  utility::split<std::string_view>("a.b.cde", "."));
    }

    {
      static_assert("true"_b);
      static_assert((not "true"_b) != "true"_b);
      static_assert("named"_b);
      static_assert(42 == 42_i);
      static_assert(0u == 0_u);
      static_assert(42u == 42_u);
      static_assert(42 == 42_s);
      static_assert(42 == 42_c);
      static_assert(42 == 42_l);
      static_assert(42 == 42_ll);
      static_assert(42u == 42_uc);
      static_assert(42u == 42_us);
      static_assert(42u == 42_us);
      static_assert(42u == 42_ul);
      static_assert(42 == 42_i8);
      static_assert(42 == 42_i16);
      static_assert(42 == 42_i32);
      static_assert(42 == 42_i64);
      static_assert(42u == 42_u8);
      static_assert(42u == 42_u16);
      static_assert(42u == 42_u32);
      static_assert(42u == 42_u64);
      static_assert(42.42f == 42.42_f);
      static_assert(42.42 == 42.42_d);
      static_assert(240.209996948 == 240.209996948_d);
      static_assert(static_cast<long double>(42.42) == 42.42_ld);
      static_assert(240.20999694824218 == 240.20999694824218_ld);
      static_assert(0 == 0_i);
      static_assert(10'000 == 10'000_i);
      static_assert(42'000'000 == 42'000'000_i);
      static_assert(9'999 == 9'999_i);
      static_assert(42 == 42_i);
      static_assert(-42 == -42_i);
      static_assert(-42 == -42_i);
      static_assert(0. == 0.0_d);
      static_assert(0.f == .0_f);
      static_assert(0.1 == .1_d);
      static_assert(0.1 == 0.1_d);
      static_assert(0.1177 == 0.1177_d);
      static_assert(-0.1177 == -0.1177_d);
      static_assert(0.01177 == 0.01177_d);
      static_assert(-0.01177 == -0.01177_d);
      static_assert(0.001177 == 0.001177_d);
      static_assert(-0.001177 == -0.001177_d);
      static_assert(001.001177 == 001.001177_d);
      static_assert(-001.001177 == -001.001177_d);
      static_assert(01.001177 == 01.001177_d);
      static_assert(-01.001177 == -01.001177_d);
      static_assert(0.42f == 0.42_f);
      static_assert(2.42f == 2.42_f);
      static_assert(-2.42 == -2.42_d);
      static_assert(123.456 == 123.456_d);
    }

    {
      static_assert(not std::is_same_v<decltype(_sc(42)), decltype(_c(42))>);
      static_assert(not std::is_same_v<decltype(42_sc), decltype(42_c)>);
      static_assert(not std::is_same_v<decltype(_sc(42)), decltype(_uc(42u))>);
      static_assert(not std::is_same_v<decltype(42_sc), decltype(42_uc)>);
    }

    {
      static_assert(_i(42) == 42_i);
      static_assert(_b(true));
      static_assert(not _b(false));
      static_assert(_s(42) == 42_s);
      static_assert(_c(42) == 42_c);
      static_assert(_sc(42) == 42_sc);
      static_assert(_l(42) == 42_l);
      static_assert(_ll(42) == 42_ll);
      static_assert(_uc(42u) == 42_uc);
      static_assert(_us(42u) == 42_us);
      static_assert(_us(42u) == 42_us);
      static_assert(_ul(42u) == 42_ul);
      static_assert(_ull(42u) == 42_ull);
      static_assert(_ull(18'446'744'073'709'551'615ull) ==
                    18'446'744'073'709'551'615_ull);
    }

    {
      test_assert(_f(42.101f) == 42.101_f);
      test_assert(_f(42.101f, 0.01f) == 42.10_f);
      test_assert(_f(42.101f, 0.1f) != 42.1000_f);
      test_assert(_f(42.1010001f, 0.1f) == 42.1_f);
      test_assert(_f(42.101f) != 42.10_f);
      test_assert(_f(42.101f) != 42.100_f);
      test_assert(_f(42.10f) == 42.1_f);
      test_assert(_f(42.42f) == 42.42_f);
      test_assert(_d(42.42) == 42.420_d);
      test_assert(_d(42.0) == 42.0_d);
      test_assert(_d(42.) == 42._d);
      test_assert(_ld{static_cast<long double>(42.42)} == 42.42_ld);

      test_assert(42_i == 42_i);
      test_assert(1234._f == 1234.f);
      test_assert(1234.56_f == 1234.56f);
      test_assert(12345678.9f == 12345678.9_f);
      test_assert(111111.42f == 111111.42_f);
      test_assert(1111111111.42 == 1111111111.42_d);
      test_assert(42_i == 42);
      test_assert(43 == 43_i);
      test_assert(not(1_i == 2));
      test_assert(not(2 == 1_i));
    }

    {
      test_assert(_ul(42) == 42ul);
      test_assert(_i(42) == 42);
      test_assert(42 == _i(42));
      test_assert(true == _b(true));
      test_assert(_b(true) != _b(false));
      test_assert(_i(42) > _i({}));
      test_assert(42_i < 88);
      test_assert("true"_b != not "true"_b);
      test_assert(42 >= 42_i);
      test_assert(43 >= 42_i);
      test_assert(42 <= 42_i);
      test_assert(42 <= 43_i);
      test_assert((not "true"_b) == (not "true"_b));
    }

    {
      test_assert("true"_b and 42_i == 42_i);
      test_assert(not "true"_b or 42_i == 42_i);
      test_assert("true"_b or 42_i != 42_i);
      test_assert(not(42_i < 0));
    }

    {
      test_assert("0 == 0" == to_string(0_s == _s(0)));
      test_assert("0 != 0" == to_string(0_s != _s(0)));
      test_assert("0 < 0" == to_string(0_s < _s(0)));
      test_assert("0 > 0" == to_string(0_s > _s(0)));
      test_assert("0 >= 0" == to_string(0_s >= _s(0)));
      test_assert("0 <= 0" == to_string(0_s <= _s(0)));
      test_assert("0 != 0" == to_string(0_s != _s(0)));
      test_assert("not true" == to_string(!"true"_b));
      test_assert("42 == 42" == to_string(42_i == 42));
      test_assert("42 != 42" == to_string(42_i != 42));
      test_assert("42 > 0" == to_string(42_ul > 0_ul));
      test_assert("int == float" == to_string(type<int> == type<float>));
      test_assert("int == float" == to_string(type<>(int{}) == type<float>));
      test_assert("void != double" == to_string(type<void> != type<double>));
      test_assert("(true or 42.42 == 12.34)" ==
                  to_string("true"_b or (42.42_d == 12.34)));
      test_assert("(not 1 == 2 and str == str2)" ==
                  to_string(not(1_i == 2) and ("str"sv == "str2"sv)));
      test_assert("lhs == rhs" == to_string("lhs"_b == "rhs"_b));
    }

    {
      constexpr auto return_int = []() -> int { return {}; };
      static_assert(type<int> == return_int());
      static_assert(type<float> != return_int());
      static_assert(type<const int&> != return_int());
      static_assert(type<int&> != return_int());
      static_assert(type<int*> != return_int());
      static_assert(type<double> != return_int());
      static_assert(type<int> == type<int>);
      static_assert(!(type<int> != type<int>));
      static_assert(type<void> == type<void>);
      static_assert(type<void*> == type<void*>);
      static_assert(type<int> != type<const int>);
      static_assert(type<int> != type<void>);
    }

    {
      std::stringstream out{};
      std::stringstream err{};
      auto* old_cout = std::cout.rdbuf(out.rdbuf());
      auto* old_cerr = std::cerr.rdbuf(err.rdbuf());

      auto reporter = test_reporter{};

      reporter.on(events::test_begin{});
      reporter.on(events::test_run{});
      reporter.on(events::assertion_pass<bool>{
          .expr = true, .location = reflection::source_location::current()});
      reporter.on(events::assertion_fail<bool>{
          .expr = false, .location = reflection::source_location::current()});
      reporter.on(events::fatal_assertion{});
      reporter.on(events::test_end{});

      reporter.on(events::summary{});
      test_assert(std::empty(out.str()));
      test_assert(not std::empty(err.str()));
      test_assert(1 == reporter.asserts_.pass);
      test_assert(1 == reporter.asserts_.fail);
      test_assert(0 == reporter.tests_.skip);
      test_assert(1 == reporter.tests_.fail);
      test_assert(0 == reporter.tests_.pass);

      reporter.on(events::test_skip{});
      test_assert(1 == reporter.tests_.skip);

      std::cout.rdbuf(old_cout);
      std::cerr.rdbuf(old_cerr);
    }

    {
      test_runner run;
      auto& reporter = run.reporter_;
      run.run_ = true;

      run.on(events::test<test_empty>{.type = "test",
                                      .name = "run",
                                      .location = {},
                                      .arg = none{},
                                      .run = test_empty{}});
      test_assert(1 == reporter.tests_.pass);
      test_assert(0 == reporter.tests_.fail);
      test_assert(0 == reporter.tests_.skip);

      run.on(events::skip<>{.type = "test", .name = "skip", .arg = none{}});
      test_assert(1 == reporter.tests_.pass);
      test_assert(0 == reporter.tests_.fail);
      test_assert(1 == reporter.tests_.skip);

      run = {.filter = "unknown"};
      run.on(events::test<test_empty>{.type = "test",
                                      .name = "filter",
                                      .location = {},
                                      .arg = none{},
                                      .run = test_empty{}});
      test_assert(1 == reporter.tests_.pass);
      test_assert(0 == reporter.tests_.fail);
      test_assert(1 == reporter.tests_.skip);

      run = options{};

      run = {.filter = "filter"};
      run.on(events::test<test_empty>{.type = "test",
                                      .name = "filter",
                                      .location = {},
                                      .arg = none{},
                                      .run = test_empty{}});
      test_assert(2 == reporter.tests_.pass);
      test_assert(0 == reporter.tests_.fail);
      test_assert(1 == reporter.tests_.skip);

      run = options{};

      run.on(
          events::test<test_assertion_true>{.type = "test",
                                            .name = "pass",
                                            .location = {},
                                            .arg = none{},
                                            .run = test_assertion_true{run}});

      test_assert(3 == reporter.tests_.pass);
      test_assert(0 == reporter.tests_.fail);
      test_assert(1 == reporter.tests_.skip);

      run.on(
          events::test<test_assertion_false>{.type = "test",
                                             .name = "fail",
                                             .location = {},
                                             .arg = none{},
                                             .run = test_assertion_false{run}});
      test_assert(3 == reporter.tests_.pass);
      test_assert(1 == reporter.tests_.fail);
      test_assert(1 == reporter.tests_.skip);

      run.on(events::test<test_throw>{.type = "test",
                                      .name = "exception",
                                      .location = {},
                                      .arg = none{},
                                      .run = test_throw{}});
      test_assert(3 == reporter.tests_.pass);
      test_assert(2 == reporter.tests_.fail);
      test_assert(1 == reporter.tests_.skip);

      run.on(events::test<test_throw_runtime_error>{
          .type = "test",
          .name = "exception",
          .location = {},
          .arg = none{},
          .run = test_throw_runtime_error{}});
      test_assert(3 == reporter.tests_.pass);
      test_assert(3 == reporter.tests_.fail);
      test_assert(1 == reporter.tests_.skip);

      run.on(events::test<test_sub_section>{.type = "test",
                                            .name = "section",
                                            .location = {},
                                            .arg = none{},
                                            .run = test_sub_section{run}});
      test_assert(4 == reporter.tests_.pass);
      test_assert(3 == reporter.tests_.fail);
      test_assert(1 == reporter.tests_.skip);

      run = {.filter = "section"};
      run.on(events::test<test_sub_sections>{.type = "test",
                                             .name = "section",
                                             .location = {},
                                             .arg = none{},
                                             .run = test_sub_sections{run}});
      test_assert(4 == reporter.tests_.pass);
      test_assert(4 == reporter.tests_.fail);
      test_assert(1 == reporter.tests_.skip);

      run = options{};

      run = {.filter = "section.sub-section-1"};
      run.on(events::test<test_sub_sections>{.type = "test",
                                             .name = "section",
                                             .location = {},
                                             .arg = none{},
                                             .run = test_sub_sections{run}});
      test_assert(5 == reporter.tests_.pass);
      test_assert(4 == reporter.tests_.fail);
      test_assert(1 == reporter.tests_.skip);
      run = options{};

      run = {.filter = "section.sub-section-2"};
      run.on(events::test<test_sub_sections>{.type = "test",
                                             .name = "section",
                                             .location = {},
                                             .arg = none{},
                                             .run = test_sub_sections{run}});
      test_assert(5 == reporter.tests_.pass);
      test_assert(5 == reporter.tests_.fail);
      test_assert(1 == reporter.tests_.skip);
      run = options{};

      run = {.filter = "section.sub-section-*"};
      run.on(events::test<test_sub_sections>{.type = "test",
                                             .name = "section",
                                             .location = {},
                                             .arg = none{},
                                             .run = test_sub_sections{run}});
      test_assert(5 == reporter.tests_.pass);
      test_assert(6 == reporter.tests_.fail);
      test_assert(1 == reporter.tests_.skip);
      run = options{};

      run.on(events::test<test_assertions>{.type = "test",
                                           .name = "fatal",
                                           .location = {},
                                           .arg = none{},
                                           .run = test_assertions{run}});
      test_assert(5 == reporter.tests_.pass);
      test_assert(7 == reporter.tests_.fail);
      test_assert(1 == reporter.tests_.skip);

      run.on(
          events::test<test_assertion_true>{.type = "test",
                                            .name = "normal",
                                            .location = {},
                                            .arg = none{},
                                            .run = test_assertion_true{run}});
      test_assert(6 == reporter.tests_.pass);
      test_assert(7 == reporter.tests_.fail);
      test_assert(1 == reporter.tests_.skip);

      reporter = printer{};
    }

    {
      std::size_t summary_count = 0;
      {
        test_summary_runner run;
        run.reporter_.count_summaries(summary_count);
        test_assert(!run.run({.report_errors = true}));
      }
      test_assert(1 == summary_count);
    }

    {
      std::size_t run_count = 0;
      {
        auto run = test_summary_runner{};
        run.reporter_.count_runs(run_count);
        test_assert(!run.run({.report_errors = true}));
      }
      test_assert(1 == run_count);
    }

    auto& test_cfg = ut::cfg<ut::override>;

    {
      test_cfg = fake_cfg{};
      test_cfg.test_filter = "run";

      "run"_test = [] {};
      "ignore"_test = [] {};

      test_assert(1 == std::size(test_cfg.run_calls));
      test_assert("test"sv == test_cfg.run_calls[0].type);
      test_assert("run"sv == test_cfg.run_calls[0].name);
      void(std::any_cast<none>(test_cfg.run_calls[0].arg));
      test_assert(std::empty(test_cfg.skip_calls));
      test_assert(std::empty(test_cfg.log_calls));
      test_assert(0 == test_cfg.fatal_assertion_calls);
    }

    {
      test_cfg = fake_cfg{};

      "run"_test = [] {};
      skip / "skip"_test = [] {};

      test_assert(1 == std::size(test_cfg.run_calls));
      test_assert("run"sv == test_cfg.run_calls[0].name);
      test_assert(1 == std::size(test_cfg.skip_calls));
      test_assert("test"sv == test_cfg.skip_calls[0].type);
      test_assert("skip"sv == test_cfg.skip_calls[0].name);
      void(std::any_cast<none>(test_cfg.skip_calls[0].arg));
      test_assert(std::empty(test_cfg.log_calls));
      test_assert(0 == test_cfg.fatal_assertion_calls);
    }

    {
      test_cfg = fake_cfg{};

      tag("tag1") / tag("tag2") / "tag"_test = [] {};

      test_assert(1 == std::size(test_cfg.run_calls));
      test_assert("tag"sv == test_cfg.run_calls[0].name);
      test_assert(std::empty(test_cfg.skip_calls));
      test_assert(std::empty(test_cfg.log_calls));
      test_assert(0 == test_cfg.fatal_assertion_calls);
    }

    {
      test_cfg = fake_cfg{};

      "logging"_test = [] {
        boost::ut::log << "msg1"
                       << "msg2" << std::string{"msg3"} << 42;
      };

      test_assert(1 == std::size(test_cfg.run_calls));
      test_assert("logging"sv == test_cfg.run_calls[0].name);
      test_assert(8 == std::size(test_cfg.log_calls));
      test_assert('\n' == std::any_cast<char>(test_cfg.log_calls[0]));
      test_assert("msg1"sv ==
                  std::any_cast<const char*>(test_cfg.log_calls[1]));
      test_assert(' ' == std::any_cast<char>(test_cfg.log_calls[2]));
      test_assert("msg2"sv ==
                  std::any_cast<const char*>(test_cfg.log_calls[3]));
      test_assert(' ' == std::any_cast<char>(test_cfg.log_calls[4]));
      test_assert("msg3"sv ==
                  std::any_cast<std::string>(test_cfg.log_calls[5]));
      test_assert(' ' == std::any_cast<char>(test_cfg.log_calls[6]));
      test_assert(42 == std::any_cast<int>(test_cfg.log_calls[7]));
    }
#if defined(BOOST_UT_HAS_FORMAT)
    {
      test_cfg = fake_cfg{};

      "logging with format"_test = [] {
        boost::ut::log("{:<10} {:#x}\n{:<10} {}\n", "expected:", 42,
                       std::string("actual:"), 99);
      };

      test_assert(1 == std::size(test_cfg.run_calls));
      test_assert("logging with format"sv == test_cfg.run_calls[0].name);
      test_assert(1 == std::size(test_cfg.log_calls));
      test_assert("expected:  0x2a\nactual:    99\n"sv ==
                  std::any_cast<std::string>(test_cfg.log_calls[0]));
    }
#endif
    {
      test_cfg = fake_cfg{};
      expect(true) << "true msg" << true;
      expect(false) << "false msg" << false;

      test_assert(2 == std::size(test_cfg.assertion_calls));
      test_assert("true" == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[0].result);

      test_assert("false" == test_cfg.assertion_calls[1].expr);
      test_assert(not test_cfg.assertion_calls[1].result);
      test_assert(4 == std::size(test_cfg.log_calls));
      test_assert(' ' == std::any_cast<char>(test_cfg.log_calls[0]));
      test_assert("false msg"sv ==
                  std::any_cast<const char*>(test_cfg.log_calls[1]));
      test_assert(' ' == std::any_cast<char>(test_cfg.log_calls[2]));
      test_assert(not std::any_cast<bool>(test_cfg.log_calls[3]));
    }

    {
      test_cfg = fake_cfg{};
      auto f = [] { return "msg"; };
      expect(false) << f;

      test_assert(1 == std::size(test_cfg.assertion_calls));

      test_assert("false" == test_cfg.assertion_calls[0].expr);
      test_assert(not test_cfg.assertion_calls[0].result);
      test_assert(2 == std::size(test_cfg.log_calls));
      test_assert(' ' == std::any_cast<char>(test_cfg.log_calls[0]));
      test_assert("msg"sv == std::any_cast<const char*>(test_cfg.log_calls[1]));
    }

    {
      struct convertible_to_bool {
        constexpr operator bool() const { return false; }
      };

      struct explicit_convertible_to_bool {
        constexpr explicit operator bool() const { return true; }
      };

      test_cfg = fake_cfg{};

      expect(convertible_to_bool{});
      expect(static_cast<bool>(explicit_convertible_to_bool{}));

      test_assert(2 == std::size(test_cfg.assertion_calls));

      test_assert("false" == test_cfg.assertion_calls[0].expr);
      test_assert(not test_cfg.assertion_calls[0].result);

      test_assert("true" == test_cfg.assertion_calls[1].expr);
      test_assert(test_cfg.assertion_calls[1].result);
    }

    {
      test_cfg = fake_cfg{};

      // clang-format off
      {
        constexpr auto c = 4;
        auto r = 2;
        expect(constant<4_i == c> and r == 2_i);
      }

      {
        constexpr auto c = 5;
        auto r = 2;
        expect(constant<4_i == c> and r == 2_i) << "fail compile-time";
      }

      {
        constexpr auto c = 4;
        auto r = 3;
        expect(constant<4_i == c> and r == 2_i) << "fail run-time";
      }
      // clang-format on

      test_assert(3 == std::size(test_cfg.assertion_calls));

#if defined(__cpp_nontype_template_parameter_class)
      test_assert("(4 == 4 and 2 == 2)" == test_cfg.assertion_calls[0].expr);
#else
      test_assert("(true and 2 == 2)" == test_cfg.assertion_calls[0].expr);
#endif
      test_assert(test_cfg.assertion_calls[0].result);

      test_assert(4 == std::size(test_cfg.log_calls));

#if defined(__cpp_nontype_template_parameter_class)
      test_assert("(4 == 5 and 2 == 2)" == test_cfg.assertion_calls[1].expr);
#else
      test_assert("(false and 2 == 2)" == test_cfg.assertion_calls[1].expr);
#endif
      test_assert(not test_cfg.assertion_calls[1].result);
      test_assert(' ' == std::any_cast<char>(test_cfg.log_calls[0]));
      test_assert("fail compile-time"sv ==
                  std::any_cast<const char*>(test_cfg.log_calls[1]));

#if defined(__cpp_nontype_template_parameter_class)
      test_assert("(4 == 4 and 3 == 2)" == test_cfg.assertion_calls[2].expr);
#else
      test_assert("(true and 3 == 2)" == test_cfg.assertion_calls[2].expr);
#endif
      test_assert(not test_cfg.assertion_calls[2].result);
      test_assert(' ' == std::any_cast<char>(test_cfg.log_calls[2]));
      test_assert("fail run-time"sv ==
                  std::any_cast<const char*>(test_cfg.log_calls[3]));
    }

    {
      test_cfg = fake_cfg{};

      expect(1 == 2_i);
      expect(2 == 2_i);
      expect(1 != 2_i);
      expect(-42 != -42_i);
      expect(-1.1_d == -1.1);
      expect((not _b(true)) == not "true"_b);
      expect(2_i > 2_i) << "msg";

      test_assert(7 == std::size(test_cfg.assertion_calls));

      test_assert("1 == 2" == test_cfg.assertion_calls[0].expr);
      test_assert(not test_cfg.assertion_calls[0].result);

      test_assert("2 == 2" == test_cfg.assertion_calls[1].expr);
      test_assert(test_cfg.assertion_calls[1].result);

      test_assert("1 != 2" == test_cfg.assertion_calls[2].expr);
      test_assert(test_cfg.assertion_calls[2].result);

      test_assert("-42 != -42" == test_cfg.assertion_calls[3].expr);
      test_assert(not test_cfg.assertion_calls[3].result);

      test_assert("-1.1 == -1.1" == test_cfg.assertion_calls[4].expr);
      test_assert(test_cfg.assertion_calls[4].result);

      test_assert("not true == not true" == test_cfg.assertion_calls[5].expr);
      test_assert(test_cfg.assertion_calls[5].result);

      test_assert("2 > 2" == test_cfg.assertion_calls[6].expr);
      test_assert(not test_cfg.assertion_calls[6].result);
      test_assert(2 == std::size(test_cfg.log_calls));
      test_assert(' ' == std::any_cast<char>(test_cfg.log_calls[0]));
      test_assert("msg"sv == std::any_cast<const char*>(test_cfg.log_calls[1]));
    }

    {
      test_cfg = fake_cfg{};

      expect(that % 42 == 42);

      test_assert(1 == std::size(test_cfg.assertion_calls));
      test_assert("42 == 42" == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[0].result);
      test_assert(0 == std::size(test_cfg.log_calls));

      expect(that % 1 == 2 or that % 3 > 7) << "msg";
      test_assert(2 == std::size(test_cfg.assertion_calls));
      test_assert("(1 == 2 or 3 > 7)" == test_cfg.assertion_calls[1].expr);
      test_assert(not test_cfg.assertion_calls[1].result);
      test_assert(2 == std::size(test_cfg.log_calls));
      test_assert(' ' == std::any_cast<char>(test_cfg.log_calls[0]));
      test_assert("msg"sv == std::any_cast<const char*>(test_cfg.log_calls[1]));

      const auto f = false;
      expect(3_i > 5 and that % not true == f);
      test_assert(3 == std::size(test_cfg.assertion_calls));
      test_assert("(3 > 5 and false == false)" ==
                  test_cfg.assertion_calls[2].expr);
      test_assert(not test_cfg.assertion_calls[2].result);
    }

    {
      test_cfg = fake_cfg{};

      constexpr auto is_gt = [](auto N) {
        return [=](auto value) { return that % value > N; };
      };

      expect(is_gt(42)(99));
      expect(is_gt(2)(1));

      test_assert(2 == std::size(test_cfg.assertion_calls));

      test_assert("99 > 42" == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[0].result);

      test_assert("1 > 2" == test_cfg.assertion_calls[1].expr);
      test_assert(not test_cfg.assertion_calls[1].result);
    }

    {
      test_cfg = fake_cfg{};

      expect(eq(42, 42));

      test_assert(1 == std::size(test_cfg.assertion_calls));
      test_assert("42 == 42" == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[0].result);
      test_assert(0 == std::size(test_cfg.log_calls));

      expect(eq(1, 2) or gt(3, 7)) << "msg";
      test_assert(2 == std::size(test_cfg.assertion_calls));
      test_assert("(1 == 2 or 3 > 7)" == test_cfg.assertion_calls[1].expr);
      test_assert(not test_cfg.assertion_calls[1].result);
      test_assert(2 == std::size(test_cfg.log_calls));
      test_assert(' ' == std::any_cast<char>(test_cfg.log_calls[0]));
      test_assert("msg"sv == std::any_cast<const char*>(test_cfg.log_calls[1]));

      const auto f = false;
      expect(3_i > 5 and eq(not true, f));
      test_assert(3 == std::size(test_cfg.assertion_calls));
      test_assert("(3 > 5 and false == false)" ==
                  test_cfg.assertion_calls[2].expr);
      test_assert(not test_cfg.assertion_calls[2].result);
    }

    {
      test_cfg = fake_cfg{};

      expect(approx(42_i, 43_i, 2_i));
      test_assert(1 == std::size(test_cfg.assertion_calls));
      test_assert("42 ~ (43 +/- 2)" == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[0].result);

      expect(approx(3.141592654, 3.1415926536, 1e-9));
      test_assert(2 == std::size(test_cfg.assertion_calls));
      test_assert("3.14159 ~ (3.14159 +/- 1e-09)" ==
                  test_cfg.assertion_calls[1].expr);
      test_assert(test_cfg.assertion_calls[1].result);

      expect(approx(1_u, 2_u, 3_u));
      test_assert(3 == std::size(test_cfg.assertion_calls));
      test_assert("1 ~ (2 +/- 3)" == test_cfg.assertion_calls[2].expr);
      test_assert(test_cfg.assertion_calls[2].result);
    }

    {
      test_cfg = fake_cfg{};

      using namespace std::literals::string_view_literals;
      using namespace std::literals::string_literals;

      {
        std::stringstream str{};
        str << "should print" << ' ' << "string"s
            << " and "
            << "string_view"sv;
        test_assert("should print string and string_view" == str.str());
      }

      expect("str"s == "str"s);
      expect("str1"s != "str2"s);

      expect("str"sv == "str"sv);
      expect("str1"sv != "str2"sv);

      expect("str"sv == "str"s);
      expect("str1"sv != "str2"s);
      expect("str"s == "str"sv);
      expect("str1"s != "str2"sv);

      expect("str1"sv == "str2"sv);

      test_assert(9 == std::size(test_cfg.assertion_calls));

      test_assert("str == str" == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[0].result);

      test_assert("str1 != str2" == test_cfg.assertion_calls[1].expr);
      test_assert(test_cfg.assertion_calls[1].result);

      test_assert("str == str" == test_cfg.assertion_calls[2].expr);
      test_assert(test_cfg.assertion_calls[2].result);

      test_assert("str1 != str2" == test_cfg.assertion_calls[3].expr);
      test_assert(test_cfg.assertion_calls[3].result);

      test_assert("str == str" == test_cfg.assertion_calls[4].expr);
      test_assert(test_cfg.assertion_calls[4].result);

      test_assert("str1 != str2" == test_cfg.assertion_calls[5].expr);
      test_assert(test_cfg.assertion_calls[5].result);

      test_assert("str == str" == test_cfg.assertion_calls[6].expr);
      test_assert(test_cfg.assertion_calls[6].result);

      test_assert("str1 != str2" == test_cfg.assertion_calls[7].expr);
      test_assert(test_cfg.assertion_calls[7].result);

      test_assert("str1 == str2" == test_cfg.assertion_calls[8].expr);
      test_assert(not test_cfg.assertion_calls[8].result);
    }

    {
      test_cfg = fake_cfg{};

      expect(std::vector<int>{} == std::vector<int>{});
      expect(std::vector{'a', 'b'} == std::vector{'a', 'b'});
      expect(std::vector{1, 2, 3} == std::vector{1, 2});

      test_assert(3 == std::size(test_cfg.assertion_calls));

      test_assert(test_cfg.assertion_calls[0].result);
      test_assert("{} == {}" == test_cfg.assertion_calls[0].expr);

      test_assert(test_cfg.assertion_calls[1].result);
      test_assert("{a, b} == {a, b}" == test_cfg.assertion_calls[1].expr);

      test_assert(not test_cfg.assertion_calls[2].result);
      test_assert("{1, 2, 3} == {1, 2}" == test_cfg.assertion_calls[2].expr);
    }

    {
      test_cfg = fake_cfg{};

      expect(std::array{1} != std::array{2});
      expect(std::array{1, 2, 3} == std::array{3, 2, 1});

      test_assert(2 == std::size(test_cfg.assertion_calls));

      test_assert(test_cfg.assertion_calls[0].result);
      test_assert("{1} != {2}" == test_cfg.assertion_calls[0].expr);

      test_assert(not test_cfg.assertion_calls[1].result);
      test_assert("{1, 2, 3} == {3, 2, 1}" == test_cfg.assertion_calls[1].expr);
    }

    {
      test_cfg = fake_cfg{};

      expect(custom{42} == _t{custom{42}});
      expect(_t{custom{42}} == _t{custom{42}});
      expect(_t{custom{42}} == custom{42});
      expect(_t{custom{42}} != custom{24});
      expect(_t{custom{1}} == custom{2} or 1_i == 22);

      test_assert(5 == std::size(test_cfg.assertion_calls));

      test_assert(test_cfg.assertion_calls[0].result);
      test_assert("custom{42} == custom{42}" ==
                  test_cfg.assertion_calls[0].expr);

      test_assert(test_cfg.assertion_calls[1].result);
      test_assert("custom{42} == custom{42}" ==
                  test_cfg.assertion_calls[1].expr);

      test_assert(test_cfg.assertion_calls[2].result);
      test_assert("custom{42} == custom{42}" ==
                  test_cfg.assertion_calls[2].expr);

      test_assert(test_cfg.assertion_calls[3].result);
      test_assert("custom{42} != custom{24}" ==
                  test_cfg.assertion_calls[3].expr);

      test_assert(not test_cfg.assertion_calls[4].result);
      test_assert("(custom{1} == custom{2} or 1 == 22)" ==
                  test_cfg.assertion_calls[4].expr);
    }

    {
      test_cfg = fake_cfg{};

      expect(custom_vec{42, 5} == custom_vec{42, 5});
      expect(custom_vec{42, 5, 3} != custom_vec{42, 5, 6});

      test_assert(2 == std::size(test_cfg.assertion_calls));

      test_assert(test_cfg.assertion_calls[0].result);
      test_assert("custom_vec{42, 5} == custom_vec{42, 5}" ==
                  test_cfg.assertion_calls[0].expr);

      test_assert(test_cfg.assertion_calls[1].result);
      test_assert("custom_vec{42, 5, 3} != custom_vec{42, 5, 6}" ==
                  test_cfg.assertion_calls[1].expr);
    }

    {
      test_cfg = fake_cfg{};

      "assertions"_test = [] { expect(42_i == 42); };

      test_assert(1 == std::size(test_cfg.run_calls));
      test_assert("assertions"sv == test_cfg.run_calls[0].name);
      test_assert(std::empty(test_cfg.skip_calls));
      test_assert(std::empty(test_cfg.log_calls));
      test_assert(0 == test_cfg.fatal_assertion_calls);
      test_assert(1 == std::size(test_cfg.assertion_calls));
      test_assert("42 == 42" == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[0].result);
    }

    {
      test_cfg = fake_cfg{};

      "fatal assertions via function"_test = [] {
        expect(fatal(1_i == 1));
        expect(fatal(2 != 2_i)) << "fatal";
      };

      "fatal assertions via operator"_test = [] {
        expect((1_i == 1) >> fatal);
        expect((2 != 2_i) >> fatal) << "fatal";
      };

      test_assert(2 == std::size(test_cfg.run_calls));
      test_assert(4 == std::size(test_cfg.assertion_calls));
      test_assert(test_cfg.assertion_calls[0].result);

      test_assert("1 == 1" == test_cfg.assertion_calls[0].expr);
      test_assert(not test_cfg.assertion_calls[1].result);
      test_assert("2 != 2" == test_cfg.assertion_calls[1].expr);

      test_assert("1 == 1" == test_cfg.assertion_calls[2].expr);
      test_assert(not test_cfg.assertion_calls[3].result);
      test_assert("2 != 2" == test_cfg.assertion_calls[3].expr);

      test_assert(2 == test_cfg.fatal_assertion_calls);
      test_assert(std::empty(test_cfg.log_calls));
    }

    {
      test_cfg = fake_cfg{};

      "fatal assertions logging"_test = [] {
        expect(2 != 2_i) << "fatal" << fatal;
      };

      test_assert(1 == std::size(test_cfg.run_calls));
      test_assert(1 == std::size(test_cfg.assertion_calls));
      test_assert(not test_cfg.assertion_calls[0].result);
      test_assert("2 != 2" == test_cfg.assertion_calls[0].expr);
      test_assert(1 == test_cfg.fatal_assertion_calls);
      test_assert(2 == std::size(test_cfg.log_calls));
      test_assert(' ' == std::any_cast<char>(test_cfg.log_calls[0]));
      test_assert("fatal"sv ==
                  std::any_cast<const char*>(test_cfg.log_calls[1]));
    }

    {
      // NOLINTBEGIN(hicpp-exception-baseclass)
      test_cfg = fake_cfg{};

      "exceptions"_test = [] {
        expect(throws<std::runtime_error>([] { throw std::runtime_error{""}; }))
            << "throws runtime_error";
        expect(throws([] { throw 0; })) << "throws any exception";
        expect(throws<std::runtime_error>([] { throw 0; }))
            << "throws wrong exception";
        expect(throws<std::runtime_error>([] {})) << "doesn't throw";
        expect(nothrow([] {})) << "doesn't throw";
        expect(nothrow([] { throw 0; })) << "throws";
      };

      test_assert(1 == std::size(test_cfg.run_calls));
      test_assert("exceptions"sv == test_cfg.run_calls[0].name);
      test_assert(6 == std::size(test_cfg.assertion_calls));
      test_assert(test_cfg.assertion_calls[0].result);
#if defined(_MSC_VER) and not defined(__clang__)
      test_assert("throws<class std::runtime_error>" ==
                  test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[1].result);
      test_assert("throws" == test_cfg.assertion_calls[1].expr);
      test_assert(not test_cfg.assertion_calls[2].result);
      test_assert("throws<class std::runtime_error>" ==
                  test_cfg.assertion_calls[2].expr);
      test_assert(not test_cfg.assertion_calls[3].result);
      test_assert("throws<class std::runtime_error>" ==
                  test_cfg.assertion_calls[3].expr);
#else
      test_assert("throws<std::runtime_error>" ==
                  test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[1].result);
      test_assert("throws" == test_cfg.assertion_calls[1].expr);
      test_assert(not test_cfg.assertion_calls[2].result);
      test_assert("throws<std::runtime_error>" ==
                  test_cfg.assertion_calls[2].expr);
      test_assert(not test_cfg.assertion_calls[3].result);
      test_assert("throws<std::runtime_error>" ==
                  test_cfg.assertion_calls[3].expr);
#endif
      test_assert(test_cfg.assertion_calls[4].result);
      test_assert("nothrow" == test_cfg.assertion_calls[4].expr);
      test_assert(not test_cfg.assertion_calls[5].result);
      test_assert("nothrow" == test_cfg.assertion_calls[5].expr);

      "should throw"_test = [] {
        auto f = [](const auto should_throw) {
          if (should_throw) {
            throw 42;
          }
          return 42;
        };
        expect(42_i == f(true));
      };

      "std::exception throw"_test = [] {
        throw std::runtime_error("std::exception message");
      };

      "events::exception throw"_test = [] {
        throw events::exception{.msg = "events::exception message"};
      };

      "generic throw"_test = [] { throw 0; };

      test_assert(5 == std::size(test_cfg.run_calls));
      test_assert("should throw"sv == test_cfg.run_calls[1].name);
      test_assert("std::exception throw"sv == test_cfg.run_calls[2].name);
      test_assert("events::exception throw"sv == test_cfg.run_calls[3].name);
      test_assert("generic throw"sv == test_cfg.run_calls[4].name);
      test_assert("std::exception message"sv == test_cfg.exception_calls[1]);
      test_assert("events::exception message"sv == test_cfg.exception_calls[2]);
      test_assert("Unknown exception"sv == test_cfg.exception_calls[3]);
      test_assert(6 == std::size(test_cfg.assertion_calls));
      // NOLINTEND(hicpp-exception-baseclass)
    }

#if __has_include(<unistd.h>) and __has_include(<sys/wait.h>) and not defined(EMSCRIPTEN)
    {
      test_cfg = fake_cfg{};

      "abort"_test = [] {
        expect(aborts([] {}));
        expect(aborts([] { throw; }));
      };

      test_assert(1 == std::size(test_cfg.run_calls));
      test_assert("abort"sv == test_cfg.run_calls[0].name);
      test_assert(2 == std::size(test_cfg.assertion_calls));
      test_assert(not test_cfg.assertion_calls[0].result);
      test_assert("aborts" == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[1].result);
      test_assert("aborts" == test_cfg.assertion_calls[1].expr);
    }
#endif

    {
      test_cfg = fake_cfg{};
      [[maybe_unused]] struct {
      } empty{};

      "[capture empty]"_test = [=] {};

      test_assert(1 == std::size(test_cfg.run_calls));
      test_assert("[capture empty]"sv == test_cfg.run_calls[0].name);
    }

    {
      test_cfg = fake_cfg{};

      "[vector]"_test = [] {
        std::vector<int> v(5);

        expect((5_ul == std::size(v)) >> fatal);

        "resize bigger"_test = [=] {
          mut(v).resize(10);
          expect(10_ul == std::size(v));
        };

        expect((5_ul == std::size(v)) >> fatal);

        "resize smaller"_test = [=]() mutable {
          v.resize(0);
          expect(0_ul == std::size(v));
        };
      };

      test_assert(3 == std::size(test_cfg.run_calls));
      test_assert("[vector]"sv == test_cfg.run_calls[0].name);
      test_assert("resize bigger"sv == test_cfg.run_calls[1].name);
      test_assert("resize smaller"sv == test_cfg.run_calls[2].name);
      test_assert(4 == std::size(test_cfg.assertion_calls));
      test_assert(test_cfg.assertion_calls[0].result);
      test_assert("5 == 5" == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[1].result);
      test_assert("10 == 10" == test_cfg.assertion_calls[1].expr);
      test_assert(test_cfg.assertion_calls[2].result);
      test_assert("5 == 5" == test_cfg.assertion_calls[2].expr);
      test_assert(test_cfg.assertion_calls[3].result);
      test_assert("0 == 0" == test_cfg.assertion_calls[3].expr);
    }

    {
      test_cfg = fake_cfg{};

      "args vector"_test = [](const auto& arg) {
        expect(arg > 0_i) << "all values greater than 0";
      } | std::vector{1, 2, 3};

      test_assert(3 == std::size(test_cfg.run_calls));
      test_assert("args vector (1)"sv == test_cfg.run_calls[0].name);
      test_assert(1 == std::any_cast<int>(test_cfg.run_calls[0].arg));
      test_assert("args vector (2)"sv == test_cfg.run_calls[1].name);
      test_assert(2 == std::any_cast<int>(test_cfg.run_calls[1].arg));
      test_assert("args vector (3)"sv == test_cfg.run_calls[2].name);
      test_assert(3 == std::any_cast<int>(test_cfg.run_calls[2].arg));
      test_assert(3 == std::size(test_cfg.assertion_calls));
      test_assert(test_cfg.assertion_calls[0].result);
      test_assert("1 > 0" == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[1].result);
      test_assert("2 > 0" == test_cfg.assertion_calls[1].expr);
      test_assert(test_cfg.assertion_calls[2].result);
      test_assert("3 > 0" == test_cfg.assertion_calls[2].expr);
    }

    {
      test_cfg = fake_cfg{};

      "args array"_test = [](const auto& arg) {
        expect(0_i <= arg) << "all values greater than 0";
      } | std::array{99, 11};

      test_assert(2 == std::size(test_cfg.run_calls));
      test_assert("args array (99)"sv == test_cfg.run_calls[0].name);
      test_assert(99 == std::any_cast<int>(test_cfg.run_calls[0].arg));
      test_assert("args array (11)"sv == test_cfg.run_calls[1].name);
      test_assert(11 == std::any_cast<int>(test_cfg.run_calls[1].arg));
      test_assert(2 == std::size(test_cfg.assertion_calls));
      test_assert(test_cfg.assertion_calls[0].result);
      test_assert("0 <= 99" == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[1].result);
      test_assert("0 <= 11" == test_cfg.assertion_calls[1].expr);
    }

    {
      test_cfg = fake_cfg{};

      "args string"_test = [](const auto& arg) {
        expect(_c('s') == arg or _c('t') == arg or _c('r') == arg);
      } | std::string{"str"};

      test_assert(3 == std::size(test_cfg.run_calls));
      test_assert("args string (s)"sv == test_cfg.run_calls[0].name);
      test_assert('s' == std::any_cast<char>(test_cfg.run_calls[0].arg));
      test_assert("args string (t)"sv == test_cfg.run_calls[1].name);
      test_assert('t' == std::any_cast<char>(test_cfg.run_calls[1].arg));
      test_assert("args string (r)"sv == test_cfg.run_calls[2].name);
      test_assert('r' == std::any_cast<char>(test_cfg.run_calls[2].arg));
      test_assert(3 == std::size(test_cfg.assertion_calls));
      test_assert(test_cfg.assertion_calls[0].result);
      test_assert("((s == s or t == s) or r == s)" ==
                  test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[1].result);
      test_assert("((s == t or t == t) or r == t)" ==
                  test_cfg.assertion_calls[1].expr);
      test_assert(test_cfg.assertion_calls[2].result);
      test_assert("((s == r or t == r) or r == r)" ==
                  test_cfg.assertion_calls[2].expr);
    }

    {
      test_cfg = fake_cfg{};

      "args map"_test = [](const auto& arg) {
        const auto [key, value] = arg;
        expect(_c(key) == 'a' or _c('b') == key);
        expect(value > 0_i);
      } | std::map<char, int>{{'a', 1}, {'b', 2}};

      test_assert(2 == std::size(test_cfg.run_calls));
      test_assert("args map (1st parameter)"sv == test_cfg.run_calls[0].name);
      test_assert(
          std::pair<const char, int>{'a', 1} ==
          std::any_cast<std::pair<const char, int>>(test_cfg.run_calls[0].arg));
      test_assert("args map (2nd parameter)"sv == test_cfg.run_calls[1].name);
      test_assert(
          std::pair<const char, int>{'b', 2} ==
          std::any_cast<std::pair<const char, int>>(test_cfg.run_calls[1].arg));
      test_assert(4 == std::size(test_cfg.assertion_calls));
      test_assert(test_cfg.assertion_calls[0].result);
      test_assert("(a == a or b == a)" == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[1].result);
      test_assert("1 > 0" == test_cfg.assertion_calls[1].expr);
      test_assert(test_cfg.assertion_calls[2].result);
      test_assert("(b == a or b == b)" == test_cfg.assertion_calls[2].expr);
      test_assert(test_cfg.assertion_calls[3].result);
      test_assert("2 > 0" == test_cfg.assertion_calls[3].expr);
    }

    {
      test_cfg = fake_cfg{};

      "types"_test = []<class T>() {
        expect(std::is_integral<T>{} or
               type<void> == type<std::remove_pointer_t<T>>)
            << "all types are integrals or void";
      } | std::tuple<bool, int, void*>{};

      test_assert(3 == std::size(test_cfg.run_calls));
      test_assert("types (bool)"sv == test_cfg.run_calls[0].name);
      void(std::any_cast<bool>(test_cfg.run_calls[0].arg));
      test_assert("types (int)"sv == test_cfg.run_calls[1].name);
      void(std::any_cast<int>(test_cfg.run_calls[1].arg));
      // the pointer is printed differently on different platforms, so we only check the prefix
      test_assert(test_cfg.run_calls[2].name.starts_with("types (void"));
      void(std::any_cast<void*>(test_cfg.run_calls[2].arg));
      test_assert(3 == std::size(test_cfg.assertion_calls));
      test_assert("(true or void == bool)" == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[0].result);
      test_assert("(true or void == int)" == test_cfg.assertion_calls[1].expr);
      test_assert(test_cfg.assertion_calls[1].result);
      test_assert("(false or void == void)" ==
                  test_cfg.assertion_calls[2].expr);
      test_assert(test_cfg.assertion_calls[2].result);
    }

    {
      test_cfg = fake_cfg{};

      using types = type_list<non_instantiable_A, non_instantiable_B,
                              non_instantiable_C, non_instantiable_D>;

      "uninstantiated types"_test = []<class T>() {
        expect(T::value()) << "all static member function should return true";
      } | types{};

      test_assert(4 == std::size(test_cfg.run_calls));
      test_assert("uninstantiated types (non_instantiable_A)"sv ==
                  test_cfg.run_calls[0].name);
      void(std::any_cast<detail::wrapped_type<non_instantiable_A>>(
          test_cfg.run_calls[0].arg));
      test_assert("uninstantiated types (non_instantiable_B)"sv ==
                  test_cfg.run_calls[1].name);
      void(std::any_cast<detail::wrapped_type<non_instantiable_B>>(
          test_cfg.run_calls[1].arg));
      test_assert("uninstantiated types (non_instantiable_C)"sv ==
                  test_cfg.run_calls[2].name);
      void(std::any_cast<detail::wrapped_type<non_instantiable_C>>(
          test_cfg.run_calls[2].arg));
      test_assert("uninstantiated types (non_instantiable_D)"sv ==
                  test_cfg.run_calls[3].name);
      void(std::any_cast<detail::wrapped_type<non_instantiable_D>>(
          test_cfg.run_calls[3].arg));

      test_assert(4 == std::size(test_cfg.assertion_calls));
      test_assert("true"sv == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[0].result);
      test_assert("true"sv == test_cfg.assertion_calls[1].expr);
      test_assert(test_cfg.assertion_calls[1].result);
      test_assert("false"sv == test_cfg.assertion_calls[2].expr);
      test_assert(not test_cfg.assertion_calls[2].result);
      test_assert("true"sv == test_cfg.assertion_calls[3].expr);
      test_assert(test_cfg.assertion_calls[3].result);
    }

    {
      test_cfg = fake_cfg{};

      "args and types"_test = []<class TArg>(const TArg& arg) {
        expect(42_i == arg or arg == _c('x'));
        expect(type<TArg> == type<int> or type<TArg> == type<char>);
      } | std::tuple{42, 'x'};

      test_assert(2 == std::size(test_cfg.run_calls));
      test_assert("args and types (42, int)"sv == test_cfg.run_calls[0].name);
      test_assert(42 == std::any_cast<int>(test_cfg.run_calls[0].arg));
      test_assert("args and types (x, char)"sv == test_cfg.run_calls[1].name);
      test_assert('x' == std::any_cast<char>(test_cfg.run_calls[1].arg));
      test_assert(4 == std::size(test_cfg.assertion_calls));
      test_assert(test_cfg.assertion_calls[0].result);
      test_assert(test_cfg.assertion_calls[1].result);
      test_assert(test_cfg.assertion_calls[2].result);
      test_assert(test_cfg.assertion_calls[3].result);
      test_assert("(42 == 42 or 42 == x)" == test_cfg.assertion_calls[0].expr);
      test_assert("(int == int or int == char)" ==
                  test_cfg.assertion_calls[1].expr);
      test_assert("(42 == x or x == x)" == test_cfg.assertion_calls[2].expr);
      test_assert("(char == int or char == char)" ==
                  test_cfg.assertion_calls[3].expr);
    }

    {
      test_cfg = fake_cfg{};

      "parameterized test names"_test =
          []<class TArg>(const TArg& /*arg*/) { expect(true); } |
          std::tuple{custom_non_printable_type{0},
                     std::string{"str"},
                     "str",
                     "str"sv,
                     42.5,
                     false,
                     std::complex{1.5, 2.0},
                     custom_printable_type{42}};

      test_assert(8 == std::size(test_cfg.run_calls));
      test_assert(test_cfg.run_calls[0].name.starts_with(
          "parameterized test names (1st parameter, "));
      test_assert(test_cfg.run_calls[0].name.find(
                      "custom_non_printable_type") != std::string::npos);
      test_assert(test_cfg.run_calls[1].name.starts_with(
          "parameterized test names (2nd parameter, "));
      test_assert(test_cfg.run_calls[1].name.find("string") !=
                  std::string::npos);
      test_assert(test_cfg.run_calls[2].name.starts_with(
          "parameterized test names (3rd parameter, const char"));
      test_assert(test_cfg.run_calls[2].name.find('*') != std::string::npos);
      test_assert(test_cfg.run_calls[3].name.starts_with(
          "parameterized test names (4th parameter, "));
      test_assert(test_cfg.run_calls[3].name.find("string_view") !=
                  std::string::npos);
      test_assert(test_cfg.run_calls[4].name ==
                  "parameterized test names (42.5, double)"sv);
      test_assert(test_cfg.run_calls[5].name ==
                  "parameterized test names (false, bool)"sv);
      test_assert(test_cfg.run_calls[6].name.starts_with(
          "parameterized test names (7th parameter, "));
      test_assert(test_cfg.run_calls[6].name.find("std::complex<double>") !=
                  std::string::npos);
      test_assert(test_cfg.run_calls[7].name.starts_with(
          "parameterized test names (custom_printable_type(42), "));
    }

    {
      // See https://github.com/boost-ext/ut/issues/581
      test_cfg = fake_cfg{};
      test("count asserts using operator|") = []() {
        test("MyTest") = [](auto fixture) {
          expect(fixture > 0);
          expect(false);
        } | std::vector{1, 2, 3, 4};
      };
      test_assert(test_cfg.assertion_calls.size() == 8);

      test_cfg = fake_cfg{};
      test("count asserts using for") = []() {
        for (auto fixture : std::vector{1, 2, 3, 4}) {
          test("MyTest" + std::to_string(fixture)) = [&fixture]() {
            expect(fixture > 0);
            expect(false);
          };
        }
      };
      test_assert(test_cfg.assertion_calls.size() == 8);
    }

    {
      test_cfg = fake_cfg{};

      using namespace ut::bdd;

      "scenario"_test = [] {
        given("I have...") = [] {
          when("I run...") = [] {
            then("I expect...") = [] { expect(1_u == 1u); };
            then("I expect...") = [] { expect(1u == 1_u); };
          };
        };
      };

      test_assert(5 == std::size(test_cfg.run_calls));
      test_assert("scenario"sv == test_cfg.run_calls[0].name);
      test_assert("given"sv == test_cfg.run_calls[1].type);
      test_assert("I have..."sv == test_cfg.run_calls[1].name);
      test_assert("when"sv == test_cfg.run_calls[2].type);
      test_assert("I run..."sv == test_cfg.run_calls[2].name);
      test_assert("then"sv == test_cfg.run_calls[3].type);
      test_assert("I expect..."sv == test_cfg.run_calls[3].name);
      test_assert("then"sv == test_cfg.run_calls[4].type);
      test_assert("I expect..."sv == test_cfg.run_calls[4].name);
      test_assert(2 == std::size(test_cfg.assertion_calls));
      test_assert(test_cfg.assertion_calls[0].result);
      test_assert("1 == 1" == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[1].result);
      test_assert("1 == 1" == test_cfg.assertion_calls[1].expr);
    }

    {
      test_cfg = fake_cfg{};

      using namespace ut::bdd;

      feature("feature...") = [] {
        scenario("scenario...") = [] {
          given("given...") = [] {};
          when("when...") = [] {};
          then("then...") = [] {};
        };
      };

      test_assert(5 == std::size(test_cfg.run_calls));
      test_assert("feature..."sv == test_cfg.run_calls[0].name);
      test_assert("feature"sv == test_cfg.run_calls[0].type);
      test_assert("scenario..."sv == test_cfg.run_calls[1].name);
      test_assert("scenario"sv == test_cfg.run_calls[1].type);
      test_assert("given..."sv == test_cfg.run_calls[2].name);
      test_assert("given"sv == test_cfg.run_calls[2].type);
      test_assert("when..."sv == test_cfg.run_calls[3].name);
      test_assert("when"sv == test_cfg.run_calls[3].type);
      test_assert("then..."sv == test_cfg.run_calls[4].name);
      test_assert("then"sv == test_cfg.run_calls[4].type);
    }

    {
      test_cfg = fake_cfg{};

      using namespace ut::spec;

      describe("describe") = [] { it("it") = [] { expect(1_u == 1u); }; };

      test_assert(2 == std::size(test_cfg.run_calls));
      test_assert("describe"sv == test_cfg.run_calls[0].name);
      test_assert("it"sv == test_cfg.run_calls[1].type);
      test_assert(test_cfg.assertion_calls[0].result);
      test_assert("1 == 1" == test_cfg.assertion_calls[0].expr);
    }

    {
      test_cfg = fake_cfg{};

      test("name") = [] {};

      test_assert(1 == std::size(test_cfg.run_calls));
      test_assert("name"sv == test_cfg.run_calls[0].name);
      test_assert("test"sv == test_cfg.run_calls[0].type);
    }

    {
      test_cfg = fake_cfg{};

      should("name") = [] {};

      test_assert(1 == std::size(test_cfg.run_calls));
      test_assert("name"sv == test_cfg.run_calls[0].name);
      test_assert("test"sv == test_cfg.run_calls[0].type);
    }

    {
      test_cfg = fake_cfg{};

      "should disambiguate operators"_test = [] {
        expect(1_i == ns::f());
        expect(2_i == f());
      };

      test_assert(1 == std::size(test_cfg.run_calls));
      test_assert("should disambiguate operators"sv ==
                  test_cfg.run_calls[0].name);
      test_assert(2 == std::size(test_cfg.assertion_calls));
      test_assert(test_cfg.assertion_calls[0].result);
      test_assert("1 == 1" == test_cfg.assertion_calls[0].expr);
      test_assert(test_cfg.assertion_calls[1].result);
      test_assert("2 == 2" == test_cfg.assertion_calls[1].expr);
    }

#if (__has_builtin(__builtin_FILE) and __has_builtin(__builtin_LINE))
    {
      using namespace ut::bdd;

      test_cfg = fake_cfg{};

      constexpr auto file = std::string_view{__FILE__};
      constexpr auto line = __LINE__;

      "test location"_test = [] {};
      test("test location") = [] {};
      should("test location") = [] {};
      given("test location") = [] {};
      when("test location") = [] {};
      then("test location") = [] {};

      test_assert(6 == std::size(test_cfg.run_calls));
      test_assert(file == test_cfg.run_calls[0].location.file_name());
      test_assert(file == test_cfg.run_calls[1].location.file_name());
      test_assert(file == test_cfg.run_calls[2].location.file_name());
      test_assert(file == test_cfg.run_calls[3].location.file_name());
      test_assert(file == test_cfg.run_calls[4].location.file_name());
      test_assert(file == test_cfg.run_calls[5].location.file_name());

      test_assert(line + 2 == test_cfg.run_calls[0].location.line());
      test_assert(line + 3 == test_cfg.run_calls[1].location.line());
      test_assert(line + 4 == test_cfg.run_calls[2].location.line());
      test_assert(line + 5 == test_cfg.run_calls[3].location.line());
      test_assert(line + 6 == test_cfg.run_calls[4].location.line());
      test_assert(line + 7 == test_cfg.run_calls[5].location.line());
    }
#endif
  }

  {
    using ut::expect;
    using ut::fatal;
    using namespace ut::literals;
    using namespace ut::operators::terse;
    using namespace std::literals::string_view_literals;

    auto& test_cfg = ut::cfg<ut::override>;

    test_cfg = fake_cfg{};

    // clang-format off
    42_i == 42;
    1 == 2_i;
    (0_i == 1 and 1_i > 2) or 3 <= 3_i;
    try {
    ("true"_b == false) >> fatal and 1_i > 0;
    } catch(const ut::events::fatal_assertion&) {}
    2 == 1_i;
    custom{42}%_t == custom{41};
    (3 >= 4_i) << "fatal";
    // clang-format on

    test_assert(7 == std::size(test_cfg.assertion_calls));
    test_assert(test_cfg.fatal_assertion_calls > 0);

    test_assert("42 == 42" == test_cfg.assertion_calls[0].expr);
    test_assert(test_cfg.assertion_calls[0].result);

    test_assert("1 == 2" == test_cfg.assertion_calls[1].expr);
    test_assert(not test_cfg.assertion_calls[1].result);

    test_assert("((0 == 1 and 1 > 2) or 3 <= 3)" ==
                test_cfg.assertion_calls[2].expr);
    test_assert(test_cfg.assertion_calls[2].result);

    test_assert("false" == test_cfg.assertion_calls[3].expr);
    test_assert(not test_cfg.assertion_calls[3].result);

    test_assert("2 == 1" == test_cfg.assertion_calls[4].expr);
    test_assert(not test_cfg.assertion_calls[4].result);

    test_assert("custom{42} == custom{41}" == test_cfg.assertion_calls[5].expr);
    test_assert(not test_cfg.assertion_calls[5].result);

    test_assert("3 >= 4" == test_cfg.assertion_calls[6].expr);
    test_assert(not test_cfg.assertion_calls[6].result);
    test_assert(2 == std::size(test_cfg.log_calls));
    test_assert('\n' == std::any_cast<char>(test_cfg.log_calls[0]));
    test_assert("fatal"sv == std::any_cast<const char*>(test_cfg.log_calls[1]));
  }

#if !defined(_MSC_VER) || _MSC_VER >= 1944
  {
    using namespace ut;
    auto& test_cfg = ut::cfg<ut::override>;
    test_cfg = fake_cfg{};

    bdd::gherkin::steps steps_int = [](auto& steps) {
      steps.feature("Calculator") = [&] {
        steps.scenario("*") = [&] {
          steps.given("I have calculator") = [&] {
            fake_calculator<int> calc{};
            steps.when("I enter {value}") = [&](int value) {
              calc.enter(value);
            };
            steps.when("I name it '{value}'") = [&](std::string value) {
              calc.name(std::move(value));
            };
            steps.when("I press add") = [&] { calc.add(); };
            steps.when("I press sub") = [&] { calc.sub(); };
            steps.then("I expect {value}") = [&](int result) {
              expect(that % calc.get() == result);
            };
            steps.then("it's name is '{value}'") = [&](std::string value) {
              expect(calc.name() == value);
            };
          };
        };
      };
    };

    // clang-format off
    "Calculator/int"_test = steps_int |
      R"(
        Feature: Calculator

          Scenario: Addition
            Given I have calculator
             When I enter 40
             When I enter 2
             When I press add
             Then I expect 42

          Scenario: Subtraction
            Given I have calculator
             When I enter 4
             When I enter 2
             When I press sub
             Then I expect 2

          Scenario: Give a calculator a name
            Given I have calculator
             When I name it 'My calculator'
             Then it's name is 'My calculator'
      )";
    // clang-format on

    test_assert(3 == std::size(test_cfg.assertion_calls));

    test_assert("42 == 42" == test_cfg.assertion_calls[0].expr);
    test_assert(test_cfg.assertion_calls[0].result);

    test_assert("2 == 2" == test_cfg.assertion_calls[1].expr);
    test_assert(test_cfg.assertion_calls[1].result);

    test_assert(test_cfg.assertion_calls[2].result);
  }

  {
    using namespace ut;
    auto& test_cfg = ut::cfg<ut::override>;
    test_cfg = fake_cfg{};

    bdd::gherkin::steps steps_double = [](auto& steps) {
      steps.feature("Calculator") = [&] {
        steps.scenario("*") = [&] {
          steps.given("I have calculator") = [&] {
            fake_calculator<double> calc{};
            steps.when("I enter {value}") = [&](double value) {
              calc.enter(value);
            };
            steps.when("I press add") = [&] { calc.add(); };
            steps.when("I press sub") = [&] { calc.sub(); };
            steps.then("I expect {value}") = [&](double result) {
              expect(that % calc.get() == result);
            };
          };
        };
      };
    };

    // clang-format off
    "Calculator/double"_test = steps_double |
      R"(
        Feature: Calculator

          Scenario: Addition
            Given I have calculator
             When I enter 40.0
             When I enter 2.0
             When I press add
             Then I expect 42.0

          Scenario: Subtraction
            Given I have calculator
             When I enter 4.
             When I enter 2.
             When I press sub
             Then I expect 2.
      )";
    // clang-format on

    test_assert(2 == std::size(test_cfg.assertion_calls));

    test_assert("42 == 42" == test_cfg.assertion_calls[0].expr);
    test_assert(test_cfg.assertion_calls[0].result);

    test_assert("2 == 2" == test_cfg.assertion_calls[1].expr);
    test_assert(test_cfg.assertion_calls[1].result);
  }
#endif

  return 0;
}
