/** @file
 *
 * Save and restore stream state.
 *
 * Created by Paul Ross on 2020-01-24.
 *
 * @verbatim
    MIT License

    Copyright (c) 2023 Paul Ross

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

#ifndef CPPSVF_SAVESTREAMSTATE_H
#define CPPSVF_SAVESTREAMSTATE_H

#include <iostream>

/**
 * @brief Class that saves the current iostream state and restores it on destruction.
 */
class SaveStreamState
{
public:
    /** Constructor that takes an existing stream. */
    explicit SaveStreamState (std::ios& stream)
            : m_stream_ref (stream),
              m_stream_flags (stream.flags()),
              m_stream_precision (stream.precision()),
              m_stream_fill (stream.fill())
    {
        // FIX: 2002-08-30 When the stream state is saved
        // it also clears any prior flags such as 'fixed' etc.
        // Uses the default ctor for std::ios_base::fmtflags
        stream.flags(std::ios_base::fmtflags());
    }
    /** Destructor restores the stream to its previous state. */
    virtual ~SaveStreamState() {
        try {
            if (m_stream_ref) {
                m_stream_ref.flags(m_stream_flags);
                m_stream_ref.precision(m_stream_precision);
                m_stream_ref.fill(m_stream_fill);
            }
        }
        catch (...) {}
    }
protected:
    // Protected members
    std::ios&				m_stream_ref;
    std::ios_base::fmtflags	m_stream_flags;
    std::streamsize			m_stream_precision;
    char					m_stream_fill;
private:
};

#endif //CPPSVF_SAVESTREAMSTATE_H
