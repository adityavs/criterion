#pragma once
#include <criterion/details/benchmark.hpp>
#include <criterion/details/benchmark_result.hpp>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>

namespace criterion {

class csv_writer {
public:
  static bool write_results(const std::string &filename,
                            const std::unordered_map<std::string, benchmark_result> &results) {
    bool result{false};
    std::ofstream os(filename);
    if (os.is_open()) {
      os << "name,warmup_runs,iterations,mean_execution_time,fastest_execution_time,slowest_"
            "execution_time,lowest_rsd_execution_time,lowest_rsd_percentage,lowest_rsd_index,"
            "average_iteration_performance,fastest_iteration_performance,slowest_iteration_"
            "performance\n";
      for (const auto &name : benchmark::benchmark_execution_order) {
        const auto &this_result = results.at(name);
        os << this_result.to_csv() << "\n";
      }
      result = true;
    }
    os.close();
    return result;
  }
};

} // namespace criterion