#pragma once

#include "nova/MIR/MIR.h"
#include "nova/HIR/HIR.h"
#include <string>

namespace nova::mir {

// Generate MIR from HIR
// This is the main entry point for MIR generation
MIRModule* generateMIR(hir::HIRModule* hirModule, const std::string& moduleName);

} // namespace nova::mir
