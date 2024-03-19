#pragma once

#include <boost/ut.hpp>

BOOST_UT_EXPORT

namespace boost::inline ext::ut::inline v2_0_1
  {

template<class TPrinter = printer>
class reporter_junit
  {
  template<typename Key, typename T>
  using map = std::unordered_map<Key, T>;
  using clock_ref = std::chrono::high_resolution_clock;
  using timePoint = std::chrono::time_point<clock_ref>;
  using timeDiff = std::chrono::milliseconds;
  enum class ReportType
    {
    CONSOLE,
    JUNIT
    } report_type_;
  static constexpr ReportType CONSOLE = ReportType::CONSOLE;
  static constexpr ReportType JUNIT = ReportType::JUNIT;

  struct test_result
    {
    test_result * parent = nullptr;
    std::string class_name;
    std::string suite_name;
    std::string test_name;
    std::string status = "STARTED";
    timePoint run_start = clock_ref::now();
    timePoint run_stop = clock_ref::now();
    std::size_t n_tests = 0LU;
    std::size_t assertions = 0LU;
    std::size_t passed = 0LU;
    std::size_t skipped = 0LU;
    std::size_t fails = 0LU;
    std::string report_string{};
    std::unique_ptr<map<std::string, test_result>> nested_tests = std::make_unique<map<std::string, test_result>>();
    };

  colors color_{};
  map<std::string, test_result> results_;
  std::string active_suite_{"global"};
  test_result * active_scope_ = &results_[active_suite_];
  std::stack<std::string> active_test_{};

  std::streambuf * cout_save = std::cout.rdbuf();
  std::ostream lcout_;
  TPrinter printer_;
  std::stringstream ss_out_{};

  void reset_printer()
    {
    ss_out_.str("");
    ss_out_.clear();
    }

  void check_for_scope(std::string_view test_name)
    {
    auto & cfg_{detail::config()};
    std::string const str_name(test_name);
    active_test_.push(str_name);
    auto const [iter, inserted] = active_scope_->nested_tests->try_emplace(
      str_name, test_result{active_scope_, cfg_.executable_name, active_suite_, str_name}
    );
    active_scope_ = &active_scope_->nested_tests->at(str_name);
    if(active_test_.size() == 1)
      reset_printer();
    active_scope_->run_start = clock_ref::now();
    if(!inserted)
      std::cout << "WARNING test '" << str_name << "' for test suite '" << active_suite_ << "' already present\n";
    }

  void pop_scope(std::string_view test_name_sv)
    {
    std::string const test_name(test_name_sv);
    active_scope_->run_stop = clock_ref::now();
    if(active_scope_->skipped)
      active_scope_->status = "SKIPPED";
    else
      active_scope_->status = active_scope_->fails > 0 ? "FAILED" : "PASSED";
    active_scope_->assertions = active_scope_->assertions + active_scope_->fails;

    if(active_test_.top() == test_name)
      {
      active_test_.pop();
      auto old_scope = active_scope_;
      if(active_scope_->parent != nullptr)
        active_scope_ = active_scope_->parent;
      else
        active_scope_ = &results_[std::string{"global"}];
      active_scope_->n_tests += old_scope->n_tests + 1LU;
      active_scope_->assertions += old_scope->assertions;
      active_scope_->passed += old_scope->passed;
      active_scope_->skipped += old_scope->skipped;
      active_scope_->fails += old_scope->fails;
      return;
      }
    std::stringstream ss("runner returned from test w/o signaling: ");
    ss << "not popping because '" << active_test_.top() << "' differs from '" << test_name << "'" << std::endl;
#if defined(__cpp_exceptions)
    throw std::logic_error(ss.str());
#else
    std::abort();
#endif
    }

public:
  constexpr auto operator=(TPrinter printer) { printer_ = static_cast<TPrinter &&>(printer); }

  reporter_junit() : lcout_(std::cout.rdbuf())
    {
    auto & cfg_{detail::config()};
    cfg_.parse(cfg_.largc, cfg_.largv);

    if(cfg_.show_reporters)
      {
      std::cout << "available reporter:\n";
      std::cout << "  console (default)\n";
      std::cout << "  junit" << std::endl;
      std::exit(0);
      }
    if(cfg_.use_reporter.starts_with("junit"))
      report_type_ = JUNIT;
    else
      report_type_ = CONSOLE;
    if(!cfg_.use_colour.starts_with("yes"))
      color_ = {"", "", "", ""};
    if(!cfg_.show_tests && !cfg_.show_test_names)
      std::cout.rdbuf(ss_out_.rdbuf());
    }

  ~reporter_junit() { std::cout.rdbuf(cout_save); }

  auto on(events::suite_begin suite) -> void
    {
    while(active_test_.size() > 0)
      pop_scope(active_test_.top());
    active_suite_ = suite.name;
    active_scope_ = &results_[active_suite_];
    }

  auto on(events::suite_end) -> void
    {
    while(active_test_.size() > 0)
      pop_scope(active_test_.top());
    active_suite_ = "global";
    active_scope_ = &results_[active_suite_];
    }

  auto on(events::test_begin test_event) -> void
    {  // starts outermost test
    check_for_scope(test_event.name);

    if(report_type_ == CONSOLE)
      {
      ss_out_ << "\n";
      ss_out_ << std::string(2 * active_test_.size() - 2, ' ');
      ss_out_ << "Running test \"" << test_event.name << "\"... ";
      }
    }

  auto on(events::test_end test_event) -> void
    {
    if(active_scope_->fails > 0)
      {
      reset_printer();
      }
    else
      {
      active_scope_->report_string = ss_out_.str();
      active_scope_->passed += 1LU;
      if(report_type_ == CONSOLE)
        {
        auto & cfg_{detail::config()};
        if(cfg_.show_successful_tests)
          {
          if(!active_scope_->nested_tests->empty())
            {
            ss_out_ << "\n";
            ss_out_ << std::string(2 * active_test_.size() - 2, ' ');
            ss_out_ << "Running test \"" << test_event.name << "\" - ";
            }
          ss_out_ << color_.pass << "PASSED" << color_.none;
          print_duration(ss_out_);
          lcout_ << ss_out_.str();
          reset_printer();
          }
        }
      }

    pop_scope(test_event.name);
    }

  auto on(events::test_run test_event) -> void
    {  // starts nested test
    on(events::test_begin{.type = test_event.type, .name = test_event.name});
    }

  auto on(events::test_finish test_event) -> void
    {  // finishes nested test
    on(events::test_end{.type = test_event.type, .name = test_event.name});
    }

  auto on(events::test_skip test_event) -> void
    {
    ss_out_.clear();
    if(!active_scope_->nested_tests->contains(std::string(test_event.name)))
      {
      check_for_scope(test_event.name);
      active_scope_->status = "SKIPPED";
      active_scope_->skipped += 1;
      if(report_type_ == CONSOLE)
        {
        lcout_ << '\n' << std::string(2 * active_test_.size() - 2, ' ');
        lcout_ << "Running \"" << test_event.name << "\"... ";
        lcout_ << color_.skip << "SKIPPED" << color_.none;
        }
      reset_printer();
      pop_scope(test_event.name);
      }
    }

  template<class TMsg>
  auto on(events::log<TMsg> log) -> void
    {
    ss_out_ << log.msg;
    if(report_type_ == CONSOLE)
      lcout_ << log.msg;
    }

  auto on(events::exception exception) -> void
    {
    active_scope_->fails++;
    if(!active_test_.empty())
      {
      active_scope_->report_string += color_.fail;
      active_scope_->report_string += "Unexpected exception with message:\n";
      active_scope_->report_string += exception.what();
      active_scope_->report_string += color_.none;
      }
    if(report_type_ == CONSOLE)
      {
      lcout_ << std::string(2 * active_test_.size() - 2, ' ');
      lcout_ << "Running test \"" << active_test_.top() << "\"... ";
      lcout_ << color_.fail << "FAILED" << color_.none;
      print_duration(lcout_);
      lcout_ << '\n';
      lcout_ << active_scope_->report_string << '\n';
      }
    auto & cfg_{detail::config()};
    if(cfg_.abort_early || active_scope_->fails >= cfg_.abort_after_n_failures)
      {
      std::cerr << "early abort for test : " << active_test_.top() << "after ";
      std::cerr << active_scope_->fails << " failures total." << std::endl;
      std::exit(-1);
      }
    }

  template<class TExpr>
  auto on(events::assertion_pass<TExpr>) -> void
    {
    active_scope_->assertions++;
    }

  template<class TExpr>
  auto on(events::assertion_fail<TExpr> assertion) -> void
    {
    TPrinter ss{};
    ss << ss_out_.str();
    if(report_type_ == CONSOLE)
      {
      ss << color_.fail << "FAILED\n" << color_.none;
      print_duration(ss);
      }
    ss << "in: " << assertion.location.file_name() << ':' << assertion.location.line();
    ss << color_.fail << " - test condition: ";
    ss << " [" << std::boolalpha << assertion.expr;
    ss << color_.fail << ']' << color_.none;
    active_scope_->report_string += ss.str();
    active_scope_->fails++;
    reset_printer();
    if(report_type_ == CONSOLE)
      lcout_ << active_scope_->report_string << "\n\n";
    auto & cfg_{detail::config()};
    if(cfg_.abort_early || active_scope_->fails >= cfg_.abort_after_n_failures)
      {
      std::cerr << "early abort for test : " << active_test_.top() << "after ";
      std::cerr << active_scope_->fails << " failures total." << std::endl;
      std::exit(-1);
      }
    }

  auto on(events::fatal_assertion) -> void { active_scope_->fails++; }

  auto on(events::summary) -> void
    {
    std::cout.flush();
    std::cout.rdbuf(cout_save);
    std::ofstream maybe_of;
    auto & cfg_{detail::config()};

    if(cfg_.output_filename != "")
      maybe_of = std::ofstream(cfg_.output_filename);

    if(report_type_ == JUNIT)
      {
      print_junit_summary(cfg_.output_filename != "" ? maybe_of : std::cout);
      return;
      }
    print_console_summary(
      cfg_.output_filename != "" ? maybe_of : std::cout, cfg_.output_filename != "" ? maybe_of : std::cerr
    );
    }

protected:
  void print_duration(auto & printer) const noexcept
    {
    auto & cfg_{detail::config()};
    if(cfg_.show_duration)
      {
      std::int64_t time_ms
        = std::chrono::duration_cast<std::chrono::milliseconds>(active_scope_->run_stop - active_scope_->run_start)
            .count();
      // rounded to nearest ms
      double time_s = static_cast<double>(time_ms) / 1000.0;
      printer << " after " << time_s << " seconds";
      }
    }

  void print_console_summary(std::ostream & out_stream, std::ostream & err_stream)
    {
    for(auto const & [suite_name, suite_result]: results_)
      {
      if(suite_result.fails)
        {
        err_stream << "\n========================================================"
                      "=======================\n"
                   << "Suite " << suite_name  //
                   << "tests:   " << (suite_result.n_tests) << " | " << color_.fail << suite_result.fails << " failed"
                   << color_.none << '\n'
                   << "asserts: " << (suite_result.assertions) << " | " << suite_result.passed << " passed" << " | "
                   << color_.fail << suite_result.fails << " failed" << color_.none << '\n';
        std::cerr << std::endl;
        }
      else
        {
        out_stream << color_.pass << "Suite '" << suite_name << "': all tests passed" << color_.none << " ("
                   << suite_result.assertions << " asserts in " << suite_result.n_tests << " tests)\n";

        if(suite_result.skipped)
          std::cout << suite_result.skipped << " tests skipped\n";

        std::cout.flush();
        }
      }
    }

  void print_junit_summary(std::ostream & stream)
    {
    // aggregate results
    size_t n_tests = 0, n_fails = 0;
    double total_time = 0.0;
    auto suite_time = [](auto const & suite_result)
    {
      std::int64_t time_ms
        = std::chrono::duration_cast<std::chrono::milliseconds>(suite_result.run_stop - suite_result.run_start).count();
      return static_cast<double>(time_ms) / 1000.0;
    };
    for(auto const & [suite_name, suite_result]: results_)
      {
      n_tests += suite_result.assertions;
      n_fails += suite_result.fails;
      total_time += suite_time(suite_result);
      }
    auto & cfg_{detail::config()};
    // mock junit output:
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    stream << "<testsuites";
    stream << " name=\"all\"";
    stream << " tests=\"" << n_tests << '\"';
    stream << " failures=\"" << n_fails << '\"';
    stream << " time=\"" << total_time << '\"';
    stream << ">\n";

    for(auto const & [suite_name, suite_result]: results_)
      {
      stream << "<testsuite";
      stream << " classname=\"" << cfg_.executable_name << '\"';
      stream << " name=\"" << suite_name << '\"';
      stream << " tests=\"" << suite_result.assertions << '\"';
      stream << " errors=\"" << suite_result.fails << '\"';
      stream << " failures=\"" << suite_result.fails << '\"';
      stream << " skipped=\"" << suite_result.skipped << '\"';
      stream << " time=\"" << suite_time(suite_result) << '\"';
      stream << " version=\"" << BOOST_UT_VERSION << "\">\n";
      print_result(stream, suite_name, " ", suite_result);
      stream << "</testsuite>\n";
      stream.flush();
      }
    stream << "</testsuites>";
    }

  void
    print_result(std::ostream & stream, std::string const & suite_name, std::string indent, test_result const & parent)
    {
    for(auto const & [name, result]: *parent.nested_tests)
      {
      stream << indent;
      stream << "<testcase classname=\"" << result.suite_name << '\"';
      stream << " name=\"" << name << '\"';
      stream << " tests=\"" << result.assertions << '\"';
      stream << " errors=\"" << result.fails << '\"';
      stream << " failures=\"" << result.fails << '\"';
      stream << " skipped=\"" << result.skipped << '\"';
      std::int64_t time_ms
        = std::chrono::duration_cast<std::chrono::milliseconds>(result.run_stop - result.run_start).count();
      stream << " time=\"" << (static_cast<double>(time_ms) / 1000.0) << "\"";
      stream << " status=\"" << result.status << '\"';
      if(result.report_string.empty() && result.nested_tests->empty())
        {
        stream << " />\n";
        }
      else if(!result.nested_tests->empty())
        {
        stream << " />\n";
        print_result(stream, suite_name, indent + "  ", result);
        stream << indent << "</testcase>\n";
        }
      else if(!result.report_string.empty())
        {
        stream << ">\n";
        stream << indent << indent << "<system-out>\n";
        stream << result.report_string << "\n";
        stream << indent << indent << "</system-out>\n";
        stream << indent << "</testcase>\n";
        }
      }
    }
  };
  }  // namespace boost::inline ext::ut::inline v2_0_1
