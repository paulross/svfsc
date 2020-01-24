//
// Created by Paul Ross on 2020-01-24.
//

#ifndef CPPSVF_SAVESTREAMSTATE_H
#define CPPSVF_SAVESTREAMSTATE_H

#include <iostream>

class SaveStreamState
{
public:
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
