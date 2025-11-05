#include "nova/HIR/HIR.h"
#include <iostream>

namespace nova::hir {

class HIRPrinter {
public:
    void print(const HIRModule& module) {
        std::cout << module.toString();
    }
    
    void print(const HIRFunction& function) {
        std::cout << function.toString();
    }
    
    void print(const HIRBasicBlock& block) {
        std::cout << block.toString();
    }
    
    void print(const HIRInstruction& inst) {
        std::cout << inst.toString() << "\n";
    }
};

void printHIRModule(const HIRModule& module) {
    HIRPrinter printer;
    printer.print(module);
}

void printHIRFunction(const HIRFunction& function) {
    HIRPrinter printer;
    printer.print(function);
}

} // namespace nova::hir
