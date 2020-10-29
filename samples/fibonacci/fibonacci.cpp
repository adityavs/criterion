#include <criterion/criterion.hpp>

uint64_t Fibonacci(uint64_t n) {
  if (n <= 1) 
    return 1;
  else
    return Fibonacci(n - 1) + Fibonacci(n - 2);
}

BENCHMARK_TEMPLATE(Fibonacci, uint64_t)
{
  SETUP_BENCHMARK(
    const auto args = BENCHMARK_ARGUMENTS;
    const auto input = std::get<0>(args);
  )

  [[maybe_unused]] const auto result = Fibonacci(input);
}

RUN_BENCHMARK_TEMPLATE(Fibonacci, "/19", 19)
RUN_BENCHMARK_TEMPLATE(Fibonacci, "/20", 20)
RUN_BENCHMARK_TEMPLATE(Fibonacci, "/21", 21)

// Example output:
//
// Fibonacci/19
//    μ = 20.9us ± 0.0433% [N = 390030]
// Fibonacci/20
//    μ = 33.8us ± 0.0355% [N = 295370]
// Fibonacci/21
//    μ = 54.6us ± 0.0304% [N = 182650]