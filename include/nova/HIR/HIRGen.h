#pragma once

#include "HIR.h"
#include "nova/Frontend/AST.h"

namespace nova::hir {

// Generate HIR from AST
HIRModule* generateHIR(Program& program, const std::string& moduleName = "main", const std::string& filePath = "");

} // namespace nova::hir
