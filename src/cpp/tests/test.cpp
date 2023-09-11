/** @file
 *
 * A Sparse Virtual File implementation.
 *
 * Created by Paul Ross on 2020-01-22.
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

#include <iomanip>

#include "test.h"
#include "SaveStreamState.h"


/**
 * Test data.
 *
 * Produced by, more or less:
 *
 * @code
 * print('{\n    ' + ',\n    '.join([', '.join([f'0x{v + (16 * i):02x}' for v in range(16)]) for i in range(16)]) + '\n}')
 * @endcode
 *
 * Imaginary file test_data_bytes_512, unique 512 bytes, unsigned char.
 */
const char test_data_bytes_512[] = {
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

/**
 * Write out all the tests results.
 *
 * @param results The tests results.
 * @param os The stream to write to.
 */
void write_test_results(const t_test_results &results, std::ostream &os) {
    SaveStreamState state(os);

    // Header
    os << std::left;
    os << std::setw(75) << "Function";
    os << "----";
    os << std::right;
    os << std::setw(16) << "Bytes";
    os << std::setw(16) << "ms";
    os << std::setw(16) << "ms/Mb";
    os << std::setw(16) << "Mb/s";
    os << " Test";
    os << std::endl;
    size_t failures = 0;
    for (const auto &result: results) {
        os << std::left;
        os << std::setw(75) << result.function();
        if (result.result() == 0) {
            os << "pass";
        } else {
            os << "FAIL";
            failures++;
        }
        os << std::right << std::fixed;
        os << std::setw(16) << result.work_bytes();
        os << std::setw(16) << std::setprecision(6) << 1000 * result.exec_time();
        os << std::setw(16) << std::setprecision(3) << result.ms_per_mb();
        os << std::setw(16) << std::setprecision(1) << result.work_rate() / (1024 * 1024);
        if (result.has_error_message()) {
            os << " " << result.error_message();
        }
        os << " " << result.test();
        os << std::endl;
    }
    if (failures) {
        os << "Failed tests:" << std::endl;
        for (const auto &result: results) {
            if (result.result() != 0) {
                os << result.function();
                os << " Name: " << result.test();
                os << " Message: " << result.error_message();
                os << std::endl;
            }
        }
    }
}
