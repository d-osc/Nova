#include "nova/MIR/MIR.h"
#include <iostream>

namespace nova::mir {

// ==================== MIR Printer Implementation ====================

class MIRPrinter {
public:
    static void print(const MIRModule& module) {
        std::cout << module.toString();
    }
    
    static void print(const MIRFunction& function) {
        std::cout << function.toString();
    }
    
    static void print(const MIRBasicBlock& block) {
        std::cout << block.toString();
    }
    
    static void dump(const MIRModule* module) {
        if (module) {
            module->dump();
        }
    }
};

} // namespace nova::mir
