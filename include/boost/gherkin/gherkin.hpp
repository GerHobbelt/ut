#pragma once

#include <boost/ut.hpp>

BOOST_UT_EXPORT

namespace boost::inline ext::ut::inline v2_0_1
  {

namespace bdd::gherkin
  {
  class steps
    {
    using step_t = std::string;
    using steps_t = void (*)(steps &);
    using gherkin_t = std::vector<step_t>;
    using call_step_t = utility::function<void(std::string const &)>;
    using call_steps_t = std::vector<std::pair<step_t, call_step_t>>;

    class step
      {
    public:
      template<class TPattern>
      step(steps & steps, TPattern const & pattern) : steps_{steps}, pattern_{pattern}
        {
        }

      ~step() { steps_.next(pattern_); }

      template<class TExpr>
      auto operator=(TExpr const & expr) -> void
        {
        for(auto const & [pattern, _]: steps_.call_steps())
          if(pattern_ == pattern)
            return;

        steps_.call_steps().emplace_back(
          pattern_,
          [expr, pattern = pattern_](auto const & _step)
          {
            [=]<class... TArgs>(type_traits::list<TArgs...>)
            {
              log << _step;
              auto i = 0u;
              auto const & ms = utility::match(pattern, _step);
              expr(lexical_cast<TArgs>(ms[i++])...);
            }(typename type_traits::function_traits<TExpr>::args{});
          }
        );
        }

    private:
      template<class T>
      static auto lexical_cast(std::string const & str)
        {
        T t{};
        std::istringstream iss{};
        iss.str(str);
        if constexpr(std::is_same_v<T, std::string>)
          t = iss.str();
        else
          iss >> t;
        return t;
        }

      steps & steps_;
      std::string pattern_{};
      };

  public:
    template<class TSteps>
    constexpr /*explicit(false)*/ steps(TSteps const & _steps) : steps_{_steps}
      {
      }

    template<class TGherkin>
    auto operator|(TGherkin const & gherkin)
      {
      gherkin_ = utility::split<std::string>(gherkin, '\n');
      for(auto & _step: gherkin_)
        _step.erase(0, _step.find_first_not_of(" \t"));

      return [this]
      {
        step_ = {};
        steps_(*this);
      };
      }

    auto feature(std::string const & pattern) { return step{*this, "Feature: " + pattern}; }

    auto scenario(std::string const & pattern) { return step{*this, "Scenario: " + pattern}; }

    auto given(std::string const & pattern) { return step{*this, "Given " + pattern}; }

    auto when(std::string const & pattern) { return step{*this, "When " + pattern}; }

    auto then(std::string const & pattern) { return step{*this, "Then " + pattern}; }

  private:
    template<class TPattern>
    auto next(TPattern const & pattern) -> void
      {
      auto const is_scenario = [&pattern](auto const & _step)
      {
        constexpr auto scenario = "Scenario";
        return pattern.find(scenario) == std::string::npos and _step.find(scenario) != std::string::npos;
      };

      auto const call_steps = [this, is_scenario](auto const & _step, auto const i)
      {
        for(auto const & [name, call]: call_steps_)
          {
          if(is_scenario(_step))
            break;

          if(utility::is_match(_step, name) or not std::empty(utility::match(name, _step)))
            {
            step_ = i;
            call(_step);
            }
          }
      };

      decltype(step_) i{};
      for(auto const & _step: gherkin_)
        if(i++ == step_)
          call_steps(_step, i);
      }

    auto call_steps() -> call_steps_t & { return call_steps_; }

    steps_t steps_{};
    gherkin_t gherkin_{};
    call_steps_t call_steps_{};
    decltype(sizeof("")) step_{};
    };
  }  // namespace gherkin
  }  // namespace boost::inline ext::ut::inline v2_0_1
