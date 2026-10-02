//
// Created by dlllibstdntc on 01.10.2026.
//

#ifndef MYVM_VALUE_H
#define MYVM_VALUE_H

#include <share/utility/base_types.h>

namespace share {
    struct Obj;

    struct Value {
        enum type : u1 {
            nil = 0,
            int_ = 1,
            float_ = 2,
            char_ = 3,
            obj = 4
        };

        union payload {
            i1 c;
            i8 i;
            f8 f;
            Obj* o;
            u8 raw;

            payload() : raw(0) {}
        };


        type m_type;
        payload m_payload;

        Value() noexcept : m_type(nil) {
            m_payload.raw = 0;
        };

        static Value Null() noexcept;
        static Value Integer(i8 value) noexcept;
        static Value Float(f8 value) noexcept;
        static Value Char(i1 value) noexcept;
        static Value Obj(Obj* value) noexcept;

        [[nodiscard]] bool isNull() const noexcept { return m_type == nil; }
        [[nodiscard]] bool isChar() const noexcept { return m_type == char_; }
        [[nodiscard]] bool isInt() const noexcept { return m_type == int_; }
        [[nodiscard]] bool isFloat() const noexcept { return m_type == float_; }
        [[nodiscard]] bool isObj() const noexcept { return m_type == obj; }
        [[nodiscard]] bool isNumber() const noexcept { return isInt() || isFloat(); }
        [[nodiscard]] char asChar() const noexcept { return m_payload.c; }
        [[nodiscard]] f8 asFloat() const noexcept { return m_payload.f; }
        [[nodiscard]] i8 asInt() const noexcept { return m_payload.i; }
        [[nodiscard]] share::Obj* asObj() const noexcept { return m_payload.o; }

        [[nodiscard]] f8 toFloat() const noexcept {
            if (isInt()) return static_cast<f8>(m_payload.i);
            if (isFloat()) return m_payload.f;
            if (isChar()) return m_payload.c;

            return 0.0;
        }

        [[nodiscard]] i8 toInt() const noexcept {
            if (isInt()) return m_payload.i;
            if (isFloat()) return static_cast<i8>(m_payload.f);
            if (isChar()) return m_payload.c;

            return 0;
        }

        [[nodiscard]] i1 toChar() const noexcept {
            if (isInt()) return static_cast<i1>(m_payload.i);
            if (isFloat()) return static_cast<i1>(m_payload.f);
            if (isChar()) return m_payload.c;

            return 0;
        }

        bool operator!=(const Value & other) const noexcept { return not (*this == other); }

        bool operator==(const Value & other) const noexcept {
            if (m_type != other.m_type) {
                if (isNumber()
                    && other.isNumber()) return toFloat() == other.toFloat();
                return false;
            }
            switch (m_type) {
                case nil:
                    return true;
                    break;
                case int_:
                    return m_payload.i == other.m_payload.i;
                    break;
                case float_:
                    return m_payload.f == other.m_payload.f;
                    break;
                case char_:
                    return m_payload.c == other.m_payload.c;
                    break;
                case obj:
                    return m_payload.o == other.m_payload.o;
                    break;
            }
            return false;
        }
    };

    inline Value Value::Null() noexcept { return {}; }

    inline Value Value::Integer(const i8 value) noexcept {
        Value x;
        x.m_payload.i = value;
        x.m_type = int_;

        return x;
    }

    inline Value Value::Float(const f8 value) noexcept {
        Value x;
        x.m_payload.f = value;
        x.m_type = float_;

        return x;
    }

    inline Value Value::Char(const i1 value) noexcept {
        Value x;
        x.m_payload.c = value;
        x.m_type = char_;

        return x;
    }

    inline Value Value::Obj(share::Obj *value) noexcept {
        Value x;
        x.m_payload.o = value;
        x.m_type = obj;

        return x;
    }
}

#endif //MYVM_VALUE_H
