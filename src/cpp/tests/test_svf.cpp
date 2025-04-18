/** @file
 *
 * A Sparse Virtual File implementation.
 *
 * Created by Paul Ross on 2020-01-22.
 *
 * @verbatim
    MIT License

    Copyright (c) 2020-2025 Paul Ross

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

#include <iostream>
#include <iomanip>
#include <thread>

#include "test_svf.h"


/** Check that the basic code examples compiles and runs. */
void test_example_code(void) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    SVFS::SparseVirtualFile svf("Some file ID", 0.0);
    // Write six char at file position 14
    svf.write(14, "ABCDEF", 6);
    // Read from it
    char read_buffer[2];
    svf.read(16, 2, read_buffer);
    // What do I have to do to read 24 bytes from file position 8?
    // This returns a std::vector<std::pair<size_t, size_t>>
    // as ((file_position, read_length), ...)
    auto need = svf.need(8, 24);
    // This prints ((8, 6), (20, 4),)
    std::cout << "(";
    for (auto &val: need) {
        std::cout << "(" << val.first << ", " << val.second << "),";
    }
    std::cout << ")" << std::endl;
    std::cout << __PRETTY_FUNCTION__ << " DONE" << std::endl;
}

namespace SVFS {
    namespace Test {

        TestCaseWrite::TestCaseWrite(const std::string &m_test_name, const t_seek_reads &m_writes,
                                     const t_seek_reads &m_expected_blocks) : TestCaseABC(m_test_name, m_writes),
                                                                              m_expected_blocks(m_expected_blocks) {}

        /// @brief Create a SVF, run the write tests and report the result.
        TestResult TestCaseWrite::run() const {
            SparseVirtualFile svf("", 0.0);

            // Run the test
            size_t bytes_written = 0;
            auto time_start = std::chrono::high_resolution_clock::now();
            try {
                bytes_written += load_writes(svf, test_data_bytes_512);
            } catch (Exceptions::ExceptionSparseVirtualFile &err) {
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
            }
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

            // Analyse the results
            int result = 0;
            std::string err;
            auto actual_blocks = svf.blocks();
            size_t num_bytes = 0;
            if (m_expected_blocks.size() != actual_blocks.size()) {
                result = 1;
                std::ostringstream os;
                os << "Expected " << m_expected_blocks.size() << " blocks but got " << actual_blocks.size()
                   << " blocks";
                err = os.str();
                return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
            } else {
                for (size_t i = 0; i < m_expected_blocks.size(); ++i) {
                    if (actual_blocks[i].first != m_expected_blocks[i].first) {
                        result = 1;
                        std::ostringstream os;
                        os << "In block " << i << " expected fpos " << m_expected_blocks[i].first;
                        os << " but got " << actual_blocks[i].first << " (other blocks not tested)";
                        err = os.str();
                        return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(),
                                          svf.num_bytes());
                    }
                    if (actual_blocks[i].second != m_expected_blocks[i].second) {
                        result = 1;
                        std::ostringstream os;
                        os << "In block " << i << " expected length " << m_expected_blocks[i].second;
                        os << " but got " << actual_blocks[i].second << " (other blocks not tested)";
                        err = os.str();
                        return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(),
                                          svf.num_bytes());
                    }
                    num_bytes += actual_blocks[i].second;
                }
            }
            // Check SVF properties are correct.
            // Blocks
            if (svf.num_blocks() != actual_blocks.size()) {
                result = 1;
                std::ostringstream os;
                os << "Found svf.num_blocks() " << svf.num_blocks() << " but expected " << actual_blocks.size();
                err = os.str();
                return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
            }
            // Overall bytes
            if (svf.num_bytes() != num_bytes) {
                result = 1;
                std::ostringstream os;
                os << "Found svf.num_bytes() " << svf.num_bytes() << " but expected " << num_bytes;
                err = os.str();
                return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
            }
            // Write properties
            if (svf.count_write() != m_writes.size()) {
                result = 1;
                std::ostringstream os;
                os << "Found svf.count_write()" << svf.count_write() << " but expected " << m_writes.size();
                err = os.str();
                return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
            }
            if (svf.bytes_write() != bytes_written) {
                result = 1;
                std::ostringstream os;
                os << "Found svf.bytes_write() " << svf.bytes_write() << " but expected " << bytes_written;
                err = os.str();
                return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
            }
            // Read properties
            if (svf.count_read() != 0) {
                result = 1;
                std::ostringstream os;
                os << "Count of reads is " << svf.count_read() << " but should be 0";
                err = os.str();
                return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
            }
            if (svf.bytes_read() != 0) {
                result = 1;
                std::ostringstream os;
                os << "Count of read bytes is " << svf.bytes_read() << " but should be 0";
                err = os.str();
                return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
            }
            return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
        }

        /** @brief The actual write tests cases.
         *
         * This will raise an uncaught ExceptionTestConfiguration if miss configured.
         */
        const std::vector<TestCaseWrite> write_test_cases = {
#if  1
                /** Special case with error found in RaPiVot tiff_dump.py when using a SVF:
                 *
                 * @code
                    svf.write(0, b'A' * 8)
                    svf.write(3028, b'B' * 74)
                    svf.write(3214, b'C' * 19)
                    assert svf.blocks() == ((0, 8), (3028, 74), (3214, 19))
                    # This should not coalesce the last two blocks as 3102 + 12 = 3114 which is less than 3214
                    # It should extend the second block as 3028 + 74 = 3102
                    svf.write(3102, b'D' * 12)
                    # 3028 + 86 = 3114
                    assert svf.blocks() == ((0, 8), (3028, 86), (3214, 19))
                 * @endcode
                */
                {
                        "Special (A)",
                        {
                                {0, 8},
                                {28, 74},
                                {214, 19},
                        },
                        {
                                {0, 8},
                                {28, 74},
                                {214, 19},
                        },
                },
#endif
#if 1
                /* Now add svf.write(3102, b'D' * 12) */
                {
                        "Special (B)",
                        {
                                {0, 8},     /* 0-8 */
                                {28, 74},   /* 0-8, 28-102 */
                                {214, 19},  /* 0-8, 28-102, 214-233 */
                                {102, 12},  /* 0-8, 28-114, 214-233 */
                        },
                        /*             assert svf.blocks() == ((0, 8), (3028, 86), (3214, 19)) */
                        {
                                {0, 8},     /* 0-8 */
                                {28, 86},   /* 28-114 */
                                {214, 19},  /* 214-233 */
                        },
                },
#endif
#if 1
                /* Write no blocks. */
                {"Write no blocks", {}, {}},

                //        |+++|
                {"Write single block", {{8, 4}}, {{8, 4}},},

                //==== Old collects new and existing blocks: _write_old_append_new()
                //        ^==|
                //        |++|
                {"Overwrite single block", {{8, 4}, {8, 4}}, {{8, 4}},},

                //        ^==|
                //        |+++|
                {"Extend single block - a", {{8, 4}, {8, 5}}, {{8, 5}},},

                //        ^==|
                //         |++|
                {"Extend single block - b", {{8, 4}, {9, 4}}, {{8, 5}},},

                //        ^==|
                //            |+++|
                {"Coalesce two blocks", {{8, 4}, {12, 5}}, {{8, 9}},},

                //      |==|      ^==|
                {"Insert a previous block", {{16, 4}, {8, 4}}, {{8, 4}, {16, 4}},},
                //      ^==|   |==|  ^==|
                {"Insert a new block in the middle", {{16, 4}, {2, 4}, {8, 4}}, {{2, 4}, {8, 4}, {16, 4}},},
                //        ^==|    |==|
                {"Add second block", {{8, 4}, {16, 4}}, {{8, 4}, {16, 4}},},
                //        ^==|    |==|
                //          |++++++|
                {"New joins two blocks", {{8, 4}, {16, 4}, {10, 8}}, {{8, 12}},},
#endif

                //        ^==|    |==|
                //            |++|

                {"New just fills gap between two blocks", {{8, 4}, {16, 4}, {12, 4}}, {{8, 12}},},

                //        ^==|    |==|
                //        |++++++++++|
                {"New overlaps two blocks exactly", {{8, 4}, {16, 4}, {8, 12}}, {{8, 12}},},

                //        ^==|    |==|
                //         |++++++++|
                {"New overlaps two blocks just short", {{8, 4}, {16, 4}, {9, 10}}, {{8, 12}},},

                //        ^==|    |==|
                //        |++++++++++++|
                {"New overlaps two blocks and adds", {{8, 4}, {16, 4}, {8, 14}}, {{8, 14}},},

                //==== New collects existing blocks: _write_new_append_old()

                //        ^==|
                //    |++|
                {"New appends old[0]", {{8, 4}, {4, 4}}, {{4, 8}},},

                //        ^==|
                //       |++|
                {"New appends part of old[0]", {{8, 4}, {7, 3}}, {{7, 5}},},

                //        ^==|
                //       |+++|
                {"New overlaps end old[0] exactly", {{8, 4}, {7, 5}}, {{7, 5}},},

                //        ^==|
                //       |++++|
                {"New overlaps end old[0] and beyond", {{8, 4}, {7, 6}}, {{7, 6}},},

                //        ^==|    |==|
                //       |+++++|
                {"New appends old[0] not [1] (a)", {{8, 4}, {16, 4}, {7, 7}}, {{7, 7}, {16, 4}},},

                //        ^==|    |==|
                //       |++++++|
                {"New appends old[0] not [1] (b)", {{8, 4}, {16, 4}, {7, 8}}, {{7, 8}, {16, 4}},},

                //        ^==|    |==|
                //       |+++++++|
                {"New appends old[0] and [1] exactly", {{8, 4}, {16, 4}, {7, 9}}, {{7, 13}},},

                //        ^===|    |==|
                //       |+++++++++|
                {"New appends old[0] and [1] - just", {{8, 4}, {16, 4}, {7, 10}}, {{7, 13}},},

                //        ^===|    |==|
                //       |++++++++++|
                {"New appends old[0] and [1] - one byte", {{8, 4}, {16, 4}, {7, 11}}, {{7, 13}},},

                //        ^===|    |==|
                //       |++++++++++++|
                {"New appends old[0] and [1] - all", {{8, 4}, {16, 4}, {7, 13}}, {{7, 13}},},

                //        ^===|    |==|
                //       |+++++++++++++|
                {"New appends old[0] and [1] overlapped", {{8, 4}, {16, 4}, {7, 14}}, {{7, 14}},},
        };

        const std::vector<TestCaseWrite> write_test_cases_special = {
                //        ^==|
                //       |++|
                {"New appends part of old[0]", {{8, 4}, {7, 3}}, {{7, 5}},},
        };

        TestCount test_write_all(t_test_results &results) {
            TestCount count;
            for (const auto &test_case: write_test_cases) {
                auto result = test_case.run();
                count.add_result(result.result());
                results.push_back(result);
            }
            for (const auto &test_case: write_test_cases_special) {
                auto result = test_case.run();
                count.add_result(result.result());
                results.push_back(result);
            }
            return count;
        }


        TestCaseWriteThrows::TestCaseWriteThrows(const std::string &m_test_name, const t_seek_reads &m_writes,
                                                 t_fpos fpos, size_t len, const char *data, const std::string &message)
                : TestCaseABC(m_test_name,
                              m_writes),
                  m_fpos(fpos),
                  m_data(data),
                  m_len(len),
                  m_message(message) {}


        // Create a SVF, run the read tests and report the result.
        TestResult TestCaseWriteThrows::run() const {
            SparseVirtualFile svf("", 0.0);

            try {
                load_writes(svf, test_data_bytes_512);
                svf.write(m_fpos, m_data, m_len);
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, "Write test failed to throw.", 0.0, 0);
            } catch (Exceptions::ExceptionSparseVirtualFileWrite &err) {
                if (err.message() != m_message) {
                    std::ostringstream os;
                    os << "Error message \"" << err.message() << "\" expected \"" << m_message << "\"";
                    return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, os.str(), 0.0, svf.num_bytes());
                }
            }
            return TestResult(__PRETTY_FUNCTION__, m_test_name, 0, "", 0.0, svf.num_bytes());
        }

        const std::vector<TestCaseWriteThrows> write_test_cases_throws = {
                {
                        "Throws: Overwrite single block", {{65, 4}}, 65, 4, test_data_bytes_512 + 66,
                        "SparseVirtualFile::write(): Difference at position 65 'B' != 'A' Ordinal 66 != 65"
                },
        };

        TestCount test_write_all_throws(t_test_results &results) {
            TestCount count;
            for (const auto &test_case: write_test_cases_throws) {
                auto result = test_case.run();
                count.add_result(result.result());
                results.push_back(result);
            }
            return count;
        }

        // Write test_data_bytes_512 with a overlap with diff checking on. Data is 128 blocks every 64 bytes.
        TestCount _test_perf_write_with_diff_check(bool compare_for_diff, t_test_results &results) {
            TestCount count;
            tSparseVirtualFileConfig config;
            config.compare_for_diff = compare_for_diff;
            SparseVirtualFile svf("", 0.0, config);
            size_t block_size = 256;
            int repeat = 4000;

            auto time_start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < repeat; ++i) {
                svf.write(0, test_data_bytes_512, block_size);
            }
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            std::ostringstream os;
            os << block_size << " block size, x" << repeat << ", compare_for_diff=" << compare_for_diff;
            auto result = TestResult(__PRETTY_FUNCTION__, std::string(os.str()), 0, "", time_exec.count(),
                                     repeat * block_size);
            count.add_result(result.result());
            results.push_back(result);
            return count;
        }

        TestCount test_perf_write_with_diff_check(t_test_results &results) {
            return _test_perf_write_with_diff_check(false, results);
        }

        // Write test_data_bytes_512 with a overlap with diff checking on. Data is 128 blocks every 64 bytes.
        TestCount test_perf_write_without_diff_check(t_test_results &results) {
            return _test_perf_write_with_diff_check(true, results);
        }

        // Simulate writing a low level RP66V1 index. Total bytes written around 1Mb.
        // Represented file size is about 190 Mb
        // 23831 * (4 + 10 * 4) is close to 1Mb
        TestCount test_perf_write_sim_index_svf(t_test_results &results) {
            TestCount count;
            SparseVirtualFile svf("", 0.0);
            auto time_start = std::chrono::high_resolution_clock::now();

            for (size_t vr = 0; vr < 23831; ++vr) {
                t_fpos fpos = 80 + vr * 8004;
                svf.write(fpos, test_data_bytes_512, 4);
                fpos += 4;
                for (int lrsh = 0; lrsh < 10; ++lrsh) {
                    svf.write(fpos, test_data_bytes_512, 4);
                    fpos += 800;
                }
            }

            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            auto result = TestResult(__PRETTY_FUNCTION__, "Sim low level index", 0, "", time_exec.count(),
                                     svf.num_bytes());
            count.add_result(result.result());
            results.push_back(result);
            return count;
        }

        // Write 1Mb of test_data_bytes_512 in different, equally sized, blocks that are all coalesced and report the time taken.
        // Essentially only one block is created and all the other test_data_bytes_512 is appended.
        TestCount test_perf_write_1M_coalesced(t_test_results &results) {
            TestCount count;
            for (size_t block_size = 1; block_size <= 256; block_size *= 2) {
                SparseVirtualFile svf("", 0.0);

                auto time_start = std::chrono::high_resolution_clock::now();
                for (t_fpos i = 0; i < (1024 * 1024 * 1) / block_size; ++i) {
                    t_fpos fpos = i * block_size;
                    svf.write(fpos, test_data_bytes_512, block_size);
                }
                std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

                std::ostringstream os;
                os << "1Mb, " << std::setw(3) << block_size << " sized blocks, coalesced";
                auto result = TestResult(__PRETTY_FUNCTION__, std::string(os.str()), 0, "", time_exec.count(),
                                         svf.num_bytes());
                count.add_result(result.result());
                results.push_back(result);
            }
            return count;
        }

        // Write 1Mb of test_data_bytes_512 in different, equally sized, blocks that are not coalesced and report the time taken.
        // Each write creates a separate block.
        TestCount test_perf_write_1M_uncoalesced(t_test_results &results) {
            TestCount count;
            for (size_t block_size = 1; block_size <= 256; block_size *= 2) {
                SparseVirtualFile svf("", 0.0);

                auto time_start = std::chrono::high_resolution_clock::now();
                for (t_fpos i = 0; i < (1024 * 1024 * 1) / block_size; ++i) {
                    t_fpos fpos = i * block_size + i;
                    svf.write(fpos, test_data_bytes_512, block_size);
                }
                std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

                std::ostringstream os;
                os << "1Mb, " << std::setw(3) << block_size << " sized blocks, uncoalesced";
                auto result = TestResult(__PRETTY_FUNCTION__, std::string(os.str()), 0, "", time_exec.count(),
                                         svf.num_bytes());
                count.add_result(result.result());
                results.push_back(result);
            }

            return count;
        }


        // Write 1Mb of test_data_bytes_512 in different, equally sized, blocks that are not coalesced and report the memory usage
        // with size_of(). Each write creates a separate  block.
        // Typically 1 byte  blocks are x35 times the memory. 256 byte blocks are x1.2 the memory.
        TestCount test_perf_write_1M_uncoalesced_size_of(t_test_results &results) {
            TestCount count;
            for (size_t block_size = 1; block_size <= 256; block_size *= 2) {
                SparseVirtualFile svf("", 0.0);

                size_t num_blocks = (1024 * 1024 * 1) / block_size;

                auto time_start = std::chrono::high_resolution_clock::now();
                for (t_fpos i = 0; i < num_blocks; ++i) {
                    t_fpos fpos = i * block_size + i;
                    svf.write(fpos, test_data_bytes_512, block_size);
                }
                std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

                std::ostringstream os;
                os << "1Mb, block size " << std::setw(3) << block_size << " sized blocks";
                os << " num_blocks " << num_blocks;
                os << " size_of " << svf.size_of();
                os << " Overhead " << svf.size_of() - svf.num_bytes();
                os << " per block " << (svf.size_of() - svf.num_bytes()) / num_blocks;
                auto result = TestResult(__PRETTY_FUNCTION__, std::string(os.str()), 0, "", time_exec.count(),
                                         svf.size_of());
                count.add_result(result.result());
                results.push_back(result);
            }

            return count;
        }


        TestCaseRead::TestCaseRead(const std::string &m_test_name, const t_seek_reads &m_writes,
                                   t_fpos fpos, size_t len) : TestCaseABC(m_test_name, m_writes),
                                                              m_fpos(fpos), m_len(len) {}


        // Create a SVF, run the read tests and report the result.
        TestResult TestCaseRead::run() const {
            SparseVirtualFile svf("", 0.0);

            // Load the SVF
            try {
                load_writes(svf, test_data_bytes_512);
            } catch (Exceptions::ExceptionSparseVirtualFile &err) {
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
            }

            // Analyse the results
            int result = 0;
            std::string err;

            char read_buffer[256];
            // Run the test
            auto time_start = std::chrono::high_resolution_clock::now();
            try {
                svf.read(m_fpos, m_len, read_buffer);
            } catch (Exceptions::ExceptionSparseVirtualFileRead &err) {
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
            }
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

            // Check the result
            for (size_t i = 0; i < m_len; ++i) {
                if (read_buffer[i] != test_data_bytes_512[m_fpos + i]) {
                    result = 1;
                    std::ostringstream os;
                    os << "In position " << m_fpos + 1 << " expected fpos "
                       << static_cast<int>(test_data_bytes_512[m_fpos + i]);
                    os << " but got " << static_cast<int>(read_buffer[i]) << " (other test_data_bytes_512 not tested)";
                    err = os.str();
                    return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(),
                                      svf.num_bytes());
                }
            }
            return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
        }

        const std::vector<TestCaseRead> read_test_cases = {
                //        ^==|
                //        |++|
                {"Read exactly a block",        {{8, 4}}, 8, 4},
                //        ^==|
                //        |+|
                {"Read leading part of block",  {{8, 4}}, 8, 3},
                //        ^==|
                //         |+|
                {"Read trailing part of block", {{8, 4}}, 9, 3},
                //        ^==|
                //         ||
                {"Read mid part of block",      {{8, 4}}, 9, 2},
        };


        const std::vector<TestCaseRead> read_test_cases_special = {
                //        ^==|
                //         |+|
                {"Read trailing part of block", {{8, 4}}, 9, 3},
                //        ^==|
                //         ||
                {"Read mid part of block",      {{8, 4}}, 9, 2},
        };


        TestCount test_read_all(t_test_results &results) {
            TestCount count;
            for (const auto &test_case: read_test_cases) {
//        for (const auto& test_case: read_test_cases_special) {
//            std::cout << "Testing: " << test_case.test_name() << std::endl;
                auto result = test_case.run();
                count.add_result(result.result());
                results.push_back(result);
            }
            return count;
        }


        TestCaseReadThrows::TestCaseReadThrows(const std::string &m_test_name, const t_seek_reads &m_writes,
                                               t_fpos fpos, size_t len, const std::string &message) : TestCaseRead(
                m_test_name,
                m_writes, fpos,
                len),
                                                                                                      m_message(
                                                                                                              message) {}


        // Create a SVF, run the read tests and report the result.
        TestResult TestCaseReadThrows::run() const {
            SparseVirtualFile svf("", 0.0);

            // Load the SVF
            try {
                load_writes(svf, test_data_bytes_512);
            } catch (Exceptions::ExceptionSparseVirtualFile &err) {
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
            }
            // Run the test.
            char read_buffer[256];
            try {
                svf.read(m_fpos, m_len, read_buffer);
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, "Test failed to throw.", 0.0, 0);
            } catch (Exceptions::ExceptionSparseVirtualFileRead &err) {
                if (err.message() != m_message) {
                    std::ostringstream os;
                    os << "Error message \"" << err.message() << "\" expected \"" << m_message << "\"";
                    return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, os.str(), 0.0, svf.num_bytes());
                }
            }
            return TestResult(__PRETTY_FUNCTION__, m_test_name, 0, "", 0.0, svf.num_bytes());
        }

        const std::vector<TestCaseReadThrows> read_test_cases_throw = {
                {"Read empty SVF throws",      {},       8,  4, "SparseVirtualFile::read(): Sparse virtual file is empty."},
                /**
                 * @code
                 *          ^==|
                 *   |++|
                 * @endcode
                 */
                {"Read before block throws",   {{8, 4}}, 2,  4,
                                                                "SparseVirtualFile::read(): Requested file position 2 precedes first block at 8"},
                ///        ^==|
                ///       |++|
                {"Read prior to block throws", {{8, 4}}, 7,  4,
                                                                "SparseVirtualFile::read(): Requested file position 7 precedes first block at 8"},
                ///        ^==|
                ///         |++|
                {"Read beyond block throws",   {{8, 4}}, 9,  4,
                                                                "SparseVirtualFile::read(): Requested position 9 length 4 (end 13) overruns block that starts at 8 has size 4 (end 12). Offset into block is 1 overrun is 1 bytes"},
                ///        ^==|
                ///             |++|
                {"Read beyond end throws",     {{8, 4}}, 12, 4,
                                                                "SparseVirtualFile::read(): Requested position 12 length 4 (end 16) overruns block that starts at 8 has size 4 (end 12). Offset into block is 4 overrun is 4 bytes"},
        };


        TestCount test_read_throws_all(t_test_results &results) {
            TestCount count;
            for (const auto &test_case: read_test_cases_throw) {
//            std::cout << "Testing: " << test_case.test_name() << std::endl;
                auto result = test_case.run();
                count.add_result(result.result());
                results.push_back(result);
            }
            return count;
        }


        // Write 1Mb of test_data_bytes_512 in different, equally sized, blocks that are all coalesced and report the time taken.
        // Essentially only one block is created and all the other test_data_bytes_512 is appended.
        TestCount test_perf_read_1M_coalesced(t_test_results &results) {
            const size_t SIZE = 1024 * 1024 * 1;
            TestCount count;
            SparseVirtualFile svf("", 0.0);
            for (t_fpos i = 0; i < (SIZE) / 256; ++i) {
                t_fpos fpos = i * 256;
                svf.write(fpos, test_data_bytes_512, 256);
            }

            char buffer[SIZE];
            auto time_start = std::chrono::high_resolution_clock::now();
            svf.read(0, SIZE, buffer);
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

            std::ostringstream os;
            auto result = TestResult(__PRETTY_FUNCTION__, "1Mb of 256 bytes in one block", 0, "", time_exec.count(),
                                     svf.num_bytes());
            count.add_result(result.result());
            results.push_back(result);
            return count;
        }

        // Read 1Mb of test_data_bytes_512 in different, equally sized, blocks that are not coalesced and report the time taken.
        TestCount test_perf_read_1M_un_coalesced(t_test_results &results) {
            const size_t SIZE = 1024 * 1024 * 1;
            TestCount count;
            for (size_t block_size = 1; block_size <= 512; block_size *= 2) {
                SparseVirtualFile svf("", 0.0);
                for (t_fpos i = 0; i < (SIZE) / block_size; ++i) {
                    t_fpos fpos = i * 512 * 2;
                    svf.write(fpos, test_data_bytes_512, block_size);
                }

                char buffer[SIZE];
                auto time_start = std::chrono::high_resolution_clock::now();
                for (t_fpos i = 0; i < (SIZE) / block_size; ++i) {
                    t_fpos fpos = i * 512 * 2;
                    svf.read(fpos, block_size, buffer);
                }
                std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

                std::ostringstream os;
                os << "1Mb " << block_size << " byte blocks " << svf.num_blocks() << " blocks ";
                auto result = TestResult(__PRETTY_FUNCTION__, os.str(), 0, "", time_exec.count(),
                                         svf.num_bytes());
                count.add_result(result.result());
                results.push_back(result);
            }
            return count;
        }

        TestCaseHas::TestCaseHas(const std::string &m_test_name, const t_seek_reads &m_writes,
                                 t_fpos fpos, size_t len, bool expected) : TestCaseABC(m_test_name, m_writes),
                                                                           m_fpos(fpos), m_len(len),
                                                                           m_expected(expected) {}


        // Create a SVF, run the read tests and report the result.
        TestResult TestCaseHas::run() const {
            SparseVirtualFile svf("", 0.0);

            // Load the SVF
            try {
                load_writes(svf, test_data_bytes_512);
            } catch (Exceptions::ExceptionSparseVirtualFile &err) {
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
            }

            // Analyse the results

            // Run the test
            try {
                auto time_start = std::chrono::high_resolution_clock::now();
                bool result_has = svf.has(m_fpos, m_len);
                std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

                // Check the result
                if (result_has != m_expected) {
                    std::ostringstream os;
                    os << "Expected has() in position " << m_fpos << " and length " << m_len;
                    return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, os.str(), time_exec.count(),
                                      svf.num_bytes());
                }
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 0, "", time_exec.count(), svf.num_bytes());
            } catch (Exceptions::ExceptionSparseVirtualFileRead &err) {
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
            }
        }

        const std::vector<TestCaseHas> has_test_cases = {
                //
                //        |++|
                {"Has empty - false",         {},       8, 4, false},
                //        ^==|
                //        |++|
                {"Has an exact block",        {{8, 4}}, 8, 4, true},
                //        ^==|
                //        |+|
                {"Has leading block",         {{8, 4}}, 8, 3, true},
                //        ^==|
                //         |+|
                {"Has trailing block",        {{8, 4}}, 9, 3, true},
                //        ^==|
                //         ||
                {"Has mid block",             {{8, 4}}, 9, 2, true},
                //        ^==|
                //       |++|
                {"Not has an exact block -1", {{8, 4}}, 7, 4, false},
                //        ^==|
                //         |++|
                {"Not has an exact block +1", {{8, 4}}, 9, 4, false},
        };


        TestCount test_has_all(t_test_results &results) {
            TestCount count;
            for (const auto &test_case: has_test_cases) {
                auto result = test_case.run();
                count.add_result(result.result());
                results.push_back(result);
            }
            return count;
        }

        TestCaseNeed::TestCaseNeed(const std::string &m_test_name, const t_seek_reads &m_writes,
                                   t_fpos fpos, size_t len, const t_seek_reads &m_need) : TestCaseABC(m_test_name,
                                                                                                      m_writes),
                                                                                          m_fpos(fpos), m_len(len),
                                                                                          m_need(m_need) {}


        // Create a SVF, run the read tests and report the result.
        TestResult TestCaseNeed::run() const {
            SparseVirtualFile svf("", 0.0);

            // Load the SVF
            try {
                load_writes(svf, test_data_bytes_512);
            } catch (Exceptions::ExceptionSparseVirtualFile &err) {
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
            }

            // Analyse the results
            // Run the test
            auto time_start = std::chrono::high_resolution_clock::now();
            t_seek_reads need = svf.need(m_fpos, m_len);
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

            // Check the result
            if (need.size() != m_need.size()) {
                std::ostringstream os;
                os << "Found " << need.size() << " need pairs but expected " << m_need.size() << " need pairs";
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, os.str(), time_exec.count(), svf.num_bytes());
            }
            for (size_t i = 0; i < need.size(); ++i) {
                if (need[i].first != m_need[i].first || need[i].second != m_need[i].second) {
                    std::ostringstream os;
                    os << "In position " << i << " expected fpos " << m_need[i].first << " and len "
                       << m_need[i].second;
                    os << " but got fpos " << need[i].first << " and len " << need[i].second;
                    return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, os.str(), time_exec.count(),
                                      svf.num_bytes());
                }
            }
            return TestResult(__PRETTY_FUNCTION__, m_test_name, 0, "", time_exec.count(), svf.num_bytes());
        }

        const std::vector<TestCaseNeed> need_test_cases = {
                //
                //        |++|
                {"Need on empty SVF",                  {},                8,  4,  {{8,  4}}},
                //        ^==|
                //        |++|
                {"Exactly one block",                  {{8, 4}},          8,  4,  {},},
                //        ^==|
                //         ||
                {"Inside one block",                   {{8, 4}},          9,  2,  {},},
                //        ^==|
                //    |++|
                {"All before one block",               {{8, 4}},          4,  4,  {{4,  4}},},
                //        ^==|
                //            |++|
                {"All after one block",                {{8, 4}},          12, 4,  {{12, 4}},},
                //        ^==|
                //    |+++++|
                {"Before and part of one block",       {{8, 4}},          4,  7,  {{4,  4}},},
                //        ^==|
                //    |++++++|
                {"Before and all of one block",        {{8, 4}},          4,  8,  {{4,  4}},},
                //        ^==|
                //    |++++++|
                {"Before, all of one block and after", {{8, 4}},          4,  15, {{4,  4}, {12, 7}},},
                //        ^==|
                //    |++++++++++|
                {"Before, all and after one block",    {{8, 4}},          4,  9,  {{4,  4}, {12, 1}},},
                //        |==|  |==|
                //        |++++++++|
                {"Two blocks and in between (a)",      {{8, 4}, {14, 4}}, 8,  10, {{12, 2}},},
                //        |==|  |==|
                //        |+++++++|
                {"Two blocks and in between (b)",      {{8, 4}, {14, 4}}, 8,  9,  {{12, 2}},},
                //        |==|  |==|
                //         |+++++++|
                {"Two blocks and in between (c)",      {{8, 4}, {14, 4}}, 9,  9,  {{12, 2}},},
                //        |==|  |==|
                //         |++++++|
                {"Two blocks and in between (d)",      {{8, 4}, {14, 4}}, 9,  7,  {{12, 2}},},
                //        |==|  |==|
                //       |++++++++|
                {"Two blocks, under-run",              {{8, 4}, {14, 4}}, 7,  11, {{7,  1}, {12, 2}},},
                //        |==|  |==|
                //        |+++++++++|
                {"Two blocks, over-run",               {{8, 4}, {14, 4}}, 8,  11, {{12, 2}, {18, 1}},},
                //        |==|  |==|
                //       |++++++++++|
                {"Two blocks, under/over-run",         {{8, 4}, {14, 4}}, 7,  12, {{7,  1}, {12, 2}, {18, 1}},},
        };

        const std::vector<TestCaseNeed> need_test_cases_special = {
                //        |==|  |==|
                //        |++++++++|
                {"Two blocks and in between (a)", {{8, 4}, {14, 4}}, 8, 10, {{12, 2}},},
        };

        TestCount test_need_all(t_test_results &results) {
            TestCount count;
            for (const auto &test_case: need_test_cases) {
                auto result = test_case.run();
                count.add_result(result.result());
                results.push_back(result);
            }
            for (const auto &test_case: need_test_cases_special) {
                auto result = test_case.run();
                count.add_result(result.result());
                results.push_back(result);
            }
            return count;
        }

        // Simulate writing a low level RP66V1 index and then running need on it. Total bytes written around 1Mb.
        // Blocks are 800 bytes apart.
        // 23831 * (4 + 10 * 4) is close to 1Mb
        TestCount _test_perf_need_sim_index(size_t need_size, t_test_results &results) {
            TestCount count;
            SparseVirtualFile svf("", 0.0);
            // Write to the SVF
            for (size_t vr = 0; vr < 23831; ++vr) {
                t_fpos fpos = 80 + vr * 8004;
                svf.write(fpos, test_data_bytes_512, 4);
                fpos += 4;
                for (int lrsh = 0; lrsh < 10; ++lrsh) {
                    svf.write(fpos, test_data_bytes_512, 4);
                    fpos += 800;
                }
            }
            // Now run need
            size_t data_size = 0;
            size_t num_need_blocks = 0;
            auto time_start = std::chrono::high_resolution_clock::now();
            for (size_t i = 0; i < svf.last_file_position(); i += need_size) {
                auto need = svf.need(i, need_size);
                num_need_blocks += need.size();
                data_size += need_size;
            }
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            std::ostringstream os;
            os << "Sim need(" << need_size << ") on index [" << num_need_blocks << "]";
            auto result = TestResult(__PRETTY_FUNCTION__, os.str(), 0, "", time_exec.count(), data_size);
            count.add_result(result.result());
            results.push_back(result);
            return count;
        }

        TestCount test_perf_need_sim_index(t_test_results &results) {
            TestCount count;
            for (size_t need_size = 32; need_size < 8 * 4096; need_size *= 2) {
                count += _test_perf_need_sim_index(need_size, results);
            }
            return count;
        }

#pragma mark - Test need() with greedy value.

        TestCaseNeedGreedy::TestCaseNeedGreedy(
                const std::string &m_test_name, const t_seek_reads &m_writes, t_fpos fpos,
                size_t len, size_t greedy_length, const t_seek_reads &m_need) : TestCaseABC(m_test_name, m_writes),
                                                                                m_fpos(fpos),
                                                                                m_len(len),
                                                                                m_greedy_length(
                                                                                        greedy_length),
                                                                                m_need(m_need) {
        }

        TestResult TestCaseNeedGreedy::run() const {
            SparseVirtualFile svf("", 0.0);

            // Load the SVF
            try {
                load_writes(svf, test_data_bytes_512);
            } catch (Exceptions::ExceptionSparseVirtualFile &err) {
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
            }

            // Analyse the results
            // Run the test
            auto time_start = std::chrono::high_resolution_clock::now();
            t_seek_reads need = svf.need(m_fpos, m_len, m_greedy_length);
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

            // Check the result
            if (need.size() != m_need.size()) {
                std::ostringstream os;
                os << "Found " << need.size() << " need pairs but expected " << m_need.size() << " need pairs";
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, os.str(), time_exec.count(), svf.num_bytes());
            }
            for (size_t i = 0; i < need.size(); ++i) {
                if (need[i].first != m_need[i].first || need[i].second != m_need[i].second) {
                    std::ostringstream os;
                    os << "In position " << i << " expected fpos " << m_need[i].first << " and len "
                       << m_need[i].second;
                    os << " but got fpos " << need[i].first << " and len " << need[i].second;
                    return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, os.str(), time_exec.count(),
                                      svf.num_bytes());
                }
            }
            return TestResult(__PRETTY_FUNCTION__, m_test_name, 0, "", time_exec.count(), svf.num_bytes());
        }

// clang-format off
// @formatter:off
        const std::vector<TestCaseNeedGreedy> need_greedy_test_cases = {
#if 1
                TestCaseNeedGreedy(
                        "Need (greedy=0) on empty SVF",
                        {},
                        8, 4, 0,
                        {{8, 4}}
                ),
                TestCaseNeedGreedy(
                        "Need 32 (greedy=4) on empty SVF",
                        {},
                        8, 32, 4,
                        {{8, 32}}
                ),
                TestCaseNeedGreedy(
                        "Need (greedy=32) on empty SVF",
                        {},
                        8, 4, 32,
                        {{8, 32}}
                ),
                TestCaseNeedGreedy(
                        "Need (greedy=0)",
                        {{8,  4},
                         {16, 4},
                         {32, 4}},
                        8, 40, 0,
                        {{12, 4},
                         {20, 12},
                         {36, 12},}
                ),
                TestCaseNeedGreedy(
                        "Need (greedy=64)",
                        {{8,  4},
                         {16, 4},
                         {32, 4}},
                        8, 40, 64,
                        {{12, 64},}
                ),
#endif
                // Test where original length of need is < greedy length.
                TestCaseNeedGreedy(
                        "Need with write one byte un-coalesced (greedy=8)",
                        {
                                {0,  1},
                                {2,  1},
                                {4,  1},
                                {6,  1},
                                {8,  1},
                                {10, 1},
                                {12, 1},
                                {14, 1},
                                {16, 1},
                        },
                        0, 2, 8,
                        {{1, 8},}
                ),
        };
// @formatter:on
// clang-format on

        TestCount test_need_greedy_all(t_test_results &results) {
            TestCount count;
            for (const auto &test_case: need_greedy_test_cases) {
                auto result = test_case.run();
                count.add_result(result.result());
                results.push_back(result);
            }
            return count;
        }


#pragma mark - Test erase()

        TestCaseErase::TestCaseErase(const std::string &m_test_name, const t_seek_reads &m_writes,
                                     t_fpos fpos) : TestCaseABC(m_test_name, m_writes),
                                                    m_fpos(fpos) {}


        // Create a SVF, run the read tests and report the result.
        TestResult TestCaseErase::run() const {
            SparseVirtualFile svf("", 0.0);

            // Load the SVF
            try {
                load_writes(svf, test_data_bytes_512);
            } catch (Exceptions::ExceptionSparseVirtualFile &err) {
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
            }

            // Analyse the results
            int result = 0;
            std::string err;

            // Run the test
            auto time_start = std::chrono::high_resolution_clock::now();
            try {
                svf.erase(m_fpos);
            } catch (Exceptions::ExceptionSparseVirtualFileRead &err) {
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
            }
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
        }

        const std::vector<TestCaseErase> erase_test_cases = {
                //        ^==|
                //        |++|
                {"Erase a block", {{8, 4}}, 8},
        };

        TestCount test_erase_all(t_test_results &results) {
            TestCount count;
            for (const auto &test_case: erase_test_cases) {
//        for (const auto& test_case: read_test_cases_special) {
//            std::cout << "Testing: " << test_case.test_name() << std::endl;
                auto result = test_case.run();
                count.add_result(result.result());
                results.push_back(result);
            }
            return count;
        }

        TestCaseEraseThrows::TestCaseEraseThrows(const std::string &m_test_name, const t_seek_reads &m_writes,
                                                 t_fpos fpos, const std::string &message) : TestCaseErase(m_test_name,
                                                                                                          m_writes,
                                                                                                          fpos),
                                                                                            m_message(message) {}

        // Create a SVF, run the read tests and report the result.
        TestResult TestCaseEraseThrows::run() const {
            SparseVirtualFile svf("", 0.0);

            // Load the SVF
            try {
                load_writes(svf, test_data_bytes_512);
            } catch (Exceptions::ExceptionSparseVirtualFile &err) {
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
            }
            // Run the test.
            try {
                svf.erase(m_fpos);
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, "Test failed to throw.", 0.0, 0);
            } catch (Exceptions::ExceptionSparseVirtualFileErase &err) {
                if (err.message() != m_message) {
                    std::ostringstream os;
                    os << "Error message \"" << err.message() << "\" expected \"" << m_message << "\"";
                    return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, os.str(), 0.0, svf.num_bytes());
                }
            }
            return TestResult(__PRETTY_FUNCTION__, m_test_name, 0, "", 0.0, svf.num_bytes());
        }


        const std::vector<TestCaseEraseThrows> erase_test_cases_throw = {
                {"Erase empty SVF throws",      {},       8, "SparseVirtualFile::erase(): Non-existent file position 8 at start of block."},
                //        ^==|
                //  |++|
                {"Erase before block throws",   {{8, 4}}, 2,
                                                             "SparseVirtualFile::erase(): Non-existent file position 2 at start of block."},
                //        ^==|
                //       |++|
                {"Erase within a block throws", {{8, 4}}, 9,
                                                             "SparseVirtualFile::erase(): Non-existent file position 9 at start of block."},
                //        ^==|
                //             |++|
                {"Erase beyond end throws",     {{8, 4}}, 12,
                                                             "SparseVirtualFile::erase(): Non-existent file position 12 at start of block."},
        };


        TestCount test_erase_throws_all(t_test_results &results) {
            TestCount count;
            for (const auto &test_case: erase_test_cases_throw) {
//            std::cout << "Testing: " << test_case.test_name() << std::endl;
                auto result = test_case.run();
                count.add_result(result.result());
                results.push_back(result);
            }
            return count;
        }

        TestCount _test_perf_erase_overwrite(bool overwrite, t_test_results &results) {
            TestCount count;
            size_t block_size = 256;
            size_t total_size = 1024 * 1024 * 1;
            int repeat = 1000;
            tSparseVirtualFileConfig config;
            config.overwrite_on_exit = overwrite;
            SparseVirtualFile svf("", 0.0, config);
            double time_total = 0.0;
            for (int r = 0; r < repeat; ++r) {
                for (t_fpos i = 0; i < total_size / block_size; ++i) {
                    // Add 1 to make non-coalesced
                    t_fpos fpos = i * block_size + 1;
                    svf.write(fpos, test_data_bytes_512, block_size);
                }
                auto time_start = std::chrono::high_resolution_clock::now();
                svf.clear();
                std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
                time_total += time_exec.count();
            }
            std::ostringstream os;
            os << "1Mb, " << std::setw(3) << block_size << "block size, x" << repeat << " overwrite=" << overwrite;
            auto result = TestResult(__PRETTY_FUNCTION__, std::string(os.str()), 0, "", time_total, total_size);
            count.add_result(result.result());
            results.push_back(result);
            return count;
        }

        TestCount test_perf_erase_overwrite_false(t_test_results &results) {
            return _test_perf_erase_overwrite(false, results);
        }

        TestCount test_perf_erase_overwrite_true(t_test_results &results) {
            return _test_perf_erase_overwrite(true, results);
        }

#ifdef SVF_THREAD_SAFE
        SparseVirtualFile g_svf_multithreaded("", 0.0);

        // This writes to the global SVF and is used by test_write_multithreaded_num_threads in multiple threads.
        void _write_multithreaded_coalesced() {
            try {
                for (size_t fpos = 0; fpos < 1024 * 1024; fpos += 8) {
                    g_svf_multithreaded.write(fpos, test_data_bytes_512, 8);
                }
            }
            catch (Exceptions::ExceptionSparseVirtualFile &err) {
                std::cout << __FUNCTION__ << "(): Fails: " << err.message() << std::endl;
            }
        }

        void _write_multithreaded_un_coalesced() {
            try {
                for (size_t fpos = 0; fpos < 1024 * 1024 * 2; fpos += 16) {
                    g_svf_multithreaded.write(fpos, test_data_bytes_512, 8);
                }
            }
            catch (Exceptions::ExceptionSparseVirtualFile &err) {
                std::cout << __FUNCTION__ << "(): Fails: " << err.message() << std::endl;
            }
        }

        // Launches num_threads threads and writes to a global SVF in the manner of test_perf_write_sim_index()
        TestCount test_write_multithreaded(int num_threads, bool is_coalesced, t_test_results &results) {
            TestCount count;
            std::vector<std::thread> threads;
            g_svf_multithreaded.clear();

            // Timed section
            auto time_start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < num_threads; ++i) {
                if (is_coalesced) {
                    threads.push_back(std::thread(_write_multithreaded_coalesced));
                } else {
                    threads.push_back(std::thread(_write_multithreaded_un_coalesced));
                }
            }
            for (size_t i = 0; i < threads.size(); ++i) {
                threads[i].join();
            }
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            // END: Timed section

            size_t work_done = num_threads * g_svf_multithreaded.num_bytes();
            g_svf_multithreaded.clear();

            std::ostringstream os;
            os << "Multi threaded write [" << num_threads << "] Coalesced " << is_coalesced;
            auto result = TestResult(__PRETTY_FUNCTION__, os.str(), 0, "", time_exec.count() / num_threads, work_done);
            count.add_result(result.result());
            results.push_back(result);
            return count;
        }

        TestCount test_write_multithreaded_coalesced(t_test_results &results) {
            TestCount count;
            for (int num_threads = 1; num_threads < 1 << 8; num_threads *= 2) {
                count += test_write_multithreaded(num_threads, true, results);
            }
            return count;
        }

        TestCount test_write_multithreaded_un_coalesced(t_test_results &results) {
            TestCount count;
            for (int num_threads = 1; num_threads < 1 << 8; num_threads *= 2) {
                count += test_write_multithreaded(num_threads, false, results);
            }
            return count;
        }

#endif

        /**
         * This bug from the simulator:
         *
         * @code
            2023-04-25 12:05:24,776 -             simulator.py#71   - INFO     - CLIENT:  blocks was: ['(0 : 1,024 : 1,024)', '(291,809,396 : 1,024 : 291,810,420)']
            2023-04-25 12:05:24,776 -             simulator.py#72   - INFO     - CLIENT: demands fpos      291,810,392 length  2,429 (     291,812,821)
            2023-04-25 12:05:24,776 -             simulator.py#81   - INFO     - CLIENT:   needs fpos      291,810,420 length  2,401 (     291,812,821)
            2023-04-25 12:05:24,799 -             simulator.py#90   - INFO     - CLIENT:   wrote fpos      291,810,420 length  2,401 (     291,812,821)
            2023-04-25 12:05:24,799 -             simulator.py#92   - ERROR    - CLIENT: demands fpos      291,810,392 length  2,429 (     291,812,821)
            2023-04-25 12:05:24,799 -             simulator.py#96   - ERROR    - CLIENT:  blocks now: ['(0 : 1,024 : 1,024)', '(291,809,396 : 3,397 : 291,812,793)']
         * @endcode
         */
        TestResult test_debug_need_read_special_A() {
            SparseVirtualFile svf("", 0.0);
            std::string test_name(__FUNCTION__);
            static char data[4096]; // Random

            // Load the SVF
            svf.write(0, data, 1024);
            svf.write(291809396, data, 1024);
            auto blocks = svf.blocks();

            // Analyse the results
            int result = 0; // Success
            std::string err;
            // Run the test
            auto time_start = std::chrono::high_resolution_clock::now();
            bool has = svf.has(291810392, 2429);
            result |= has;
            auto need = svf.need(291810392, 2429, 1024);
            svf.write(291810420, data, 2401);
            blocks = svf.blocks();
            has = svf.has(291810392, 2429);

            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            return TestResult(__PRETTY_FUNCTION__, test_name, result, err, time_exec.count(), svf.num_bytes());
        }

        TestResult test_debug_need_read_special_B() {
            SparseVirtualFile svf("", 0.0);
            std::string test_name(__FUNCTION__);
            static char data[4096]; // Random

            // Load the SVF
            auto blocks = svf.blocks();

            // Analyse the results
            int result = 0; // Success
            std::string err;
            // Run the test
            auto time_start = std::chrono::high_resolution_clock::now();
            auto need = svf.need(0, 32, 1);
            svf.write(0, data, 32);
            blocks = svf.blocks();
            bool has = svf.has(0, 32);
            result |= has;

            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            return TestResult(__PRETTY_FUNCTION__, test_name, result, err, time_exec.count(), svf.num_bytes());
        }

        /// This is special test created to check the problem reading TUPAC-TR-004.svs
        TestResult test_debug_need_read_special_C() {
            int result = 0; // Success
            SparseVirtualFile svf("", 0.0);
            std::string test_name(__FUNCTION__);
            static char data[1 << 16]; // 65536 greedy_length

            t_seek_reads needs;
            t_seek_reads blocks;
            std::string err;
            // Run the test
            auto time_start = std::chrono::high_resolution_clock::now();
            // Go through the sequence
            needs = svf.need(515'913'022, 6283);
            result |= needs.size() != 1;
            svf.write(515'913'022, data, 1 << 16);
            blocks = svf.blocks();
            result |= blocks.size() != 1;
            svf.read(515'913'022, 6283, data);
            // Second tile, already in the SVF.
            needs = svf.need(515'919'305, 5873);
            result |= needs.size() != 0;
            svf.read(515'919'305, 5873, data);
            // Third tile, not in the SVF.
            needs = svf.need(486'156'341, 6039);
            result |= needs.size() != 1;
            // This is where e go wrong, the two writes should be distinct but they are being coalesced.
            svf.write(486'156'341, data, 1 << 16);
            blocks = svf.blocks();
            result |= blocks.size() != 2;

            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            return TestResult(__PRETTY_FUNCTION__, test_name, result, err, time_exec.count(), svf.num_bytes());
        }

        TestCount test_block_size(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            TestCount count;
            SparseVirtualFile svf("", 0.0);
            svf.write(894, test_data_bytes_512, 22);
            auto time_start = std::chrono::high_resolution_clock::now();
            result |= svf.block_size(894) != 22;
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        TestCount test_block_size_throws(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            std::string test_error_message = "";
            TestCount count;
            SparseVirtualFile svf("", 0.0);
            svf.write(894, test_data_bytes_512, 22);
            auto time_start = std::chrono::high_resolution_clock::now();
            try {
                svf.block_size(895);
            } catch (Exceptions::ExceptionSparseVirtualFileRead &err) {
                std::string expected_message = "SparseVirtualFile::block_size(): Requested file position 895 is not at the start of a block";
                if (err.message() != expected_message) {
                    std::ostringstream os;
                    os << "Error message \"" << err.message() << "\" expected \"" << expected_message << "\"";
                    test_error_message = os.str();
                    result = 1;
                }
            }
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, test_error_message,
                                                time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        TestCount test_block_touch_single_block(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            TestCount count;
            SparseVirtualFile svf("", 0.0);
            auto time_start = std::chrono::high_resolution_clock::now();
            result |= svf.block_touch() != 0;
            // Write a block
            svf.write(894, test_data_bytes_512, 22);
            result |= svf.block_touch() != 1;

            t_block_touches block_touches = svf.block_touches();
            result |= block_touches.size() != 1;

            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        TestCount test_block_touch_single_block_read_updates(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            TestCount count;
            static char data[1 << 16]; // 65536 greedy_length

            SparseVirtualFile svf("", 0.0);
            auto time_start = std::chrono::high_resolution_clock::now();
            result |= svf.block_touch() != 0;
            // Write a block
            svf.write(894, test_data_bytes_512, 22);
            result |= svf.block_touch() != 1;

            t_block_touches block_touches = svf.block_touches();
            result |= block_touches.size() != 1;
            result |= block_touches.begin()->first != 0;
            result |= block_touches.begin()->second != 894;

            // Read from the block, touch should be incremented.
            svf.read(900, 4, data);
            result |= svf.block_touch() != 2;
            block_touches = svf.block_touches();
            result |= block_touches.size() != 1;
            result |= block_touches.begin()->first != 1;
            result |= block_touches.begin()->second != 894;

            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        TestCount test_block_touch_two_blocks(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            TestCount count;
            SparseVirtualFile svf("", 0.0);
            auto time_start = std::chrono::high_resolution_clock::now();
            result |= svf.block_touch() != 0;
            // Write a block
            svf.write(894, test_data_bytes_512, 22);
            result |= svf.block_touch() != 1;
            // Write a block that will NOT be coalesced.
            svf.write(1440, test_data_bytes_512, 4);
            result |= svf.block_touch() != 2;

            t_block_touches block_touches = svf.block_touches();
            result |= block_touches.size() != 2;

            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        // Write two blocks then coalesce them with another and check the block_touch
        TestCount test_block_touch_coalesced(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            TestCount count;
            SparseVirtualFile svf("", 0.0);
            auto time_start = std::chrono::high_resolution_clock::now();
            result |= svf.block_touch() != 0;
            // Write two blocks
            svf.write(0, test_data_bytes_512, 8);
            result |= svf.block_touch() != 1;
            svf.write(12, test_data_bytes_512, 12);
            result |= svf.block_touch() != 2;
            // Write a block that will coalesce all.
            svf.write(0 + 8, test_data_bytes_512, 4);
            result |= svf.block_touch() != 3;

            t_block_touches block_touches = svf.block_touches();
            result |= block_touches.size() != 1;

            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        TestCount test_lru_block_punting_a(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            TestCount count;
            SparseVirtualFile svf("", 0.0);
            // Populate the SVF.
            size_t block_size = 128;
            size_t block_count = 256;
            t_fpos file_positon = 0;
            for (size_t i = 0; i < block_count; ++i) {
                svf.write(file_positon, test_data_bytes_512, block_size);
                file_positon += block_size;
                // + 1 so not coalesced.
                file_positon += 1;
            }
            // Sanity check
            result |= svf.num_blocks() != block_count;
            result |= svf.num_bytes() != (block_count * block_size);
            // Is this clearer?
            result |= svf.num_blocks() == block_count ? 0 : 1 << 0;
            result |= svf.num_bytes() == (block_count * block_size) ? 0 : 1 << 1;

            auto time_start = std::chrono::high_resolution_clock::now();

            size_t cache_upper_bound = 1024;
            result |= svf.num_bytes() < cache_upper_bound;
            // Is this clearer?
            result |= svf.num_bytes() >= cache_upper_bound ? 0 : 1 << 2;
            // Punt blocks.
            if (svf.num_blocks() > 1 && svf.num_bytes() >= cache_upper_bound) {
                auto touch_fpos_map = svf.block_touches();
                for (const auto &iter: touch_fpos_map) {
                    if (svf.num_blocks() > 1 && svf.num_bytes() >= cache_upper_bound) {
                        svf.erase(iter.second);
                    } else {
                        break;
                    }
                }
            }
            result |= svf.num_bytes() >= cache_upper_bound;
            // Is this clearer?
            result |= svf.num_bytes() < cache_upper_bound ? 0 : 1 << 3;;

            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        TestCount test_lru_block_punting_b(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            TestCount count;
            SparseVirtualFile svf("", 0.0);
            // Populate the SVF.
            size_t block_size = 128;
            size_t block_count = 256;
            t_fpos file_position = 0;
            for (size_t i = 0; i < block_count; ++i) {
                svf.write(file_position, test_data_bytes_512, block_size);
                file_position += block_size;
                // + 1 so not coalesced.
                file_position += 1;
            }
            // Sanity check
            // Is this clearer?
            int error_bit = 0;
            result |= svf.num_blocks() == block_count ? 0 : 1 << error_bit;
            ++error_bit;
            result |= svf.num_bytes() == (block_count * block_size) ? 0 : 1 << error_bit;
            ++error_bit;

            auto time_start = std::chrono::high_resolution_clock::now();

            size_t cache_upper_bound = 1024;
//            auto num_bytes = svf.num_bytes();
            // Is this clearer?
            result |= svf.num_bytes() >= cache_upper_bound ? 0 : 1 << error_bit;
            ++error_bit;
            // Punt blocks.
            size_t punted = svf.lru_punt(cache_upper_bound);
//            fprintf(stdout, "XXX punted %zu\n", punted); // 31872
//            num_bytes = svf.num_bytes();
            result |= svf.num_bytes() == 7 * block_size ? 0 : 1 << error_bit;
            ++error_bit;
            result |= punted == (block_size * block_count - 896) ? 0 : 1 << error_bit;
            ++error_bit;

            // Is this clearer?
            result |= svf.num_bytes() < cache_upper_bound ? 0 : 1 << error_bit;
            ++error_bit;

            auto block_touches = svf.block_touches();

            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        // Special case where _write_append_new_to_old() was incrementing m_block_touch wrongly
        TestCount test_lru_block_punting_c(t_test_results &results) {
            TestCount count;
            int result_code = 0; // Success
            SparseVirtualFile svf("", 0.0);
            auto time_start = std::chrono::high_resolution_clock::now();
            svf.write(16, test_data_bytes_512, 8);
            // Add a new block that will be merged with the old one.
            svf.write(16 + 8, test_data_bytes_512, 8);

            int error_bit = 0;
            result_code |= svf.num_blocks() == 1 ? 0 : 1 << error_bit;
            ++error_bit;
            result_code |= svf.block_touch() == 2 ? 0 : 1 << error_bit;
            ++error_bit;
            auto block_touches = svf.block_touches();
            auto iter = block_touches.begin();
            result_code |= iter != block_touches.end() ? 0 : 1 << error_bit;
            ++error_bit;
            result_code |= iter->first == 1 ? 0 : 1 << error_bit;
            ++error_bit;
            result_code |= iter->second == 16 ? 0 : 1 << error_bit;
            ++error_bit;

            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            auto result = TestResult(__PRETTY_FUNCTION__, "Sim low level index", result_code, "", time_exec.count(),
                                     svf.num_bytes());
            count.add_result(result.result());
            results.push_back(result);
            return count;
        }


        // Test the basic operation of needs_many()
        TestCount test_needs_many_empty(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            int error_bit = 0;
            TestCount count;

            // Empty SVF
            SparseVirtualFile svf("", 0.0);
            t_seek_reads seek_reads = {
                    {0,   128},
                    {256, 512},
            };

            auto time_start = std::chrono::high_resolution_clock::now();
            t_seek_reads seek_reads_result = svf.need_many(seek_reads, 0);
            result |= seek_reads_result.size() == 2 ? 0 : 1 << error_bit;
            ++error_bit;
            result |= seek_reads_result[0].first == 0 ? 0 : 1 << error_bit;
            ++error_bit;
            result |= seek_reads_result[0].second == 128 ? 0 : 1 << error_bit;
            ++error_bit;
            result |= seek_reads_result[1].first == 256 ? 0 : 1 << error_bit;
            ++error_bit;
            result |= seek_reads_result[1].second == 512 ? 0 : 1 << error_bit;
            ++error_bit;
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        TestCount test_needs_many_empty_overlap(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            int error_bit = 0;
            TestCount count;

            // Empty SVF
            SparseVirtualFile svf("", 0.0);
            t_seek_reads seek_reads = {
                    {0,  128},
                    {64, 512},
            };

            auto time_start = std::chrono::high_resolution_clock::now();
            t_seek_reads seek_reads_result = svf.need_many(seek_reads, 0);
            result |= seek_reads_result.size() == 1 ? 0 : 1 << error_bit++;
            result |= seek_reads_result[0].first == 0 ? 0 : 1 << error_bit++;
            result |= seek_reads_result[0].second == 64 + 512 ? 0 : 1 << error_bit++;
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        TestCount test_needs_many_empty_greedy_length(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            int error_bit = 0;
            TestCount count;

            // Empty SVF
            SparseVirtualFile svf("", 0.0);
            t_seek_reads seek_reads = {
                    {0,   128},
                    {256, 512},
            };

            auto time_start = std::chrono::high_resolution_clock::now();
            t_seek_reads seek_reads_result = svf.need_many(seek_reads, 256);
            result |= seek_reads_result.size() == 1 ? 0 : 1 << error_bit++;
            result |= seek_reads_result[0].first == 0 ? 0 : 1 << error_bit++;
            result |= seek_reads_result[0].second == 256 + 512 ? 0 : 1 << error_bit++;
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        // Test need_many() when there is one existing block.
        TestCount test_needs_many_one_block(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            int error_bit = 0;
            TestCount count;

            SparseVirtualFile svf("", 0.0);
            svf.write(64, test_data_bytes_512, 128);
            t_seek_reads seek_reads = {
                    {0,  128},
                    {64, 256},
            };

            auto time_start = std::chrono::high_resolution_clock::now();
            t_seek_reads seek_reads_result = svf.need_many(seek_reads, 0);
            result |= seek_reads_result.size() == 2 ? 0 : 1 << error_bit;
            ++error_bit;
            result |= seek_reads_result[0].first == 0 ? 0 : 1 << error_bit;
            ++error_bit;
            result |= seek_reads_result[0].second == 64 ? 0 : 1 << error_bit;
            ++error_bit;
            result |= seek_reads_result[1].first == 64 + 128 ? 0 : 1 << error_bit;
            ++error_bit;
            result |= seek_reads_result[1].second == (64 + 256) - (64 + 128) ? 0 : 1 << error_bit;
            ++error_bit;
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        // Test need_many() when there is one existing block and a greedy length.
        TestCount test_needs_many_one_block_greedy(t_test_results &results) {
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            int error_bit = 0;
            TestCount count;

            SparseVirtualFile svf("", 0.0);
            svf.write(64, test_data_bytes_512, 128);
            t_seek_reads seek_reads = {
                    {0,  128},
                    {64, 256},
            };

            auto time_start = std::chrono::high_resolution_clock::now();
            t_seek_reads seek_reads_result = svf.need_many(seek_reads, 512);
            result |= seek_reads_result.size() == 1 ? 0 : 1 << error_bit;
            ++error_bit;
            result |= seek_reads_result[0].first == 0 ? 0 : 1 << error_bit;
            ++error_bit;
            result |= seek_reads_result[0].second == 512 ? 0 : 1 << error_bit;
            ++error_bit;
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }


        TestCount test_erase_updates_counters(t_test_results &results) {
            TestCount count;
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            SparseVirtualFile svf("", 0.0);

            auto time_start = std::chrono::high_resolution_clock::now();

            // Values should be zero
            result |= svf.blocks_erased() != 0;
            result |= svf.bytes_erased() != 0;

            svf.write(64, test_data_bytes_512, 128);

            // Values should be zero
            result |= svf.blocks_erased() != 0;
            result |= svf.bytes_erased() != 0;

            // Values should not be zero
            svf.erase(64);
            result |= svf.blocks_erased() != 1;
            result |= svf.bytes_erased() != 128;

            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        TestCount test_erase_updates_counters_not_punt(t_test_results &results) {
            TestCount count;
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            SparseVirtualFile svf("", 0.0);

            auto time_start = std::chrono::high_resolution_clock::now();

            // Values should be zero
            result |= svf.blocks_punted() != 0;
            result |= svf.bytes_punted() != 0;

            svf.write(64, test_data_bytes_512, 128);

            // Values should be zero
            result |= svf.blocks_punted() != 0;
            result |= svf.bytes_punted() != 0;

            // Values should be zero as nothing got punted.
            svf.erase(64);
            result |= svf.blocks_punted() != 0;
            result |= svf.bytes_punted() != 0;

            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }

        TestCount test_punt_updates_counters(t_test_results &results) {
            TestCount count;
            std::string test_name(__FUNCTION__);
            int result = 0; // Success
            SparseVirtualFile svf("", 0.0);

            auto time_start = std::chrono::high_resolution_clock::now();

            // Values should be zero
            result |= svf.blocks_punted() != 0;
            result |= svf.bytes_punted() != 0;

            svf.write(64, test_data_bytes_512, 128);
            svf.write(512, test_data_bytes_512, 64);

            // Values should be zero
            result |= svf.blocks_punted() != 0;
            result |= svf.bytes_punted() != 0;
            result |= svf.num_blocks() != 2;

            // Now punt to upperbound of 256, should leave one block.
            svf.lru_punt(128 + 32);
            result |= svf.num_blocks() != 1;

            result |= svf.blocks_punted() != 1;
            result |= svf.bytes_punted() != 128;

            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
            TestResult test_result = TestResult(__PRETTY_FUNCTION__, test_name, result, "", time_exec.count(),
                                                svf.num_bytes());
            results.push_back(test_result);
            count.add_result(test_result.result());
            return count;
        }


#define INCLUDE_TESTS 1

        TestCount test_svf_all(t_test_results &results) {
#if INCLUDE_TESTS
            test_example_code();
#endif
#if INCLUDE_TESTS
            test_debug_need_read_special_A();
            test_debug_need_read_special_B();
            test_debug_need_read_special_C();
#endif
            TestCount count;
#if INCLUDE_TESTS
            // Write
            count += test_write_all(results);
            count += test_write_all_throws(results);
            // write() - performance
            count += test_perf_write_with_diff_check(results);
            count += test_perf_write_without_diff_check(results);
            count += test_perf_write_sim_index_svf(results);
            count += test_perf_write_1M_coalesced(results);
            count += test_perf_write_1M_uncoalesced(results);
            count += test_perf_write_1M_uncoalesced_size_of(results);
            // read()
            count += test_read_all(results);
            count += test_read_throws_all(results);
            count += test_perf_read_1M_un_coalesced(results);
            count += test_perf_read_1M_coalesced(results);
            // has()
            count += test_has_all(results);
            // need()
            count += test_need_all(results);
            count += test_perf_need_sim_index(results);
#endif
#if INCLUDE_TESTS
            count += test_need_greedy_all(results);
            // erase()
            count += test_erase_all(results);
            count += test_erase_throws_all(results);
            count += test_perf_erase_overwrite_false(results);
            count += test_perf_erase_overwrite_true(results);
#ifdef SVF_THREAD_SAFE
            count += test_write_multithreaded_coalesced(results);
            count += test_write_multithreaded_un_coalesced(results);
#endif
            count += test_block_size(results);
            count += test_block_size_throws(results);

            count += test_block_touch_single_block(results);
            count += test_block_touch_single_block_read_updates(results);
            count += test_block_touch_two_blocks(results);
            count += test_block_touch_coalesced(results);

#endif
#if INCLUDE_TESTS
            // Block punting example
            count += test_lru_block_punting_a(results);
            count += test_lru_block_punting_b(results);
            count += test_lru_block_punting_c(results);
#endif
#if INCLUDE_TESTS
            count += test_needs_many_empty(results);
            count += test_needs_many_empty_overlap(results);
            count += test_needs_many_empty_greedy_length(results);
            count += test_needs_many_one_block(results);
            count += test_needs_many_one_block_greedy(results);
#endif
#if INCLUDE_TESTS
            // erase() causes m_blocks_erased and m_bytes_erased to be updated.
            count += test_erase_updates_counters(results);
            count += test_erase_updates_counters_not_punt(results);
            count += test_punt_updates_counters(results);
#endif
            return count;
        }

    } // namespace Test
} // namespace SVFS
