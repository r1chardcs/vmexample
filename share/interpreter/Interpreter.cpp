//
// Created by dlllibstdntc on 01.10.2026.
//

#include <cstring>

#include "interpreter.h"
#include "share/vm/call_frame.h"
#include "share/vm/vm.h"

// ---------------------------------------------------------------------------
// Макросы для чтения байткода. frame — CallFrame*.
// ---------------------------------------------------------------------------
#define READ_BYTE()  (*frame->ip++)

#define READ_U32()                                                       \
    ( frame->ip += 4,                                                    \
      ( static_cast<u4>(frame->ip[-4])        ) |                        \
      ( static_cast<u4>(frame->ip[-3]) <<  8 ) |                         \
      ( static_cast<u4>(frame->ip[-2]) << 16 ) |                         \
      ( static_cast<u4>(frame->ip[-1]) << 24 ) )

#define READ_I32()   (static_cast<i4>(READ_U32()))
#define READ_CONST() (frame->chunk->m_constants[READ_U32()])

void share::interpreter::run() {
    CallFrame* frame = &m_vm->m_frames.back();

    for (;;) {
            if (m_vm->m_gc.bytesAllocated > m_vm->m_gc.nextGC)
                m_vm->collectGarbage();

            if (frame->ip >= frame->chunk->m_code.data() + frame->chunk->m_code.size())
                m_vm->runtimeError("Instruction pointer out of bounds.");

            const auto op = static_cast<share::op>(READ_BYTE());

            switch (op) {
                case op::nop: break;

                case op::push_const:
                    m_vm->push(READ_CONST());
                    break;

                case op::push_null:  m_vm->push(Value::Null());    break;
                case op::push_true:  m_vm->push(Value::Integer(1)); break;
                case op::push_false: m_vm->push(Value::Integer(0)); break;

                case op::pop: m_vm->pop();         break;
                case op::dup: m_vm->push(m_vm->peek());  break;

                case op::load_local: {
                    const u4 slotIdx = READ_U32();
                    m_vm->push(m_vm->m_stack[frame->stackBase + slotIdx]);
                    break;
                }
                case op::store_local: {
                    const u4 slotIdx = READ_U32();
                    m_vm->m_stack[frame->stackBase + slotIdx] = m_vm->pop();
                    break;
                }

                case op::load_global: {
                    Obj* o = READ_CONST().asObj();
                    if (!o || o->m_type != Obj::STRING)
                        m_vm->runtimeError("load_global: constant is not a string.");
                    const auto& name = as<ObjString>(o)->m_str;
                    auto it = m_vm->m_globals.find(name);
                    m_vm->push(it == m_vm->m_globals.end() ? Value::Null() : it->second);
                    break;
                }
                case op::store_global: {
                    Obj* o = READ_CONST().asObj();
                    if (!o || o->m_type != Obj::STRING)
                        m_vm->runtimeError("store_global: constant is not a string.");
                    const auto& name = as<ObjString>(o)->m_str;
                    m_vm->m_globals[name] = m_vm->pop();
                    break;
                }

                case op::load_field: {
                    const u4 slotIdx = READ_U32();
                    const Value receiver = m_vm->pop();
                    if (!receiver.isObj() || receiver.asObj()->m_type != Obj::INSTANCE)
                        m_vm->runtimeError("Only instances have fields.");
                    m_vm->push(as<ObjInstance>(receiver.asObj())->m_fields[slotIdx]);
                    break;
                }
                case op::store_field: {
                    const u4 slotIdx = READ_U32();
                    Value value = m_vm->pop();
                    Value receiver = m_vm->pop();
                    if (!receiver.isObj() || receiver.asObj()->m_type != Obj::INSTANCE)
                        m_vm->runtimeError("Only instances have fields.");
                    as<ObjInstance>(receiver.asObj())->m_fields[slotIdx] = value;
                    m_vm->push(value);
                    break;
                }

                case op::add: {
                    Value b = m_vm->pop(); Value a = m_vm->pop();
                    if (a.isInt() && b.isInt())
                        m_vm->push(Value::Integer(a.asInt() + b.asInt()));
                    else
                        m_vm->push(Value::Float(a.toFloat() + b.toFloat()));
                    break;
                }
                case op::sub: {
                    Value b = m_vm->pop(); Value a = m_vm->pop();
                    if (a.isInt() && b.isInt())
                        m_vm->push(Value::Integer(a.asInt() - b.asInt()));
                    else
                        m_vm->push(Value::Float(a.toFloat() - b.toFloat()));
                    break;
                }
                case op::mul: {
                    Value b = m_vm->pop(); Value a = m_vm->pop();
                    if (a.isInt() && b.isInt())
                        m_vm->push(Value::Integer(a.asInt() * b.asInt()));
                    else
                        m_vm->push(Value::Float(a.toFloat() * b.toFloat()));
                    break;
                }
                case op::div: {
                    Value b = m_vm->pop(); Value a = m_vm->pop();
                    if (b.toFloat() == 0.0) m_vm->runtimeError("Division by zero.");
                    m_vm->push(Value::Float(a.toFloat() / b.toFloat()));
                    break;
                }
                case op::neg: {
                    Value a = m_vm->pop();
                    if (a.isInt()) m_vm->push(Value::Integer(-a.asInt()));
                    else           m_vm->push(Value::Float(-a.toFloat()));
                    break;
                }
                case op::not_: {
                    Value a = m_vm->pop();
                    m_vm->push(Value::Integer(m_vm->isFalsey(a) ? 1 : 0));
                    break;
                }
                case op::eq: {
                    Value b = m_vm->pop(); Value a = m_vm->pop();
                    m_vm->push(Value::Integer((a == b) ? 1 : 0));
                    break;
                }
                case op::neq: {
                    Value b = m_vm->pop(); Value a = m_vm->pop();
                    m_vm->push(Value::Integer((a != b) ? 1 : 0));
                    break;
                }
                case op::lt: {
                    Value b = m_vm->pop(); Value a = m_vm->pop();
                    m_vm->push(Value::Integer(a.toFloat() < b.toFloat() ? 1 : 0));
                    break;
                }
                case op::le: {
                    Value b = m_vm->pop(); Value a = m_vm->pop();
                    m_vm->push(Value::Integer(a.toFloat() <= b.toFloat() ? 1 : 0));
                    break;
                }
                case op::gt: {
                    Value b = m_vm->pop(); Value a = m_vm->pop();
                    m_vm->push(Value::Integer(a.toFloat() > b.toFloat() ? 1 : 0));
                    break;
                }
                case op::ge: {
                    Value b = m_vm->pop(); Value a = m_vm->pop();
                    m_vm->push(Value::Integer(a.toFloat() >= b.toFloat() ? 1 : 0));
                    break;
                }

                case op::jmp: {
                    const i4 off = READ_I32();
                    frame->ip += off;
                    break;
                }
                case op::jmp_false: {
                    const i4 off = READ_I32();
                    if (m_vm->isFalsey(m_vm->peek())) frame->ip += off;
                    break;
                }
                case op::jmp_true: {
                    const i4 off = READ_I32();
                    if (!m_vm->isFalsey(m_vm->peek())) frame->ip += off;
                    break;
                }

                case op::call: {
                    const u4 argCount = READ_U32();
                    if (m_vm->m_stack.size() < static_cast<size_t>(argCount + 1))
                        m_vm->runtimeError("Stack underflow on call.");

                    Value callee = m_vm->m_stack[m_vm->m_stack.size() - argCount - 1];
                    m_vm->callValue(callee, static_cast<i4>(argCount));

                    if (m_vm->m_frames.empty()) return;
                    frame = &m_vm->m_frames.back();
                    break;
                }

                case op::tail_call: {
                    const u4 argCount = READ_U32();
                    Value callee = m_vm->m_stack[m_vm->m_stack.size() - argCount - 1];

                    if (!callee.isObj() || callee.asObj()->m_type != Obj::FUNC)
                        m_vm->runtimeError("Tail call requires a function.");

                    ObjFunc* fn = as<ObjFunc>(callee.asObj());
                    if (argCount != fn->m_arg_count)
                        m_vm->runtimeError("Wrong arity in tail call.");

                    m_vm->closeUpvalues(frame->stackBase);

                    const u8 calleeSlot = frame->stackBase - 1;
                    std::memmove(&m_vm->m_stack[calleeSlot],
                                 &m_vm->m_stack[m_vm->m_stack.size() - argCount],
                                 argCount * sizeof(Value));
                    m_vm->m_stack.resize(calleeSlot + argCount);

                    frame->func      = fn;
                    frame->chunk     = &fn->m_chunk;
                    frame->ip        = fn->m_chunk.m_code.data();
                    frame->stackBase = calleeSlot + 1;
                    frame->slotCount = static_cast<u8>(fn->m_local_count);
                    m_vm->m_stack.resize(frame->stackBase + frame->slotCount, Value::Null());
                    break;
                }

                case op::ret: {
                    Value result = m_vm->pop();
                    m_vm->closeUpvalues(frame->stackBase);
                    const u8 base = frame->stackBase;
                    m_vm->popFrame();

                    if (m_vm->m_frames.empty()) {
                        m_vm->push(result);
                        return;
                    }

                    m_vm->m_stack.resize(base - 1);
                    m_vm->push(result);
                    frame = &m_vm->m_frames.back();
                    break;
                }

                case op::ret_null: {
                    m_vm->closeUpvalues(frame->stackBase);
                    const u8 base = frame->stackBase;
                    m_vm->popFrame();

                    if (m_vm->m_frames.empty()) {
                        m_vm->push(Value::Null());
                        return;
                    }

                    m_vm->m_stack.resize(base - 1);
                    m_vm->push(Value::Null());
                    frame = &m_vm->m_frames.back();
                    break;
                }

                case op::new_array: {
                    const u4 n = READ_U32();
                    if (m_vm->m_stack.size() < n)
                        m_vm->runtimeError("Stack underflow on new_array.");

                    ObjArray* arr = m_vm->makeArray();
                    arr->m_items.reserve(n);
                    for (u4 i = 0; i < n; ++i)
                        arr->m_items.push_back(m_vm->peek(n - 1 - i));
                    m_vm->m_stack.resize(m_vm->m_stack.size() - n);
                    m_vm->push(Value::Obj(arr));
                    break;
                }

                case op::closure: {
                    Obj* o = READ_CONST().asObj();
                    if (!o || o->m_type != Obj::FUNC)
                        m_vm->runtimeError("closure: constant is not a function.");
                    m_vm->push(Value::Obj(o));
                    break;
                }

                case op::halt:
                    return;

                default:
                    m_vm->runtimeError("Unknown opcode: " +
                                 std::to_string(static_cast<int>(op)));
            }
        }
}
