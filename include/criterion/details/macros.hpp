#pragma once
#include <criterion/details/benchmark.hpp>
#include <criterion/details/benchmark_config.hpp>
#include <criterion/details/csv_writer.hpp>
#include <chrono>
#include <functional>
#include <string.h>
#include <unordered_map>
#include <string>

namespace criterion {

struct benchmark_registration_helper_struct {
  static std::vector<benchmark_config> &
  registered_benchmarks() {
    static std::vector<benchmark_config> v;
    return v;
  }

  static void register_benchmark(const benchmark_config& config) {
    registered_benchmarks().push_back(config);
  }

  static void execute_registered_benchmarks() {
    for (const auto& config : registered_benchmarks()) {
      benchmark{config}.run();
    }
  }
};

}

#define SETUP_BENCHMARK(...)                                                   \
  __VA_ARGS__                                                                  \
  __benchmark_start_timestamp =                                                \
      std::chrono::steady_clock::now(); // updated benchmark start timestamp

#define TEARDOWN_BENCHMARK(...)                                                \
  __benchmark_teardown_timestamp = std::chrono::steady_clock::now();           \
  __VA_ARGS__

namespace criterion {

struct benchmark_template_registration_helper_struct {
  static std::unordered_multimap<std::string, benchmark_config> & registered_benchmark_templates() {
    static std::unordered_multimap<std::string, benchmark_config> m;
    return m;
  }

  static void register_benchmark_template(const benchmark_config& config) {
    registered_benchmark_templates().insert({config.name, config});
  }

  template <class ArgTuple>
  static void execute_registered_benchmark_template(const std::string& template_name, const std::string& instance_name, ArgTuple& arg_tuple) {
    for (auto& [k, v] : registered_benchmark_templates()) {
      if (k == template_name) {
        benchmark_registration_helper_struct::register_benchmark(
          benchmark_config{ 
            .name = template_name, 
            .fn = v.fn,
            .parameterized_instance_name = instance_name,
            .parameters = (void *)(&arg_tuple)
          }
        );
      }
    }
  }
};

}

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

#define BENCHMARK_WITHOUT_PARAMETERS(Name) \
  typedef std::tuple<> CONCAT(Name, BenchmarkParameters); \
  namespace detail {                                                           \
  /* forward declare the benchmark function that we define later */            \
  template <class T = CONCAT(Name, BenchmarkParameters)> \
  struct CONCAT(__benchmark_function_wrapper__, __LINE__) {                    \
    static void CONCAT(_registered_fun_, __LINE__)(                            \
        std::chrono::steady_clock::time_point &,                               \
        std::optional<std::chrono::steady_clock::time_point> &, \
        void *);       \
  };                                                                           \
                                                                               \
  namespace /* ensure internal linkage for struct */                           \
  {                                                                            \
  /* helper struct for static registration in ctor */                          \
  struct CONCAT(_register_struct_, __LINE__) {                                 \
    CONCAT(_register_struct_, __LINE__)() { /* called once before main */      \
      criterion::benchmark_template_registration_helper_struct::register_benchmark_template(criterion::benchmark_config{                            \
          .name = #Name,                                                        \
          .fn = CONCAT(__benchmark_function_wrapper__,                         \
                       __LINE__)<CONCAT(Name, BenchmarkParameters)>::CONCAT(_registered_fun_, __LINE__)});        \
    }                                                                          \
  } CONCAT(_register_struct_instance_, __LINE__);                              \
  }                                                                            \
  \
  namespace /* ensure internal linkage for struct */                           \
  {                                                                            \
  static CONCAT(Name, BenchmarkParameters) CONCAT(CONCAT(Name, _benchmark_template_parameters), __LINE__) = {}; \
  /* helper struct for static registration in ctor */                          \
  struct CONCAT(_instantiation_struct_, __LINE__) {                                 \
    CONCAT(_instantiation_struct_, __LINE__)() { /* called once before main */      \
      criterion::benchmark_template_registration_helper_struct::execute_registered_benchmark_template<CONCAT(Name, BenchmarkParameters)>(#Name, "", CONCAT(CONCAT(Name, _benchmark_template_parameters), __LINE__)); \
    }                                                                          \
  } CONCAT(_instantiation_struct_instance_, __LINE__);                              \
  } \
  }                                                                            \
                                                                               \
  /* now actually defined to allow BENCHMARK("name") { ... } syntax */         \
  template <class T> \
  void detail::CONCAT(__benchmark_function_wrapper__, __LINE__)<T>::CONCAT(       \
      _registered_fun_, __LINE__)(                                             \
      [[maybe_unused]] std::chrono::steady_clock::time_point &                 \
          __benchmark_start_timestamp,                                         \
      [[maybe_unused]] std::optional<std::chrono::steady_clock::time_point> &  \
          __benchmark_teardown_timestamp, \
      [[maybe_unused]] void * __benchmark_parameters)


#define BENCHMARK_WITH_PARAMETERS(Name, ...) \
  typedef std::tuple<__VA_ARGS__> CONCAT(Name, BenchmarkParameters); \
  namespace detail {                                                           \
  /* forward declare the benchmark function that we define later */            \
  template <class T = CONCAT(Name, BenchmarkParameters)> \
  struct CONCAT(__benchmark_function_wrapper__, __LINE__) {                    \
    static void CONCAT(_registered_fun_, __LINE__)(                            \
        std::chrono::steady_clock::time_point &,                               \
        std::optional<std::chrono::steady_clock::time_point> &, \
        void *);       \
  };                                                                           \
                                                                               \
  namespace /* ensure internal linkage for struct */                           \
  {                                                                            \
  /* helper struct for static registration in ctor */                          \
  struct CONCAT(_register_struct_, __LINE__) {                                 \
    CONCAT(_register_struct_, __LINE__)() { /* called once before main */      \
      criterion::benchmark_template_registration_helper_struct::register_benchmark_template(criterion::benchmark_config{                            \
          .name = #Name,                                                        \
          .fn = CONCAT(__benchmark_function_wrapper__,                         \
                       __LINE__)<CONCAT(Name, BenchmarkParameters)>::CONCAT(_registered_fun_, __LINE__)});        \
    }                                                                          \
  } CONCAT(_register_struct_instance_, __LINE__);                              \
  }                                                                            \
  }                                                                            \
                                                                               \
  /* now actually defined to allow BENCHMARK("name") { ... } syntax */         \
  template <class T> \
  void detail::CONCAT(__benchmark_function_wrapper__, __LINE__)<T>::CONCAT(       \
      _registered_fun_, __LINE__)(                                             \
      [[maybe_unused]] std::chrono::steady_clock::time_point &                 \
          __benchmark_start_timestamp,                                         \
      [[maybe_unused]] std::optional<std::chrono::steady_clock::time_point> &  \
          __benchmark_teardown_timestamp, \
      [[maybe_unused]] void * __benchmark_parameters)

#define BENCHMARK_ARGUMENTS_TUPLE \
  *((T *)__benchmark_parameters)

#define BENCHMARK_ARGUMENTS(index) \
  std::get<index>(*((T *)__benchmark_parameters));

#define REGISTER_BENCHMARK(TemplateName, InstanceName, ...) \
  \
  namespace /* ensure internal linkage for struct */                           \
  {                                                                            \
  static CONCAT(TemplateName, BenchmarkParameters) CONCAT(CONCAT(TemplateName, _benchmark_template_parameters), __LINE__) = {__VA_ARGS__}; \
  /* helper struct for static registration in ctor */                          \
  struct CONCAT(_instantiation_struct_, __LINE__) {                                 \
    CONCAT(_instantiation_struct_, __LINE__)() { /* called once before main */      \
      criterion::benchmark_template_registration_helper_struct::execute_registered_benchmark_template<CONCAT(TemplateName, BenchmarkParameters)>(#TemplateName, InstanceName, CONCAT(CONCAT(TemplateName, _benchmark_template_parameters), __LINE__)); \
    }                                                                          \
  } CONCAT(_instantiation_struct_instance_, __LINE__);                              \
  }

#define BENCHMARK_1(Name) \
  BENCHMARK_WITHOUT_PARAMETERS(Name)

#define BENCHMARK_2(Name, A) BENCHMARK_WITH_PARAMETERS(Name, A)
#define BENCHMARK_3(Name, A, B) BENCHMARK_WITH_PARAMETERS(Name, A, B)
#define BENCHMARK_4(Name, A, B, C) BENCHMARK_WITH_PARAMETERS(Name, A, B, C)
#define BENCHMARK_5(Name, A, B, C, D) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D)
#define BENCHMARK_6(Name, A, B, C, D, E) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F)
#define BENCHMARK_7(Name, A, B, C, D, E, F) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F)
#define BENCHMARK_8(Name, A, B, C, D, E, F, G) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G)
#define BENCHMARK_9(Name, A, B, C, D, E, F, G, H) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H)
#define BENCHMARK_10(Name, A, B, C, D, E, F, G, H, I) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I)
#define BENCHMARK_11(Name, A, B, C, D, E, F, G, H, I, J) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J)
#define BENCHMARK_12(Name, A, B, C, D, E, F, G, H, I, J, K) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K)
#define BENCHMARK_13(Name, A, B, C, D, E, F, G, H, I, J, K, L) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K, L)
#define BENCHMARK_14(Name, A, B, C, D, E, F, G, H, I, J, K, L, M) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K, L, M)
#define BENCHMARK_15(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N)
#define BENCHMARK_16(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O)
#define BENCHMARK_17(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P)
#define BENCHMARK_18(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q)
#define BENCHMARK_19(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R)
#define BENCHMARK_20(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S)
#define BENCHMARK_21(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T)
#define BENCHMARK_22(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U)
#define BENCHMARK_23(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V)
#define BENCHMARK_24(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W)
#define BENCHMARK_25(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X)
#define BENCHMARK_26(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y)
#define BENCHMARK_27(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z) BENCHMARK_WITH_PARAMETERS(Name, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z)

// The interim macro that simply strips the excess and ends up with the required macro
#define BENCHMARK_X(Name,x,A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,FUNC, ...) FUNC

// BENCHMARK macro supports no more than 26 parameters
// The macro that the programmer uses 
#define BENCHMARK(...) \
  BENCHMARK_X(,##__VA_ARGS__,\
    BENCHMARK_27(__VA_ARGS__),\
    BENCHMARK_26(__VA_ARGS__),\
    BENCHMARK_25(__VA_ARGS__),\
    BENCHMARK_24(__VA_ARGS__),\
    BENCHMARK_23(__VA_ARGS__),\
    BENCHMARK_22(__VA_ARGS__),\
    BENCHMARK_21(__VA_ARGS__),\
    BENCHMARK_20(__VA_ARGS__),\
    BENCHMARK_19(__VA_ARGS__),\
    BENCHMARK_18(__VA_ARGS__),\
    BENCHMARK_17(__VA_ARGS__),\
    BENCHMARK_16(__VA_ARGS__),\
    BENCHMARK_15(__VA_ARGS__),\
    BENCHMARK_14(__VA_ARGS__),\
    BENCHMARK_13(__VA_ARGS__),\
    BENCHMARK_12(__VA_ARGS__),\
    BENCHMARK_11(__VA_ARGS__),\
    BENCHMARK_10(__VA_ARGS__),\
    BENCHMARK_9(__VA_ARGS__),\
    BENCHMARK_8(__VA_ARGS__),\
    BENCHMARK_7(__VA_ARGS__),\
    BENCHMARK_6(__VA_ARGS__),\
    BENCHMARK_5(__VA_ARGS__),\
    BENCHMARK_4(__VA_ARGS__),\
    BENCHMARK_3(__VA_ARGS__),\
    BENCHMARK_2(__VA_ARGS__),\
    BENCHMARK_1(__VA_ARGS__) \
    )

static inline void signal_handler(int signal) {
  indicators::show_console_cursor(true);
  std::cout << termcolor::reset;
  exit(signal);
}

#define CRITERION_BENCHMARK_MAIN                                                         \
  int main() {                                                                 \
    std::signal(SIGTERM, signal_handler);                                      \
    std::signal(SIGSEGV, signal_handler);                                      \
    std::signal(SIGINT, signal_handler);                                       \
    std::signal(SIGILL, signal_handler);                                       \
    std::signal(SIGABRT, signal_handler);                                      \
    std::signal(SIGFPE, signal_handler);                                       \
    criterion::benchmark_registration_helper_struct::execute_registered_benchmarks();                                           \
    criterion::csv_writer::write_results("results.csv", criterion::benchmark::results); \
  }