/** @file
 *
 * A Sparse Virtual File implementation.
 *
 * Created by Paul Ross on 2020-04-29.
 *
 * @verbatim
    MIT License

    Copyright (c) 2020-2023 Paul Ross

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
 @endverbatim
 */

//#include <iostream>
//#include <iomanip>
#include <sstream>
//#include <thread>

#include "svfs.h"
#include "test_svfs.h"


namespace SVFS {
    namespace Test {

        // Simulate writing a low level RP66V1 index. Total bytes written around 1Mb.
        // Represented file size is about 190 Mb
        // 23831 * (4 + 10 * 4) is close to 1Mb
        TestCount _test_perf_write_sim_index_svfs(size_t count_vr, size_t count_lr, t_test_results &results) {
            TestCount count;
            SparseVirtualFileSystem svfs;
            std::string id = "ID";
            svfs.insert(id, 12.0);
            auto time_start = std::chrono::high_resolution_clock::now();

            for (size_t vr = 0; vr < count_vr; ++vr) {
                t_fpos fpos = 80 + vr * 8004;
                svfs.at(id).write(fpos, test_data_bytes_512, 4);
                fpos += 4;
                for (size_t lrsh = 0; lrsh < count_lr; ++lrsh) {
                    svfs.at(id).write(fpos, test_data_bytes_512, 4);
                    fpos += 8000 / count_lr;
                }
            }
            std::ostringstream os;
            os << "Sim SVFS:" << " vr=" << count_vr << " lr=" << count_lr;

            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            auto result = TestResult(__PRETTY_FUNCTION__, os.str(), 0, "", time_exec.count(), svfs.at(id).num_bytes());
            count.add_result(result.result());
            results.push_back(result);
            return count;
        }


        // Simulate writing a low level RP66V1 index. Total bytes written around 1Mb.
        // Represented file size is about 190 Mb
        // 23831 * (4 + 10 * 4) is close to 1Mb
        TestCount test_perf_write_sim_index_svfs(t_test_results &results) {
            TestCount count;
            count += _test_perf_write_sim_index_svfs(1, 10, results);
            count += _test_perf_write_sim_index_svfs(10, 10, results);
            count += _test_perf_write_sim_index_svfs(100, 10, results);
            count += _test_perf_write_sim_index_svfs(1000, 10, results);
            count += _test_perf_write_sim_index_svfs(10000, 10, results);
            count += _test_perf_write_sim_index_svfs(23831, 10, results);
            count += _test_perf_write_sim_index_svfs(100000, 10, results);
//        count += _test_perf_write_sim_index_svfs(1000000, 10, results);

//        SparseVirtualFileSystem svfs;
//        std::string id = "ID";
//        svfs.insert(id, 12.0);
//        auto time_start = std::chrono::high_resolution_clock::now();
//
//        for (size_t vr = 0; vr < 23831; ++vr) {
//            t_fpos fpos = 80 + vr * 8004;
//            svfs.at(id).write(fpos, test_data_bytes_512, 4);
//            fpos += 4;
//            for (int lrsh = 0; lrsh < 10; ++lrsh) {
//                svfs.at(id).write(fpos, test_data_bytes_512, 4);
//                fpos += 800;
//            }
//        }
//
//        std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
//        auto result = TestResult(__PRETTY_FUNCTION__, "Sim low level index on SVFS", 0, "", time_exec.count(), svfs.at(id).num_bytes());
//        count.add_result(result.result());
//        results.push_back(result);
            return count;
        }


        TestCount test_svfs_all(t_test_results &results) {
            TestCount count;
            count += test_perf_write_sim_index_svfs(results);
            return count;
        }
    } // namespace Test
} // namespace SVFS
