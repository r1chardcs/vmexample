//
// Created by dlllibstdntc on 01.10.2026.
//

#ifndef MYVM_INTEPRETER_H
#define MYVM_INTEPRETER_H

namespace share {
    class vm;
    class interpreter {
    public:
        interpreter(vm* vm) : m_vm(vm) {}
        void run();
    private:
        vm* m_vm;
    };
}

#endif //MYVM_INTEPRETER_H
