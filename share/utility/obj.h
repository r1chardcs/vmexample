//
// Created by dlllibstdntc on 01.10.2026.
//

#ifndef MYVM_OBJ_H
#define MYVM_OBJ_H

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <share/utility/base_types.h>

#include "chunk.h"
#include "hash_util.h"
#include "value.h"

namespace share {
    class Obj {
    public:
        enum type : u1 {
            STRING,
            ARRAY,
            FUNC,
            NATIVE,
            CLASS,
            INSTANCE,
            METHOD,
            UPVALUE
        };

        bool m_marked;
        type m_type;
        Obj* m_next;

        explicit Obj(type type) : m_type(type) {
            m_next = nullptr;
            m_marked = false;
        }
        virtual ~Obj() = default;

        Obj(const Obj&) = delete;
        Obj& operator=(const Obj &) = delete;
    };

    template <typename T>
    struct ObjTag;

    #define NEW_OBJ_TAG(Class, EnumVal)                 \
        template <> struct ObjTag<Class>                \
        {                                               \
            static constexpr auto value = Obj::type::EnumVal;   \
        }

    inline bool isType(const Obj* obj, Obj::type type) { return obj && obj->m_type == type; }


    struct ObjString : Obj {
        std::wstring m_str;
        u8 m_hash;

        explicit ObjString(std::wstring str) : Obj(STRING), m_str(std::move(str)), m_hash(0) {}

        static u8 computeHash(const std::wstring &str) { return share::computeHash(str); }

        [[nodiscard]] u8 size() const noexcept { return m_str.size(); }
    };

    NEW_OBJ_TAG(ObjString, STRING);

    struct ObjArray : Obj {
        std::vector<Value> m_items;

        ObjArray() noexcept : Obj(ARRAY) {}
        explicit ObjArray(const u8 size) : Obj(ARRAY) { m_items.reserve(size); }

        [[nodiscard]] u8 size() const noexcept { return m_items.size(); }
        void push(const Value& v) { m_items.push_back(v); }
        Value& operator[](const size_t i ) { return m_items[i]; }
    };

    NEW_OBJ_TAG(ObjArray, ARRAY);

    struct ObjUpvalue;

    struct ObjFunc : Obj {
        std::wstring m_name;
        u4 m_arg_count = 0;
        u4 m_local_count = 0;
        Chunk m_chunk;
        std::vector<ObjUpvalue*> m_upvalues;
        ObjFunc() : Obj(FUNC) {}
    };

    NEW_OBJ_TAG(ObjFunc, FUNC);

    struct ObjUpvalue : Obj {
        Value* m_location;
        Value m_closed;
        ObjUpvalue* m_openNext = nullptr;

        explicit ObjUpvalue(Value* slot) : Obj(UPVALUE), m_location(slot) {}

        [[nodiscard]] Value& get() const { return *m_location;}
        void close() { m_closed = *m_location; m_location = &m_closed; }
    };

    NEW_OBJ_TAG(ObjUpvalue, UPVALUE);

    class vm;
    using NativeFn = Value(*)(vm*, i4 argCount, Value* args);

    struct ObjNative : Obj {
        NativeFn m_func;
        std::string m_name;
        u1 m_arity;

        ObjNative(const NativeFn fn, std::string name, const u1 arity) : Obj(NATIVE),
         m_func(fn), m_name(std::move(name)), m_arity(arity) {}
    };

    NEW_OBJ_TAG(ObjNative, NATIVE);

    struct ObjClass : Obj {
        std::wstring m_name;
        ObjClass* m_parent = nullptr;
        std::unordered_map<std::wstring, ObjFunc*> m_methods;
        std::unordered_map<std::wstring, Value*> m_fields;
        std::unordered_map<std::wstring, u4> m_field_slots;
        u4 m_field_slot = 0;

        explicit ObjClass(std::wstring  name) : Obj(CLASS), m_name(std::move(name)) {}
    };

    NEW_OBJ_TAG(ObjClass, CLASS);

    struct ObjInstance : Obj {
        ObjClass* m_klass;
        std::vector<Value> m_fields;

        explicit ObjInstance(ObjClass* klass) : Obj(INSTANCE), m_klass(klass) {}
    };

    NEW_OBJ_TAG(ObjInstance, INSTANCE);

    struct ObjMethod : Obj {
        Value m_self;
        ObjFunc* m_func;

        ObjMethod(Value self, ObjFunc* func) noexcept
            : Obj(METHOD), m_self(self), m_func(func) {
        }
    };

    NEW_OBJ_TAG(ObjMethod, METHOD);

    template <typename T>
    T* as(Obj* o) {
        if (!o || o->m_type != ObjTag<T>::value) return nullptr;
        return static_cast<T*>(o);
    }

    template <typename T>
    const T* as(const Obj* o) {
        if (!o || o->m_type != ObjTag<T>::value) return nullptr;
        return static_cast<const T*>(o);
    }

    inline bool isString  (const Obj* o) { return isType(o, Obj::STRING);  }
    inline bool isArray   (const Obj* o) { return isType(o, Obj::ARRAY);   }
    inline bool isFunction(const Obj* o) { return isType(o, Obj::FUNC);}
    inline bool isNative  (const Obj* o) { return isType(o, Obj::NATIVE);  }
    inline bool isClass   (const Obj* o) { return isType(o, Obj::CLASS);   }
    inline bool isInstance(const Obj* o) { return isType(o, Obj::INSTANCE);}
}

#endif //MYVM_OBJ_H
