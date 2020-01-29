//
// Created by Paul Ross on 2020-01-22.
//

#include <iomanip>

#include "test.h"
#include "SaveStreamState.h"


void write_test_results(const t_test_results &results, std::ostream &os) {
    SaveStreamState state(os);

    // Header
    os << std::left;
    os << std::setw(40) << "Function";
    os << std::setw(40) << "Test";
    os << "----";
    os << std::right;
    os << std::setw(16) << "Bytes";
    os << std::setw(16) << "ms";
    os << std::setw(16) << "ms/Mb";
    os << std::setw(16) << "Mb/s";
    os << std::endl;

    for (const auto &result: results) {
        os << std::left;
        os << std::setw(40) << result.function();
        os << std::setw(40) << result.test();
        if (result.result() == 0) {
            os << "pass";
        } else {
            os << "FAIL";
        }
        os << std::right << std::fixed;
        os << std::setw(16) << result.work_bytes();
        os << std::setw(16) << std::setprecision(6) << 1000 * result.exec_time();
        os << std::setw(16) << std::setprecision(3) << result.ms_per_mb();
        os << std::setw(16) << std::setprecision(1) << result.work_rate() / (1024 * 1024);
        if (result.has_error_message()) {
            os << " " << result.error_message();
        }
        os << std::endl;
    }
}
