//
// Created by dlllibstdntc on 01.10.2026.
//

#ifndef MYVM_VM_H
#define MYVM_VM_H

#include <string>
#include <unordered_map>
#include <vector>

#include <share/gc/gc.h>
#include <share/utility/obj.h>
#include <share/vm/call_frame.h>
#include <share/vm/op_code.h>

#include "share/interpreter/interpreter.h"

namespace share {

    constexpr u8 get_max_frames() { return 1024; }
    constexpr u8 get_stack_max()  { return 4096; }

    struct vm {
        gc                          m_gc;
        std::vector<Value>          m_stack;
        std::vector<CallFrame>      m_frames;
        std::unordered_map<std::wstring, ObjString*> m_strings;
        std::unordered_map<std::wstring, Value>      m_globals;
        ObjUpvalue*                 m_openUpvalues = nullptr;
        bool                        m_hadError = false;
        std::string                 m_errorMsg;

        vm() {
            m_stack.reserve(get_stack_max());
            m_frames.reserve(get_max_frames());
        }

        // ------------------------------------------------------------------
        // Управление
        // ------------------------------------------------------------------
        void  interpret(ObjFunc* fn);

        Value callValue(Value callee, i4 argCount);
        Value callFunc(ObjFunc* fn, i4 argCount);
        Value callNative(ObjNative* nf, i4 argCount);
        Value callClass(ObjClass* klass, i4 argCount);
        Value callMethod(ObjMethod* m, i4 argCount);

        // ------------------------------------------------------------------
        // Фабрики
        // ------------------------------------------------------------------
        ObjString*   makeString(std::wstring s);
        ObjArray*    makeArray();
        ObjFunc*     makeFunc(std::wstring name, u4 argCount, u4 localCount);
        ObjNative*   makeNative(NativeFn fn, std::string name, u1 arity);
        ObjClass*    makeClass(std::wstring name);
        ObjInstance* makeInstance(ObjClass* klass);

        // ------------------------------------------------------------------
        // GC
        // ------------------------------------------------------------------
        void markRoots();
        void collectGarbage();
        void pushRoot(Obj** slot);
        void popRoot();

        // ------------------------------------------------------------------
        // Ошибки
        // ------------------------------------------------------------------
        [[noreturn]] void runtimeError(const std::string& msg);
        void printStackTrace() const;
        [[nodiscard]] bool hadError() const noexcept { return m_hadError; }

        // ------------------------------------------------------------------
        // Стек (inline — hot path)
        // ------------------------------------------------------------------
        void push(Value v) { m_stack.push_back(v); }

        Value pop() {
            Value v = m_stack.back();
            m_stack.pop_back();
            return v;
        }

        Value& peek(i4 distance = 0) {
            return m_stack[m_stack.size() - 1 - distance];
        }

        Value& slot(u8 base, u8 i) {
            return m_stack[base + i];
        }

        CallFrame& currentFrame() { return m_frames.back(); }

        void pushFrame(const CallFrame& f) {
            if (m_frames.size() >= get_max_frames())
                runtimeError("Stack overflow: too many frames.");
            m_frames.push_back(f);
        }

        void popFrame() { m_frames.pop_back(); }

        // ------------------------------------------------------------------
        // Upvalues
        // ------------------------------------------------------------------
        ObjUpvalue* captureUpvalue(Value* local);
        void        closeUpvalues(u8 fromBase);

        // ------------------------------------------------------------------
        // Интерпретация
        // ------------------------------------------------------------------
        void run();

        static bool isFalsey(const Value& v) noexcept {
            return v.isNull() || (v.isInt() && v.asInt() == 0);
        }

        friend interpreter;
    };

} // namespace share

#endif // MYVM_VM_H