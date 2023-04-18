//
// Created by Paul Ross on 2020-01-22.
//
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>

#include "test_svf.h"

namespace SVFS {

    TestCaseWrite::TestCaseWrite(const std::string &m_test_name, const t_seek_read &m_writes,
                                 const t_seek_read &m_expected_blocks) : TestCaseABC(m_test_name, m_writes),
                                                                         m_expected_blocks(m_expected_blocks) {}

    // Create a SVF, run the write tests and report the result.
    TestResult TestCaseWrite::run() const {
        SparseVirtualFile svf("", 0.0);

        // Run the test
        size_t bytes_written = 0;
        auto time_start = std::chrono::high_resolution_clock::now();
        try {
            bytes_written += load_writes(svf, test_data_bytes_512);
        } catch (ExceptionSparseVirtualFile &err) {
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
            os << "Expected " << m_expected_blocks.size() << " blocks but got " << actual_blocks.size() << " blocks";
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
                    return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
                }
                if (actual_blocks[i].second != m_expected_blocks[i].second) {
                    result = 1;
                    std::ostringstream os;
                    os << "In block " << i << " expected length " << m_expected_blocks[i].second;
                    os << " but got " << actual_blocks[i].second << " (other blocks not tested)";
                    err = os.str();
                    return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
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

    const std::vector<TestCaseWrite> write_test_cases = {
        //
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

        //        ^==|    |==|
        {"Add second block", {{8, 4}, {16, 4}}, {{8, 4}, {16, 4}},},

        //        ^==|    |==|
        //          |++++++|
        {"New joins two blocks", {{8, 4}, {16, 4}, {10, 8}}, {{8, 12}},},

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
        for (const auto& test_case: write_test_cases) {

//        for (const auto& test_case: write_test_cases_special) {
//            std::cout << "Testing: " << test_case.test_name() << std::endl;

            auto result = test_case.run();
            count.add_result(result.result());
            results.push_back(result);
        }
        return count;
    }


    TestCaseWriteThrows::TestCaseWriteThrows(const std::string &m_test_name, const t_seek_read &m_writes,
                                             t_fpos fpos, size_t len, const char *data, const std::string &message) : TestCaseABC(m_test_name,
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
        } catch (ExceptionSparseVirtualFileWrite &err) {
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
        for (const auto& test_case: write_test_cases_throws) {
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
        auto result =TestResult(__PRETTY_FUNCTION__, std::string(os.str()), 0, "", time_exec.count(), repeat * block_size);
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
        auto result = TestResult(__PRETTY_FUNCTION__, "Sim low level index", 0, "", time_exec.count(), svf.num_bytes());
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
            auto result =TestResult(__PRETTY_FUNCTION__, std::string(os.str()), 0, "", time_exec.count(), svf.num_bytes());
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
            auto result =TestResult(__PRETTY_FUNCTION__, std::string(os.str()), 0, "", time_exec.count(), svf.num_bytes());
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

            auto time_start = std::chrono::high_resolution_clock::now();
            for (t_fpos i = 0; i < (1024 * 1024 * 1) / block_size; ++i) {
                t_fpos fpos = i * block_size + i;
                svf.write(fpos, test_data_bytes_512, block_size);
            }
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

            std::ostringstream os;
            os << "1Mb, " << std::setw(3) << block_size << " sized blocks";
            auto result =TestResult(__PRETTY_FUNCTION__, std::string(os.str()), 0, "", time_exec.count(), svf.size_of());
            count.add_result(result.result());
            results.push_back(result);
        }

        return count;
    }


    TestCaseRead::TestCaseRead(const std::string &m_test_name, const t_seek_read &m_writes,
                               t_fpos fpos, size_t len) : TestCaseABC(m_test_name, m_writes),
                                                          m_fpos(fpos), m_len(len) {}


    // Create a SVF, run the read tests and report the result.
    TestResult TestCaseRead::run() const {
        SparseVirtualFile svf("", 0.0);

        // Load the SVF
        try {
            load_writes(svf, test_data_bytes_512);
        } catch (ExceptionSparseVirtualFile &err) {
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
        } catch (ExceptionSparseVirtualFileRead & err) {
            return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
        }
        std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

        // Check the result
        for (size_t i = 0; i < m_len; ++i) {
            if (read_buffer[i] != test_data_bytes_512[m_fpos + i]) {
                result = 1;
                std::ostringstream os;
                os << "In position " << m_fpos + 1 << " expected fpos " << static_cast<int>(test_data_bytes_512[m_fpos + i]);
                os << " but got " << static_cast<int>(read_buffer[i]) << " (other test_data_bytes_512 not tested)";
                err = os.str();
                return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
            }
        }
        return TestResult(__PRETTY_FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
    }

    const std::vector<TestCaseRead> read_test_cases = {
            //        ^==|
            //        |++|
            {"Read exactly a block", {{8, 4}}, 8, 4},
            //        ^==|
            //        |+|
            {"Read leading part of block", {{8, 4}}, 8, 3},
            //        ^==|
            //         |+|
            {"Read trailing part of block", {{8, 4}}, 9, 3},
            //        ^==|
            //         ||
            {"Read mid part of block", {{8, 4}}, 9, 2},
    };


    const std::vector<TestCaseRead> read_test_cases_special = {
            //        ^==|
            //         |+|
            {"Read trailing part of block", {{8, 4}}, 9, 3},
            //        ^==|
            //         ||
            {"Read mid part of block", {{8, 4}}, 9, 2},
    };


    TestCount test_read_all(t_test_results &results) {
        TestCount count;
        for (const auto& test_case: read_test_cases) {
//        for (const auto& test_case: read_test_cases_special) {
//            std::cout << "Testing: " << test_case.test_name() << std::endl;
            auto result = test_case.run();
            count.add_result(result.result());
            results.push_back(result);
        }
        return count;
    }


    TestCaseReadThrows::TestCaseReadThrows(const std::string &m_test_name, const t_seek_read &m_writes,
                                           t_fpos fpos, size_t len, const std::string &message) : TestCaseRead(
            m_test_name,
            m_writes, fpos,
            len),
                                                                                                  m_message(message) {}


    // Create a SVF, run the read tests and report the result.
    TestResult TestCaseReadThrows::run() const {
        SparseVirtualFile svf("", 0.0);

        // Load the SVF
        try {
            load_writes(svf, test_data_bytes_512);
        } catch (ExceptionSparseVirtualFile &err) {
            return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
        }
        // Run the test.
        char read_buffer[256];
        try {
            svf.read(m_fpos, m_len, read_buffer);
            return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, "Test failed to throw.", 0.0, 0);
        } catch (ExceptionSparseVirtualFileRead & err) {
            if (err.message() != m_message) {
                std::ostringstream os;
                os << "Error message \"" << err.message() << "\" expected \"" << m_message << "\"";
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, os.str(), 0.0, svf.num_bytes());
            }
        }
        return TestResult(__PRETTY_FUNCTION__, m_test_name, 0, "", 0.0, svf.num_bytes());
    }


    const std::vector<TestCaseReadThrows> read_test_cases_throw = {
            {"Read empty SVF throws",    {},       8, 4, "SparseVirtualFile::read(): Sparse virtual file is empty."},
            //        ^==|
            //  |++|
            {"Read before block throws", {{8, 4}}, 2, 4,
             "SparseVirtualFile::read(): Requested file position 2 precedes first block at 8"},
            //        ^==|
            //       |++|
            {"Read prior to block throws", {{8, 4}}, 7, 4,
             "SparseVirtualFile::read(): Requested file position 7 precedes first block at 8"},
            //        ^==|
            //         |++|
            {"Read beyond block throws", {{8, 4}}, 9, 4,
             "SparseVirtualFile::read(): Requested position 9 and length 4 overruns block at 8 of size 4"},
            //        ^==|
            //             |++|
            {"Read beyond end throws", {{8, 4}}, 12, 4,
             "SparseVirtualFile::read(): Requested position 12 and length 4 overruns block at 8 of size 4"},
    };


    TestCount test_read_throws_all(t_test_results &results) {
        TestCount count;
        for (const auto& test_case: read_test_cases_throw) {
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
        auto result =TestResult(__PRETTY_FUNCTION__, "1Mb of 256 bytes in one block", 0, "", time_exec.count(), svf.num_bytes());
        count.add_result(result.result());
        results.push_back(result);
        return count;
    }


    TestCaseHas::TestCaseHas(const std::string &m_test_name, const t_seek_read &m_writes,
                               t_fpos fpos, size_t len, bool expected) : TestCaseABC(m_test_name, m_writes),
                                                          m_fpos(fpos), m_len(len), m_expected(expected) {}


    // Create a SVF, run the read tests and report the result.
    TestResult TestCaseHas::run() const {
        SparseVirtualFile svf("", 0.0);

        // Load the SVF
        try {
            load_writes(svf, test_data_bytes_512);
        } catch (ExceptionSparseVirtualFile &err) {
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
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, os.str(), time_exec.count(), svf.num_bytes());
            }
            return TestResult(__PRETTY_FUNCTION__, m_test_name, 0, "", time_exec.count(), svf.num_bytes());
        } catch (ExceptionSparseVirtualFileRead & err) {
            return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
        }
    }

    const std::vector<TestCaseHas> has_test_cases = {
            //
            //        |++|
            {"Has empty - false", {}, 8, 4, false},
            //        ^==|
            //        |++|
            {"Has an exact block", {{8, 4}}, 8, 4, true},
            //        ^==|
            //        |+|
            {"Has leading block", {{8, 4}}, 8, 3, true},
            //        ^==|
            //         |+|
            {"Has trailing block", {{8, 4}}, 9, 3, true},
            //        ^==|
            //         ||
            {"Has mid block", {{8, 4}}, 9, 2, true},
            //        ^==|
            //       |++|
            {"Not has an exact block -1", {{8, 4}}, 7, 4, false},
            //        ^==|
            //         |++|
            {"Not has an exact block +1", {{8, 4}}, 9, 4, false},
    };


    TestCount test_has_all(t_test_results &results) {
        TestCount count;
        for (const auto& test_case: has_test_cases) {
            auto result = test_case.run();
            count.add_result(result.result());
            results.push_back(result);
        }
        return count;
    }

    TestCaseNeed::TestCaseNeed(const std::string &m_test_name, const t_seek_read &m_writes,
                               t_fpos fpos, size_t len, const t_seek_read &m_need) : TestCaseABC(m_test_name, m_writes),
                                                                                     m_fpos(fpos), m_len(len),
                                                                                     m_need(m_need) {}


    // Create a SVF, run the read tests and report the result.
    TestResult TestCaseNeed::run() const {
        SparseVirtualFile svf("", 0.0);

        // Load the SVF
        try {
            load_writes(svf, test_data_bytes_512);
        } catch (ExceptionSparseVirtualFile &err) {
            return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
        }

        // Analyse the results
        // Run the test
        auto time_start = std::chrono::high_resolution_clock::now();
        t_seek_read need = svf.need(m_fpos, m_len);
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
                os << "In position " << i << " expected fpos " << m_need[i].first << " and len " << m_need[i].second;
                os << " but got fpos " << need[i].first << " and len " << need[i].second;
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, os.str(), time_exec.count(), svf.num_bytes());
            }
        }
        return TestResult(__PRETTY_FUNCTION__, m_test_name, 0, "", time_exec.count(), svf.num_bytes());
    }

    const std::vector<TestCaseNeed> need_test_cases = {
        //
        //        |++|
        {"Need on empty SVF", {}, 8, 4, {{8, 4}}},
        //        ^==|
        //        |++|
        {"Exactly one block", {{8, 4}}, 8,  4, {},},
        //        ^==|
        //         ||
        {"Inside one block", {{8, 4}}, 9,  2, {},},
        //        ^==|
        //    |++|
        {"All before one block", {{8, 4}}, 4,  4, {{4, 4}},},
        //        ^==|
        //            |++|
        {"All after one block", {{8, 4}}, 12,  4, {{12, 4}},},
        //        ^==|
        //    |+++++|
        {"Before and part of one block", {{8, 4}}, 4,  7, {{4, 4}},},
        //        ^==|
        //    |++++++|
        {"Before and all of one block", {{8, 4}}, 4,  8, {{4, 4}},},
        //        ^==|
        //    |+++++++|
        {"Before, all and after one block", {{8, 4}}, 4,  9, {{4, 4}, {12, 1}},},
        //        |==|  |==|
        //        |++++++++|
        {"Two blocks and in between (a)", {{8, 4}, {14, 4}}, 8,  10, {{12, 2}},},
        //        |==|  |==|
        //        |+++++++|
        {"Two blocks and in between (b)", {{8, 4}, {14, 4}}, 8,  9, {{12, 2}},},
        //        |==|  |==|
        //         |+++++++|
        {"Two blocks and in between (c)", {{8, 4}, {14, 4}}, 9,  9, {{12, 2}},},
        //        |==|  |==|
        //         |++++++|
        {"Two blocks and in between (d)", {{8, 4}, {14, 4}}, 9,  7, {{12, 2}},},
        //        |==|  |==|
        //       |++++++++|
        {"Two blocks, under-run", {{8, 4}, {14, 4}}, 7,  11, {{7, 1}, {12, 2}},},
        //        |==|  |==|
        //        |+++++++++|
        {"Two blocks, over-run", {{8, 4}, {14, 4}}, 8,  11, {{12, 2}, {18, 1}},},
        //        |==|  |==|
        //       |++++++++++|
        {"Two blocks, under/over-run", {{8, 4}, {14, 4}}, 7,  12, {{7, 1}, {12, 2}, {18, 1}},},
    };

    const std::vector<TestCaseNeed> need_test_cases_special = {
            //        |==|  |==|
            //        |++++++++|
            {"Two blocks and in between (a)", {{8, 4}, {14, 4}}, 8,  10, {{12, 2}},},
    };

    TestCount test_need_all(t_test_results &results) {
        TestCount count;
        for (const auto& test_case: need_test_cases) {
//        for (const auto& test_case: need_test_cases_special) {
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

#pragma mark - Test erase()
    TestCaseErase::TestCaseErase(const std::string &m_test_name, const t_seek_read &m_writes,
                               t_fpos fpos) : TestCaseABC(m_test_name, m_writes),
                                                          m_fpos(fpos) {}


    // Create a SVF, run the read tests and report the result.
    TestResult TestCaseErase::run() const {
        SparseVirtualFile svf("", 0.0);

        // Load the SVF
        try {
            load_writes(svf, test_data_bytes_512);
        } catch (ExceptionSparseVirtualFile &err) {
            return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
        }

        // Analyse the results
        int result = 0;
        std::string err;

        // Run the test
        auto time_start = std::chrono::high_resolution_clock::now();
        try {
            svf.erase(m_fpos);
        } catch (ExceptionSparseVirtualFileRead & err) {
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
        for (const auto& test_case: erase_test_cases) {
//        for (const auto& test_case: read_test_cases_special) {
//            std::cout << "Testing: " << test_case.test_name() << std::endl;
            auto result = test_case.run();
            count.add_result(result.result());
            results.push_back(result);
        }
        return count;
    }

    TestCaseEraseThrows::TestCaseEraseThrows(const std::string &m_test_name, const t_seek_read &m_writes,
                                             t_fpos fpos, const std::string &message) : TestCaseErase(m_test_name,
                                                                                                      m_writes, fpos),
                                                                                        m_message(message) {}

    // Create a SVF, run the read tests and report the result.
    TestResult TestCaseEraseThrows::run() const {
        SparseVirtualFile svf("", 0.0);

        // Load the SVF
        try {
            load_writes(svf, test_data_bytes_512);
        } catch (ExceptionSparseVirtualFile &err) {
            return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
        }
        // Run the test.
        try {
            svf.erase(m_fpos);
            return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, "Test failed to throw.", 0.0, 0);
        } catch (ExceptionSparseVirtualFileErase & err) {
            if (err.message() != m_message) {
                std::ostringstream os;
                os << "Error message \"" << err.message() << "\" expected \"" << m_message << "\"";
                return TestResult(__PRETTY_FUNCTION__, m_test_name, 1, os.str(), 0.0, svf.num_bytes());
            }
        }
        return TestResult(__PRETTY_FUNCTION__, m_test_name, 0, "", 0.0, svf.num_bytes());
    }


    const std::vector<TestCaseEraseThrows> erase_test_cases_throw = {
            {"Erase empty SVF throws",      {},       8, "SparseVirtualFile::erase(): Non-existent file position 8."},
            //        ^==|
            //  |++|
            {"Erase before block throws",   {{8, 4}}, 2,
                                                        "SparseVirtualFile::erase(): Non-existent file position 2."},
            //        ^==|
            //       |++|
            {"Erase within a block throws", {{8, 4}}, 9,
                                                        "SparseVirtualFile::erase(): Non-existent file position 9."},
            //        ^==|
            //             |++|
            {"Erase beyond end throws",     {{8, 4}}, 12,
                                                        "SparseVirtualFile::erase(): Non-existent file position 12."},
    };


    TestCount test_erase_throws_all(t_test_results &results) {
        TestCount count;
        for (const auto& test_case: erase_test_cases_throw) {
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
        auto result =TestResult(__PRETTY_FUNCTION__, std::string(os.str()), 0, "", time_total, total_size);
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
    void _write_multithreaded() {
        try {
            for (size_t vr = 0; vr < 23831; ++vr) {
                t_fpos fpos = 80 + vr * 8004;
                g_svf_multithreaded.write(fpos, test_data_bytes_512, 4);
                fpos += 4;
                for (int lrsh = 0; lrsh < 10; ++lrsh) {
                    g_svf_multithreaded.write(fpos, test_data_bytes_512, 4);
                    fpos += 800;
                }
            }
        }
        catch (ExceptionSparseVirtualFile &err) {
            std::cout << "_write_multithreaded(): Fails: " << err.message() << std::endl;
        }
    }

    // Launches num_threads threads and writes to a global SVF in the manner of test_perf_write_sim_index()
    TestCount test_write_multithreaded_n(int num_threads, t_test_results &results) {
        TestCount count;
        std::vector<std::thread> threads;
        g_svf_multithreaded.clear();

        // Timed section
        auto time_start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_threads; ++i) {
            threads.push_back(std::thread(_write_multithreaded));
        }
        for (size_t i = 0; i < threads.size(); ++i) {
            threads[i].join();
        }
        std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
        // END: Timed section

//        std::cout << "All wrote " << g_svf_multithreaded.num_bytes() << std::endl;
        size_t work_done = num_threads * g_svf_multithreaded.num_bytes();
        g_svf_multithreaded.clear();

        std::ostringstream os;
        os << "Multi threaded write [" << num_threads << "]";
        auto result = TestResult(__PRETTY_FUNCTION__, os.str(), 0, "", time_exec.count(), work_done);
        count.add_result(result.result());
        results.push_back(result);
        return count;
    }

    TestCount test_write_multithreaded(t_test_results &results) {
        TestCount count;
        for (int i = 1; i < 1 << 8; i *=2) {
            count += test_write_multithreaded_n(i, results);
        }
        return count;
    }

#endif

    TestCount test_svf_all(t_test_results &results) {
        TestCount count;
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
        count += test_perf_read_1M_coalesced(results);
        // has()
        count += test_has_all(results);
        // need()
        count += test_need_all(results);
        count += test_perf_need_sim_index(results);
        // erase()
        count += test_erase_all(results);
        count += test_erase_throws_all(results);
        count += test_perf_erase_overwrite_false(results);
        count += test_perf_erase_overwrite_true(results);
#ifdef SVF_THREAD_SAFE
        count += test_write_multithreaded(results);
#endif
        return count;
    }

} // namespace SVFS
