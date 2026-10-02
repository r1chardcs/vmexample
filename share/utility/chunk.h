//
// Created by dlllibstdntc on 01.10.2026.
//

#ifndef MYVM_CHUNK_H
#define MYVM_CHUNK_H

#include <share/utility/value.h>
#include <cstdint>
#include <vector>

namespace share {
    struct Chunk {
        std::vector<u1> m_code;
        std::vector<i4> m_lines;
        std::vector<Value> m_constants;

        u4 addConstant(Value v) {
            m_constants.emplace_back(v);
            return static_cast<u4>(m_constants.size() - 1);
        }
        void emit(const uint8_t b, const i4 line = 0) {
            m_code.push_back(b);
            m_lines.push_back(line);
        }
        void emitU32(const u4 v, const i4 line = 0) {
            m_code.push_back(v & 0xFF); m_code.push_back((v >> 8) & 0xFF);
            m_code.push_back((v >> 16) & 0xFF); m_code.push_back((v >> 24) & 0xFF);
            for (int i = 0; i < 4; ++i) m_lines.push_back(line);
        }
    };
}

#endif //MYVM_CHUNK_H
