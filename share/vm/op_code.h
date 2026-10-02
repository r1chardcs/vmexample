//
// Created by dlllibstdntc on 01.10.2026.
//

#ifndef MYVM_OP_CODE_H
#define MYVM_OP_CODE_H

#include <share/utility/base_types.h>

namespace share {
    enum class op : u1 {
        nop = 0,
        push_const,
        push_null,
        push_true,
        push_false,
        pop,
        dup,


        load_local,
        store_local,

        load_global,
        store_global,

        load_field,
        store_field,

        load_method,

        add, sub, mul, div, mod,
        neg, not_, eq, neq, lt, le, gt, ge,

        jmp, jmp_true, jmp_false,

        call, tail_call, ret, ret_null,

        new_class, new_instance, inherit,

        new_array, index_get, index_set,
        closure,halt
    };
}

#endif //MYVM_OP_CODE_H
