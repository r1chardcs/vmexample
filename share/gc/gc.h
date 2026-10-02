//
// Created by dlllibstdntc on 01.10.2026.
//

#ifndef MYVM_GC_H
#define MYVM_GC_H

#include <ranges>
#include <share/utility/value.h>
#include <share/utility/obj.h>

namespace share {

    constexpr auto limit_gc() { return 1024 * 1024; }

    class gc {
    public:
        Obj* head = nullptr;
        size_t bytesAllocated = 0;
        size_t nextGC = limit_gc();

        std::vector<Obj**> extraRoots;

        template <typename T, typename... Args>
        T* allocate(Args&&... args) {
            if (bytesAllocated > nextGC) collect();

            T* obj = new T(std::forward<Args>(args)...);
            obj->m_next = head;
            head = obj;
            bytesAllocated += sizeof(T);
            return static_cast<T *>(obj);
        }

        void collect();
        void markChildren(Obj * obj);
        void sweep();
        void traceRoots();

        void markObject(Obj* obj) {
            if (!obj || obj->m_marked) return;
            obj->m_marked = true;
            markChildren(obj);
        }

        void markValue(const Value& val) {
            if (val.isObj()) markObject(val.asObj());
        }
    };

    inline void gc::collect() {
        traceRoots();
        sweep();
        nextGC = bytesAllocated * 2;
    }

    inline void gc::markChildren(Obj *obj) {
        switch (obj->m_type) {
            case Obj::STRING:
            case Obj::NATIVE:
                break;
            case Obj::ARRAY: {
                for (const auto* r = static_cast<ObjArray *>(obj) /*NOLINT(*-pro-type-static-cast-downcast)*/;
                    const auto& i : r->m_items) markValue(i);
                break;
            }
            case Obj::FUNC: {
                const auto* f = static_cast<ObjFunc*>(obj); // NOLINT(*-pro-type-static-cast-downcast)
                for (const auto& c : f->m_chunk.m_constants) markValue(c);
                for (auto* up : f->m_upvalues) markObject(up);
                break;
            }
            case Obj::UPVALUE: {
                auto* u = static_cast<ObjUpvalue*>(obj); // NOLINT(*-pro-type-static-cast-downcast)
                markValue(*u->m_location);
                break;
            }
            case Obj::CLASS: {
                auto* c = static_cast<ObjClass*>(obj); // NOLINT(*-pro-type-static-cast-downcast)
                if (c->m_parent) markObject(c->m_parent);
                for (const auto &m: c->m_methods | std::views::values) markObject(m);
                for (const auto &v: c->m_fields | std::views::values)  markValue(*v);
                break;
            }
            case Obj::INSTANCE: {
                const auto* i = static_cast<ObjInstance*>(obj); // NOLINT(*-pro-type-static-cast-downcast)
                markObject(i->m_klass);
                for (auto& v : i->m_fields) markValue(v);
                break;
            }
            case Obj::METHOD: {
                const auto* b = static_cast<ObjMethod*>(obj); // NOLINT(*-pro-type-static-cast-downcast)
                markValue(b->m_self);
                markObject(b->m_func);
                break;
            }

        }
    }

    inline void gc::sweep() {
        Obj** p = &head;
        while (*p) {
            Obj* o = *p;
            if (o->m_marked) {
                o->m_marked = false;
                p = &o->m_next;
            } else {
                *p = o->m_next;
                bytesAllocated -= sizeof(*o);
                delete o;
            }
        }
    }

    inline void gc::traceRoots() {
        for (Obj** root : extraRoots) markObject(*root);
    }
}

#endif //MYVM_GC_H
