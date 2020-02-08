//
// Created by Paul Ross on 2020-01-22.
//
#include <iomanip>
#include <sstream>
#include <thread>

#include "test_svf.h"

// Produced by, more or less:
// print('{\n    ' + ',\n    '.join([', '.join([f'0x{v + (16 * i):02x}' for v in range(16)]) for i in range(16)]) + '\n}')
// Imaginary file data, unique 256 bytes, unsigned char.
static const char data[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    -0x80, -0x7f, -0x7e, -0x7d, -0x7c, -0x7b, -0x7a, -0x79, -0x78, -0x77, -0x76, -0x75, -0x74, -0x73, -0x72, -0x71,
    -0x70, -0x6f, -0x6e, -0x6d, -0x6c, -0x6b, -0x6a, -0x69, -0x68, -0x67, -0x66, -0x65, -0x64, -0x63, -0x62, -0x61,
    -0x60, -0x5f, -0x5e, -0x5d, -0x5c, -0x5b, -0x5a, -0x59, -0x58, -0x57, -0x56, -0x55, -0x54, -0x53, -0x52, -0x51,
    -0x50, -0x4f, -0x4e, -0x4d, -0x4c, -0x4b, -0x4a, -0x49, -0x48, -0x47, -0x46, -0x45, -0x44, -0x43, -0x42, -0x41,
    -0x40, -0x3f, -0x3e, -0x3d, -0x3c, -0x3b, -0x3a, -0x39, -0x38, -0x37, -0x36, -0x35, -0x34, -0x33, -0x32, -0x31,
    -0x30, -0x2f, -0x2e, -0x2d, -0x2c, -0x2b, -0x2a, -0x29, -0x28, -0x27, -0x26, -0x25, -0x24, -0x23, -0x22, -0x21,
    -0x20, -0x1f, -0x1e, -0x1d, -0x1c, -0x1b, -0x1a, -0x19, -0x18, -0x17, -0x16, -0x15, -0x14, -0x13, -0x12, -0x11,
    -0x10, -0xf, -0xe, -0xd, -0xc, -0xb, -0xa, -0x9, -0x8, -0x7, -0x6, -0x5, -0x4, -0x3, -0x2, -0x1,
};


namespace SparseVirtualFileSystem {

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
            bytes_written += load_writes(svf, data);
        } catch (ExceptionSparseVirtualFile &err) {
            return TestResult(__FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
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
            return TestResult(__FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
        } else {
            for (size_t i = 0; i < m_expected_blocks.size(); ++i) {
                if (actual_blocks[i].first != m_expected_blocks[i].first) {
                    result = 1;
                    std::ostringstream os;
                    os << "In block " << i << " expected fpos " << m_expected_blocks[i].first;
                    os << " but got " << actual_blocks[i].first << " (other blocks not tested)";
                    err = os.str();
                    return TestResult(__FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
                }
                if (actual_blocks[i].second != m_expected_blocks[i].second) {
                    result = 1;
                    std::ostringstream os;
                    os << "In block " << i << " expected length " << m_expected_blocks[i].second;
                    os << " but got " << actual_blocks[i].second << " (other blocks not tested)";
                    err = os.str();
                    return TestResult(__FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
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
            return TestResult(__FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
        }
        // Overall bytes
        if (svf.num_bytes() != num_bytes) {
            result = 1;
            std::ostringstream os;
            os << "Found svf.num_bytes() " << svf.num_bytes() << " but expected " << num_bytes;
            err = os.str();
            return TestResult(__FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
        }
        // Write properties
        if (svf.count_write() != m_writes.size()) {
            result = 1;
            std::ostringstream os;
            os << "Found svf.count_write()" << svf.count_write() << " but expected " << m_writes.size();
            err = os.str();
            return TestResult(__FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
        }
        if (svf.bytes_write() != bytes_written) {
            result = 1;
            std::ostringstream os;
            os << "Found svf.bytes_write() " << svf.bytes_write() << " but expected " << bytes_written;
            err = os.str();
            return TestResult(__FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
        }
        // Read properties
        if (svf.count_read() != 0) {
            result = 1;
            std::ostringstream os;
            os << "Count of reads is " << svf.count_read() << " but should be 0";
            err = os.str();
            return TestResult(__FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
        }
        if (svf.bytes_read() != 0) {
            result = 1;
            std::ostringstream os;
            os << "Count of read bytes is " << svf.bytes_read() << " but should be 0";
            err = os.str();
            return TestResult(__FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
        }
        return TestResult(__FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
    }

    const std::vector<TestCaseWrite> write_test_cases = {
        {"Write no blocks", {}, {}},
        //        |+++|
        //
        //==== New is added to existing blocks: _write_append_new_to_old()
        {"Write single block", {{8, 4}}, {{8, 4}},},
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
        //
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
        {"New appends old[0] and [1] overlaped", {{8, 4}, {16, 4}, {7, 14}}, {{7, 14}},},
    };

    const std::vector<TestCaseWrite> write_test_cases_special = {
        //        ^==|
        //    |++|
        {"New appends old[0]", {{8, 4}, {4, 4}}, {{4, 8}},},
//        {"New appends old[0]", {{8, 4}, {4, 4}, {16, 4}, {32, 4}}, {{4, 8}},},
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
            load_writes(svf, data);
            svf.write(m_fpos, m_data, m_len);
            return TestResult(__FUNCTION__, m_test_name, 1, "Write test failed to throw.", 0.0, 0);
        } catch (ExceptionSparseVirtualFileWrite &err) {
            if (err.message() != m_message) {
                std::ostringstream os;
                os << "Error message \"" << err.message() << "\" expected \"" << m_message << "\"";
                return TestResult(__FUNCTION__, m_test_name, 1, os.str(), 0.0, svf.num_bytes());
            }
        }
        return TestResult(__FUNCTION__, m_test_name, 0, "", 0.0, svf.num_bytes());
    }

    const std::vector<TestCaseWriteThrows> write_test_cases_throws = {
        {
            "Throws: Overwrite single block", {{65, 4}}, 65, 4, data + 66,
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


    // Simulate writing a low level RP66V1 index. Total bytes written around 1Mb.
    // 23831 * (4 + 10 * 4) is close to 1Mb
    TestCount test_perf_sim_index(t_test_results &results) {
        TestCount count;
        SparseVirtualFileSystem::SparseVirtualFile svf("", 0.0);
        auto time_start = std::chrono::high_resolution_clock::now();

        for (size_t vr = 0; vr < 23831; ++vr) {
            t_fpos fpos = 80 + vr * 8004;
            svf.write(fpos, data, 4);
            fpos += 4;
            for (int lrsh = 0; lrsh < 10; ++lrsh) {
                svf.write(fpos, data, 4);
                fpos += 800;
            }
        }

        std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
        auto result = TestResult(__FUNCTION__, "Sim low level index", 0, "", time_exec.count(), svf.num_bytes());
        count.add_result(result.result());
        results.push_back(result);
        return count;
    }


    // Write 1Mb of data in different, equally sized, blocks that are all coalesced and report the time taken.
    // Essentially only one block is created and all the other data is appended.
    TestCount test_perf_1M_coalesced(t_test_results &results) {
        TestCount count;
        for (size_t block_size = 1; block_size <= 256; block_size *= 2) {
            SparseVirtualFileSystem::SparseVirtualFile svf("", 0.0);

            auto time_start = std::chrono::high_resolution_clock::now();
            for (t_fpos i = 0; i < (1024 * 1024 * 1) / block_size; ++i) {
                t_fpos fpos = i * block_size;
                svf.write(fpos, data, block_size);
            }
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

            std::ostringstream os;
            os << "1Mb, " << std::setw(3) << block_size << " sized blocks, coalesced";
            auto result =TestResult(__FUNCTION__, std::string(os.str()), 0, "", time_exec.count(), svf.num_bytes());
            count.add_result(result.result());
            results.push_back(result);
        }

        return count;
    }

    // Write 1Mb of data in different, equally sized, blocks that are not coalesced and report the time taken.
    // Each write creates a separate block.
    TestCount test_perf_1M_uncoalesced(t_test_results &results) {
        TestCount count;
        for (size_t block_size = 1; block_size <= 256; block_size *= 2) {
            SparseVirtualFileSystem::SparseVirtualFile svf("", 0.0);

            auto time_start = std::chrono::high_resolution_clock::now();
            for (t_fpos i = 0; i < (1024 * 1024 * 1) / block_size; ++i) {
                t_fpos fpos = i * block_size + i;
                svf.write(fpos, data, block_size);
            }
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

            std::ostringstream os;
            os << "1Mb, " << std::setw(3) << block_size << " sized blocks, uncoalesced";
            auto result =TestResult(__FUNCTION__, std::string(os.str()), 0, "", time_exec.count(), svf.num_bytes());
            count.add_result(result.result());
            results.push_back(result);
        }

        return count;
    }


    // Write 1Mb of data in different, equally sized, blocks that are not coalesced and report the memory usage
    // with size_of(). Each write creates a separate  block.
    // Typically 1 byte  blocks are x35 times the memory. 256 byte blocks are x1.2 the memory.
    TestCount test_perf_1M_uncoalesced_size_of(t_test_results &results) {
        TestCount count;
        for (size_t block_size = 1; block_size <= 256; block_size *= 2) {
            SparseVirtualFileSystem::SparseVirtualFile svf("", 0.0);

            auto time_start = std::chrono::high_resolution_clock::now();
            for (t_fpos i = 0; i < (1024 * 1024 * 1) / block_size; ++i) {
                t_fpos fpos = i * block_size + i;
                svf.write(fpos, data, block_size);
            }
            std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

            std::ostringstream os;
            os << "1Mb, " << std::setw(3) << block_size << " sized blocks";
            auto result =TestResult(__FUNCTION__, std::string(os.str()), 0, "", time_exec.count(), svf.size_of());
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
            load_writes(svf, data);
        } catch (ExceptionSparseVirtualFile &err) {
            return TestResult(__FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
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
            return TestResult(__FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
        }
        std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

        // Check the result
        for (size_t i = 0; i < m_len; ++i) {
            if (read_buffer[i] != data[m_fpos + i]) {
                result = 1;
                std::ostringstream os;
                os << "In position " << m_fpos + 1 << " expected fpos " << static_cast<int>(data[m_fpos + i]);
                os << " but got " << static_cast<int>(read_buffer[i]) << " (other data not tested)";
                err = os.str();
                return TestResult(__FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
            }
        }
        return TestResult(__FUNCTION__, m_test_name, result, err, time_exec.count(), svf.num_bytes());
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
            load_writes(svf, data);
        } catch (ExceptionSparseVirtualFile &err) {
            return TestResult(__FUNCTION__, m_test_name, 1, err.message(), 0.0, 0);
        }
        // Run the test.
        char read_buffer[256];
        try {
            svf.read(m_fpos, m_len, read_buffer);
            return TestResult(__FUNCTION__, m_test_name, 1, "Test failed to throw.", 0.0, 0);
        } catch (ExceptionSparseVirtualFileRead & err) {
            if (err.message() != m_message) {
                std::ostringstream os;
                os << "Error message \"" << err.message() << "\" expected \"" << m_message << "\"";
                return TestResult(__FUNCTION__, m_test_name, 1, os.str(), 0.0, svf.num_bytes());
            }
        }
        return TestResult(__FUNCTION__, m_test_name, 0, "", 0.0, svf.num_bytes());
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


    // Write 1Mb of data in different, equally sized, blocks that are all coalesced and report the time taken.
    // Essentially only one block is created and all the other data is appended.
    TestCount test_perf_read_1M_coalesced(t_test_results &results) {
        const size_t SIZE = 1024 * 1024 * 1;
        TestCount count;
        SparseVirtualFileSystem::SparseVirtualFile svf("", 0.0);
        for (t_fpos i = 0; i < (SIZE) / 256; ++i) {
            t_fpos fpos = i * 256;
            svf.write(fpos, data, 256);
        }

        char buffer[SIZE];
        auto time_start = std::chrono::high_resolution_clock::now();
        svf.read(0, SIZE, buffer);
        std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;

        std::ostringstream os;
        auto result =TestResult(__FUNCTION__, "1Mb of 256 bytes in one block", 0, "", time_exec.count(), svf.num_bytes());
        count.add_result(result.result());
        results.push_back(result);
        return count;
    }

#ifdef SVF_THREAD_SAFE
    SparseVirtualFileSystem::SparseVirtualFile g_svf_multithreaded("", 0.0);

    // This writes to the global SVF and is used by test_write_multithreaded_num_threads in multiple threads.
    void _write_multithreaded() {
        try {
            for (size_t vr = 0; vr < 23831; ++vr) {
                t_fpos fpos = 80 + vr * 8004;
                g_svf_multithreaded.write(fpos, data, 4);
                fpos += 4;
                for (int lrsh = 0; lrsh < 10; ++lrsh) {
                    g_svf_multithreaded.write(fpos, data, 4);
                    fpos += 800;
                }
            }
        }
        catch (SparseVirtualFileSystem::ExceptionSparseVirtualFile &err) {
            std::cout << "_write_multithreaded(): Fails: " << err.message() << std::endl;
        }
    }

    // Launches num_threads threads and writes to a global SVF in the manner of test_perf_sim_index()
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
        auto result = TestResult(__FUNCTION__, os.str(), 0, "", time_exec.count(), work_done);
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
        count += test_perf_sim_index(results);
        count += test_perf_1M_coalesced(results);
        count += test_perf_1M_uncoalesced(results);
        count += test_perf_1M_uncoalesced_size_of(results);
        // Read
        count += test_read_all(results);
        count += test_read_throws_all(results);
        count += test_perf_read_1M_coalesced(results);

#ifdef SVF_THREAD_SAFE
        count += test_write_multithreaded(results);
#endif
        return count;
    }

} // namespace SparseVirtualFileSystem