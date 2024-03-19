#include <boost/ut.hpp>
#include <boost/gherkin/gherkin.hpp>
#include <boost/reporter_junit.hpp>

namespace boost::inline ext::ut::inline v2_0_1::detail
  {
reflection::source_location cfg::location{};
bool cfg::wip{};
#if defined(_MSC_VER)
int cfg::largc = __argc;
char const ** cfg::largv = const_cast<char const **>(__argv);
#else
int cfg::largc = 0;
char const ** cfg::largv = nullptr;
#endif

auto config() -> cfg &
  {
  static cfg _ [[clang::no_destroy]]
  ;
  return _;
  }

cfg::cfg()
  {
  options = std::vector<option>{
    // clang-format off
  // <short long option name>, <option arg>, <ref to cfg>, <description>
  {"-? -h --help", "", std::ref(show_help), "display usage information"},
  {"-l --list-tests", "", std::ref(show_tests), "list all/matching test cases"},
  {"-t, --list-tags", "", std::ref(list_tags), "list all/matching tags"},
  {"-s, --success", "", std::ref(show_successful_tests), "include successful tests in output"},
  {"-o, --out", "<filename>", std::ref(output_filename), "output filename"},
  {"-r, --reporter", "<name>", std::ref(use_reporter), "reporter to use (defaults to console)"},
  {"-n, --name", "<name>", std::ref(suite_name), "suite name"},
  {"-a, --abort", "", std::ref(abort_early), "abort at first failure"},
  {"-x, --abortx", "<no. failures>", std::ref(abort_after_n_failures), "abort after x failures"},
  {"-d, --durations", "", std::ref(show_duration), "show test durations"},
  {"-D, --min-duration", "<seconds>", std::ref(show_min_duration), "show test durations for [...]"},
  {"-f, --input-file", "<filename>", std::ref(input_filename), "load test names to run from a file"},
  {"--list-test-names-only", "", std::ref(show_test_names), "list all/matching test cases names only"},
  {"--list-reporters", "", std::ref(show_reporters), "list all reporters"},
  {"--order <decl|lex|rand>", "", std::ref(sort_order), "test case order (defaults to decl)"},
  {"--rng-seed", "<'time'|number>", std::ref(rnd_seed), "set a specific seed for random numbers"},
  {"--use-colour", "<yes|no>", std::ref(use_colour), "should output be colourised"},
  {"--libidentify", "", std::ref(show_lib_identity), "report name and version according to libidentify standard"},
  {"--wait-for-keypress", "<never|start|exit|both>", std::ref(wait_for_keypress), "waits for a keypress before exiting"}
    // clang-format on
  };
  }

auto cfg::find_arg(std::string_view arg) -> std::optional<cfg::option>
  {
  for(auto const & option: cfg::options)
    if(std::get<0>(option).find(arg) != std::string::npos)
      return option;

  return std::nullopt;
  }

void cfg::print_usage()
  {
  std::size_t opt_width = 30;
  std::cout << cfg::executable_name << " [<test name|pattern|tags> ... ] options\n\nwith options:\n";
  for(auto const & [cmd, arg, val, description]: cfg::options)
    {
    std::string s = cmd;
    s.append(" ");
    s.append(arg);
    // pad fixed column width
    auto const pad_by = (s.size() <= opt_width) ? opt_width - s.size() : 0;
    s.insert(s.end(), pad_by, ' ');
    std::cout << "  " << s << description << std::endl;
    }
  }

void cfg::print_identity()
  {
  // according to: https://github.com/janwilmans/LibIdentify
  std::cout << "description:    A UT / μt test executable\n";
  std::cout << "category:       testframework\n";
  std::cout << "framework:      UT: C++20 μ(micro)/Unit Testing Framework\n";
  std::cout << "version:        " << BOOST_UT_VERSION << std::endl;
  }

void cfg::parse(int argc, char const * argv[])
  {
  std::size_t const n_args = static_cast<std::size_t>(argc);
  if(n_args > 0 && argv != nullptr)
    {
    cfg::largc = argc;
    cfg::largv = argv;
    executable_name = argv[0];
    }
  query_pattern = "";
  bool found_first_option = false;
  for(auto i = 1U; i < n_args; i++)
    {
    std::string cmd(argv[i]);
    auto cmd_option = find_arg(cmd);
    if(!cmd_option.has_value())
      {
      if(found_first_option)
        {
        std::cerr << "unknown option: '" << argv[i] << "' run:" << std::endl;
        std::cerr << "'" << argv[0] << " --help'" << std::endl;
        std::cerr << "for additional help" << std::endl;
        std::exit(-1);
        }
      else
        {
        if(i > 1U)
          query_pattern.append(" ");
        query_pattern.append(argv[i]);
        }
      continue;
      }
    found_first_option = true;
    auto var = std::get<value_ref>(*cmd_option);
    bool const has_option_arg = !std::get<1>(*cmd_option).empty();
    if(!has_option_arg && std::holds_alternative<std::reference_wrapper<bool>>(var))
      {
      std::get<std::reference_wrapper<bool>>(var).get() = true;
      continue;
      }
    if((i + 1) >= n_args)
      {
      std::cerr << "missing argument for option " << argv[i] << std::endl;
      std::exit(-1);
      }
    i += 1;  // skip to next argv for parsing
    if(std::holds_alternative<std::reference_wrapper<std::size_t>>(var))
      {
      // parse size argument
      std::size_t last;
      std::string argument(argv[i]);
      std::size_t val = std::stoull(argument, &last);
      if(last != argument.length())
        {
        std::cerr << "cannot parse option of " << argv[i - 1] << " " << argv[i] << std::endl;
        std::exit(-1);
        }
      std::get<std::reference_wrapper<std::size_t>>(var).get() = val;
      }
    if(std::holds_alternative<std::reference_wrapper<std::string>>(var))
      {
      // parse string argument
      std::get<std::reference_wrapper<std::string>>(var).get() = argv[i];
      continue;
      }
    }

  if(show_help)
    {
    print_usage();
    std::exit(0);
    }

  if(show_lib_identity)
    {
    print_identity();
    std::exit(0);
    }

  if(!query_pattern.empty())
    {  // simple glob-like search
    query_regex_pattern = "";
    for(char const c: query_pattern)
      if(c == '!')
        invert_query_pattern = true;
      else if(c == '*')
        query_regex_pattern += ".*";
      else if(c == '?')
        query_regex_pattern += '.';
      else if(c == '.')
        query_regex_pattern += "\\.";
      else if(c == '\\')
        query_regex_pattern += "\\\\";
      else
        query_regex_pattern += c;
    }
  }
  }  // namespace boost::inline ext::ut::inline v2_0_1::detail

namespace boost::inline ext::ut::inline v2_0_1
  {
  }
