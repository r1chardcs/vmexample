//
// Created by dlllibstdntc on 01.10.2026.
//

#include <cstdio>
#include <cstring>
#include <stdexcept>

#include <share/vm/vm.h>

namespace share {

    // ---------------------------------------------------------------------------
    // Точка входа
    // ---------------------------------------------------------------------------
    void vm::interpret(ObjFunc* fn) {
        m_stack.clear();
        m_frames.clear();
        m_openUpvalues = nullptr;
        m_hadError     = false;
        m_errorMsg.clear();

        push(Value::Obj(fn));       // callee-слот
        callFunc(fn, 0);
        run();
    }

    // ---------------------------------------------------------------------------
    // Вызов значения любого типа
    // ---------------------------------------------------------------------------
    Value vm::callValue(Value callee, i4 argCount) {
        if (!callee.isObj()) {
            const char* t = "unknown";
            switch (callee.m_type) {
                case Value::nil:    t = "null";  break;
                case Value::int_:   t = "int";   break;
                case Value::float_: t = "float"; break;
                case Value::char_:  t = "char";  break;
                default:            t = "obj";   break;
            }
            runtimeError(std::string("Can only call functions, methods and classes, got ") + t);
        }

        Obj* o = callee.asObj();
        switch (o->m_type) {
            case Obj::FUNC:   return callFunc  (as<ObjFunc>(o),   argCount);
            case Obj::NATIVE: return callNative(as<ObjNative>(o), argCount);
            case Obj::CLASS:  return callClass (as<ObjClass>(o),  argCount);
            case Obj::METHOD: return callMethod(as<ObjMethod>(o), argCount);
            default:
                runtimeError("Value is not callable.");
        }
    }

    // ---------------------------------------------------------------------------
    // Вызов байткод-функции
    // ---------------------------------------------------------------------------
    Value vm::callFunc(ObjFunc* fn, i4 argCount) {
        if (argCount != static_cast<i4>(fn->m_arg_count)) {
            runtimeError("Wrong arity: expected " +
                         std::to_string(fn->m_arg_count) +
                         ", got " + std::to_string(argCount));
        }

        if (m_frames.size() >= get_max_frames())
            runtimeError("Stack overflow: too many frames.");

        if (m_stack.size() < static_cast<size_t>(argCount + 1))
            runtimeError("Stack underflow: missing callee/args.");

        CallFrame frame{};
        frame.func       = fn;
        frame.chunk      = &fn->m_chunk;
        frame.ip         = fn->m_chunk.m_code.data();
        frame.stackBase  = static_cast<u8>(m_stack.size() - argCount);
        frame.slotCount  = static_cast<u8>(fn->m_local_count);

        m_frames.push_back(frame);

        if (frame.slotCount > argCount) {
            m_stack.resize(frame.stackBase + frame.slotCount, Value::Null());
        }

        return Value::Null();
    }

    // ---------------------------------------------------------------------------
    // Вызов нативной функции
    // ---------------------------------------------------------------------------
    Value vm::callNative(ObjNative* nf, i4 argCount) {
        if (argCount != static_cast<i4>(nf->m_arity))
            runtimeError("Wrong arity for native " + nf->m_name);

        Value* args = &m_stack[m_stack.size() - argCount];

        Value result = nf->m_func(this, argCount, args);

        // Срезаем callee + аргументы
        m_stack.resize(m_stack.size() - argCount - 1);
        push(result);
        return result;
    }

    // ---------------------------------------------------------------------------
    // Вызов класса: создать instance, вызвать <init> если есть
    // ---------------------------------------------------------------------------
    Value vm::callClass(ObjClass* klass, i4 argCount) {
        ObjInstance* inst = makeInstance(klass);
        m_stack[m_stack.size() - argCount - 1] = Value::Obj(inst);

        auto it = klass->m_methods.find(L"init");
        if (it == klass->m_methods.end()) {
            if (argCount != 0)
                runtimeError("Class has no initializer.");

            Value instVal = Value::Obj(inst);
            m_stack.resize(m_stack.size() - argCount - 1);
            push(instVal);
            return instVal;
        }
        return callFunc(it->second, argCount);
    }

    // ---------------------------------------------------------------------------
    // Вызов bound-метода
    // ---------------------------------------------------------------------------
    Value vm::callMethod(ObjMethod* m, i4 argCount) {
        m_stack[m_stack.size() - argCount - 1] = m->m_self;
        return callFunc(m->m_func, argCount);
    }

    // ---------------------------------------------------------------------------
    // Главный цикл
    // ---------------------------------------------------------------------------
    void vm::run() {
        interpreter interpreter(this);
        interpreter.run();
    }

    // ---------------------------------------------------------------------------
    // Upvalues
    // ---------------------------------------------------------------------------
    ObjUpvalue* vm::captureUpvalue(Value* local) {
        ObjUpvalue* prev = nullptr;
        ObjUpvalue* cur  = m_openUpvalues;

        while (cur && cur->m_location > local) {
            prev = cur;
            cur  = cur->m_openNext;
        }
        if (cur && cur->m_location == local) return cur;

        auto* created = m_gc.allocate<ObjUpvalue>(local);
        created->m_openNext = cur;

        if (prev) prev->m_openNext = created;
        else      m_openUpvalues   = created;

        return created;
    }

    void vm::closeUpvalues(u8 fromBase) {
        while (m_openUpvalues &&
               m_openUpvalues->m_location >= &m_stack[fromBase]) {
            ObjUpvalue* up = m_openUpvalues;
            up->close();
            m_openUpvalues = up->m_openNext;
        }
    }

    // ---------------------------------------------------------------------------
    // GC-корни
    // ---------------------------------------------------------------------------
    void vm::markRoots() {
        for (auto& v : m_stack)  m_gc.markValue(v);
        for (auto& f : m_frames) m_gc.markObject(f.func);
        for (auto& [_, s] : m_strings) m_gc.markObject(s);
        for (auto& [_, v] : m_globals) m_gc.markValue(v);

        for (ObjUpvalue* up = m_openUpvalues; up; up = up->m_openNext)
            m_gc.markObject(up);
    }

    void vm::collectGarbage() {
        markRoots();
        m_gc.collect();
    }

    // ---------------------------------------------------------------------------
    // Ошибки
    // ---------------------------------------------------------------------------
    void vm::runtimeError(const std::string& msg) {
        m_hadError = true;
        m_errorMsg = msg;

        std::fprintf(stderr, "RuntimeError: %s\n", msg.c_str());
        printStackTrace();
        std::fflush(stderr);

        m_stack.clear();
        m_frames.clear();
        m_openUpvalues = nullptr;

        throw std::runtime_error(msg);
    }

    void vm::printStackTrace() const {
        for (auto it = m_frames.rbegin(); it != m_frames.rend(); ++it) {
            const CallFrame& f = *it;
            std::wprintf(L"  at %ls (line %d)\n",
                         f.func->m_name.c_str(),
                         static_cast<int>(f.line()));
        }
    }

    // ---------------------------------------------------------------------------
    // Фабрики
    // ---------------------------------------------------------------------------
    ObjString* vm::makeString(std::wstring s) {
        auto it = m_strings.find(s);
        if (it != m_strings.end()) return it->second;

        auto* o = m_gc.allocate<ObjString>(std::move(s));
        m_strings[o->m_str] = o;
        return o;
    }

    ObjArray* vm::makeArray() {
        return m_gc.allocate<ObjArray>();
    }

    ObjFunc* vm::makeFunc(std::wstring name, u4 argCount, u4 localCount) {
        auto* f = m_gc.allocate<ObjFunc>();
        f->m_name        = std::move(name);
        f->m_arg_count   = argCount;
        f->m_local_count = localCount;
        return f;
    }

    ObjNative* vm::makeNative(NativeFn fn, std::string name, u1 arity) {
        return m_gc.allocate<ObjNative>(fn, std::move(name), arity);
    }

    ObjClass* vm::makeClass(std::wstring name) {
        return m_gc.allocate<ObjClass>(std::move(name));
    }

    ObjInstance* vm::makeInstance(ObjClass* klass) {
        auto* inst = m_gc.allocate<ObjInstance>(klass);

        size_t total = 0;
        for (ObjClass* k = klass; k; k = k->m_parent)
            total += k->m_field_slot;

        inst->m_fields.resize(total, Value::Null());
        return inst;
    }

    void vm::pushRoot(Obj** slot) { m_gc.extraRoots.push_back(slot); }
    void vm::popRoot()            { m_gc.extraRoots.pop_back(); }

} // namespace share