//
// Created by dlllibstdntc on 01.10.2026.
//

#ifndef MYVM_CALL_FRAME_H
#define MYVM_CALL_FRAME_H

#include <share/utility/base_types.h>
#include <share/utility/chunk.h>
#include <share/utility/obj.h>

namespace share {
    struct CallFrame {
        ObjFunc*      func;
        const Chunk*  chunk;
        const u1*     ip;
        u8            stackBase;
        u8            slotCount;

        [[nodiscard]] u8 offset() const noexcept {
            return static_cast<u8>(ip - chunk->m_code.data());
        }

        [[nodiscard]] i4 line() const noexcept {
            const auto off = offset();
            return off < chunk->m_lines.size() ? chunk->m_lines[off] : 0;
        }
    };
}

#endif //MYVM_CALL_FRAME_H
