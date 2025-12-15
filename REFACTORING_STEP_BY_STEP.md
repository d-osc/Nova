# การแยกไฟล์ HIRGen.cpp - คู่มือทีละขั้นตอน

**วันที่:** 2025-12-07
**ไฟล์เป้าหมาย:** `src/hir/HIRGen.cpp` (18,470 บรรทัด)
**สถานะ:** ✅ เริ่มแล้ว - สร้าง HIRGen_Literals.cpp

---

## 📊 สถานะปัจจุบัน

### ✅ ทำเสร็จแล้ว:
1. สร้างแผนการแยกไฟล์ (`REFACTORING_PLAN.md`)
2. สร้างโมดูลแรก: `HIRGen_Literals.cpp` (ตัวอย่าง)

### ⏳ ต้องทำต่อ:
1. ลบโค้ดซ้ำออกจาก `HIRGen.cpp` ต้นฉบับ
2. สร้างโมดูลอื่นๆ (9 โมดูล)
3. อัพเดท CMakeLists.txt
4. ทดสอบ compilation

---

## 🎯 ปัญหาที่พบ

### ⚠️ HIRGenerator Class Structure

โครงสร้างปัจจุบันของ `HIRGen.cpp`:

```cpp
// src/hir/HIRGen.cpp
namespace nova::hir {

class HIRGenerator : public ASTVisitor {
public:
    // Constructor (line 20)
    explicit HIRGenerator(HIRModule* module) { ... }

    // Visitor methods (lines 26-18291) - 18,265 บรรทัด!
    void visit(NumberLiteral& node) override { ... }
    void visit(StringLiteral& node) override { ... }
    // ... 74 visitor methods อื่นๆ

private:
    // Member variables (lines 18292-18470) - 178 บรรทัด
    HIRModule* module_;
    std::unique_ptr<HIRBuilder> builder_;
    HIRValue* lastValue_;
    // ... ตัวแปรอีกมากมาย
};

} // namespace
```

**ปัญหา:** Class definition อยู่ใน .cpp file ทำให้ไม่สามารถแยก implementation ออกได้โดยตรง

---

## 💡 วิธีแก้ไข (2 ทางเลือก)

### Option 1: ใช้ Partial Class Pattern (แนะนำ)

ใน C++ สามารถแยก implementation ของ class methods ไปไฟล์อื่นได้โดย:
1. เก็บ class definition ไว้ในไฟล์หลัก
2. ประกาศ methods เป็น forward declarations
3. Implement methods ในไฟล์แยก

**ข้อดี:** ไม่ต้องแก้ไข header files มาก
**ข้อเสีย:** ต้องระวังเรื่อง compilation order

### Option 2: Extract Class to Header (ซับซ้อนกว่า)

1. ย้าย class definition ไป header file
2. แยก implementations ไปไฟล์ต่างๆ
3. อัพเดท includes ทั้งหมด

**ข้อดี:** Standard C++ pattern
**ข้อเสีย:** ต้องแก้ไขหลายไฟล์, build time อาจนานขึ้น

---

## 🚀 แนวทางที่แนะนำ: Partial Class Pattern

### ขั้นตอนที่ 1: เตรียม HIRGen.cpp หลัก

เก็บเฉพาะ:
- Class declaration
- Constructor
- Helper methods
- Private members

ตัด Visitor methods ออกทั้งหมด (เก็บแค่ declarations)

### ขั้นตอนที่ 2: สร้างไฟล์แยก

สำหรับแต่ละ module:

```cpp
// src/hir/HIRGen_Literals.cpp
#include "nova/HIR/HIRGen_Internal.h"

namespace nova::hir {

// Implement visitor methods here
void HIRGenerator::visit(NumberLiteral& node) {
    // Implementation
}

void HIRGenerator::visit(StringLiteral& node) {
    // Implementation
}

// ... other literal methods

} // namespace
```

### ขั้นตอนที่ 3: อัพเดท Build System

```cmake
# src/hir/CMakeLists.txt
set(HIR_SOURCES
    HIRGen.cpp
    HIRGen_Literals.cpp        # ← เพิ่มไฟล์ใหม่
    # ... other modules
    MIRGen.cpp
)
```

---

## 📝 ตัวอย่าง: HIRGen_Literals.cpp

ผมได้สร้างไฟล์ตัวอย่างแล้ว: `src/hir/HIRGen_Literals.cpp`

**เนื้อหา:**
- ✅ NumberLiteral
- ✅ BigIntLiteral
- ✅ StringLiteral
- ✅ RegexLiteralExpr
- ✅ BooleanLiteral
- ✅ NullLiteral
- ✅ UndefinedLiteral

**ขนาด:** ~150 บรรทัด

---

## ⚠️ ปัญหาที่ต้องแก้ไข

### 1. Class Definition Visibility

ปัจจุบัน `HIRGenerator` class อยู่ใน .cpp file ทำให้ไฟล์อื่นมองไม่เห็น

**แก้ไข:**
- สร้าง internal header: `include/nova/HIR/HIRGen_Internal.h`
- ย้าย class definition ไปที่นั่น
- Include ใน all implementation files

### 2. Member Access

Implementation files ต้องเข้าถึง private members

**แก้ไข:**
- ทำเป็น friend class, หรือ
- เปลี่ยน private → protected

### 3. Circular Dependencies

อาจเกิด circular includes

**แก้ไข:**
- ใช้ forward declarations
- จัดเรียง includes ให้ถูกต้อง

---

## 🔧 การแก้ไขที่ต้องทำ

### Step A: สร้าง Internal Header

```cpp
// include/nova/HIR/HIRGen_Internal.h
#pragma once

#include "nova/HIR/HIR.h"
#include "nova/Frontend/AST.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace nova::hir {

class HIRGenerator : public ASTVisitor {
public:
    explicit HIRGenerator(HIRModule* module);
    HIRModule* getModule();

    // Visitor method declarations (all 76 methods)
    void visit(NumberLiteral& node) override;
    void visit(BigIntLiteral& node) override;
    void visit(StringLiteral& node) override;
    // ... all other visitor declarations

private:
    // All private members
    HIRModule* module_;
    std::unique_ptr<HIRBuilder> builder_;
    HIRValue* lastValue_;
    // ... all other private members

    // Allow implementation files to access private members
    friend class HIRGeneratorImpl;  // If needed
};

} // namespace nova::hir
```

### Step B: แก้ไข HIRGen.cpp หลัก

```cpp
// src/hir/HIRGen.cpp
#include "nova/HIR/HIRGen_Internal.h"

namespace nova::hir {

// Constructor implementation
HIRGenerator::HIRGenerator(HIRModule* module)
    : module_(module), builder_(nullptr), currentFunction_(nullptr) {
}

HIRModule* HIRGenerator::getModule() {
    return module_;
}

// NOTE: Visitor implementations moved to separate files:
// - HIRGen_Literals.cpp
// - HIRGen_Operators.cpp
// - etc.

} // namespace
```

### Step C: แก้ไข HIRGen_Literals.cpp

```cpp
// src/hir/HIRGen_Literals.cpp
#include "nova/HIR/HIRGen_Internal.h"

namespace nova::hir {

void HIRGenerator::visit(NumberLiteral& node) {
    // Implementation here
}

// ... other implementations

} // namespace
```

### Step D: อัพเดท CMakeLists.txt

```cmake
# src/CMakeLists.txt หรือ src/hir/CMakeLists.txt

# Find all HIRGen source files
set(HIRGEN_SOURCES
    hir/HIRGen.cpp
    hir/HIRGen_Literals.cpp
    hir/HIRGen_Operators.cpp
    # Add more as you create them
)

# Add to library
add_library(novacore
    ${HIRGEN_SOURCES}
    # ... other sources
)
```

---

## 📋 Checklist การแยกไฟล์

### ✅ Phase 1: เตรียมการ (ทำแล้ว)
- [x] วิเคราะห์โครงสร้างไฟล์
- [x] สร้างแผนการแยก
- [x] สร้างโมดูลตัวอย่าง (Literals)

### ⏳ Phase 2: Refactor (ต้องทำ)
- [ ] สร้าง `HIRGen_Internal.h`
- [ ] ย้าย class definition ไป header
- [ ] ลบ implementations ออกจาก `HIRGen.cpp`
- [ ] สร้างไฟล์แยกทั้งหมด (9 ไฟล์)
- [ ] อัพเดท CMakeLists.txt

### ⏳ Phase 3: ทดสอบ (ต้องทำ)
- [ ] Compile และแก้ไข errors
- [ ] ทดสอบ functionality
- [ ] Verify ไม่มีอะไรพัง

---

## 🎯 โมดูลที่ต้องสร้าง (เหลืออีก 9 ไฟล์)

1. **HIRGen_Literals.cpp** ✅ (สร้างแล้ว - 150 lines)
2. **HIRGen_Operators.cpp** ⏳ (ประมาณ 1,500 lines)
   - BinaryExpr, UnaryExpr, UpdateExpr, ConditionalExpr
3. **HIRGen_Functions.cpp** ⏳ (ประมาณ 2,000 lines)
   - FunctionExpr, ArrowFunctionExpr
4. **HIRGen_Classes.cpp** ⏳ (ประมาณ 3,000 lines)
   - ClassExpr, NewExpr, ThisExpr, SuperExpr
5. **HIRGen_Arrays.cpp** ⏳ (ประมาณ 1,000 lines)
   - ArrayExpr, array methods
6. **HIRGen_Objects.cpp** ⏳ (ประมาณ 1,500 lines)
   - ObjectExpr, MemberExpr
7. **HIRGen_ControlFlow.cpp** ⏳ (ประมาณ 2,500 lines)
   - IfStmt, ForStmt, WhileStmt, SwitchStmt, etc.
8. **HIRGen_Statements.cpp** ⏳ (ประมาณ 2,000 lines)
   - VariableDecl, BlockStmt, ExprStmt, etc.
9. **HIRGen_Calls.cpp** ⏳ (ประมาณ 3,500 lines)
   - CallExpr, built-in functions
10. **HIRGen_Advanced.cpp** ⏳ (ประมาณ 1,000 lines)
    - AwaitExpr, YieldExpr, JSX, etc.

---

## 💻 คำสั่งที่ต้องใช้

### 1. Backup ไฟล์ต้นฉบับ
```bash
cp src/hir/HIRGen.cpp src/hir/HIRGen.cpp.backup
```

### 2. สร้างไฟล์ใหม่
```bash
# สร้างโมดูลแต่ละไฟล์
touch src/hir/HIRGen_Operators.cpp
touch src/hir/HIRGen_Functions.cpp
# ... etc
```

### 3. แก้ไข CMakeLists.txt
```bash
# แก้ไขไฟล์ build configuration
vim CMakeLists.txt  # หรือ editor ที่ชอบ
```

### 4. ทดสอบ Build
```bash
cmake --build build --config Release
```

---

## ⚡ เคล็ดลับ

### 1. ทำทีละโมดูล
- อย่าพยายามแยกทั้งหมดในครั้งเดียว
- แยก 1 โมดูล → test → แยกต่อ

### 2. เก็บ Backup
- Backup ทุกครั้งก่อนแก้ไข
- ใช้ git commit บ่อยๆ

### 3. Test บ่อยๆ
- Build หลังแยกแต่ละโมดูล
- อย่ารอให้แยกเสร็จหมดค่อย build

### 4. ใช้ Script
สร้าง script ช่วยแยกโค้ด:

```bash
#!/bin/bash
# extract_visitor.sh - Extract visitor method to new file

VISITOR_NAME=$1
OUTPUT_FILE=$2

# Extract from HIRGen.cpp
grep -A 100 "void visit($VISITOR_NAME" src/hir/HIRGen.cpp > temp.cpp
# Process and add to output file
# ...
```

---

## 🎓 สรุป

### สิ่งที่ได้:
1. ✅ แผนการแยกไฟล์ละเอียด
2. ✅ ตัวอย่างโมดูลแรก (Literals)
3. ✅ คู่มือทีละขั้นตอน

### สิ่งที่ต้องทำต่อ:
1. สร้าง `HIRGen_Internal.h`
2. แยกโมดูลที่เหลือ (9 ไฟล์)
3. อัพเดท build system
4. ทดสอบ compilation

### ประมาณเวลา:
- สร้าง header: 30 นาที
- แยกแต่ละโมดูล: 15-30 นาที/โมดูล
- รวมทั้งหมด: **2-4 ชั่วโมง**

### ประโยชน์ที่ได้:
- ✅ ไฟล์เล็กลง อ่านง่ายขึ้น
- ✅ Build เร็วขึ้น (incremental)
- ✅ ง่ายต่อการ maintain
- ✅ ลด merge conflicts

---

## 📞 ต้องการความช่วยเหลือ?

ถ้าต้องการให้ผมช่วยแยกไฟล์ต่อ สามารถบอกได้ว่า:
1. ต้องการแยกโมดูลไหนต่อ
2. เจอปัญหาตรงไหน
3. ต้องการคำแนะนำอะไรเพิ่มเติม

---

**Nova Compiler - Refactoring Guide**
**สถานะ:** เริ่มแล้ว (1/10 modules)
**Next:** สร้าง HIRGen_Internal.h และแยกโมดูลที่เหลือ
