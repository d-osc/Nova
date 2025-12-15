# ✅ เปลี่ยนชื่อจาก npm → nnpm สำเร็จ

## เหตุผล
ชื่อ `npm` ซ้ำกับ npm ของ Node.js ดังนั้นจึงเปลี่ยนเป็น **`nnpm`** (Nova npm)

---

## การเปลี่ยนแปลง

### ✅ ไฟล์ที่เปลี่ยนชื่อ:
- `src/npm_main.cpp` → `src/nnpm_main.cpp`
- `build/Release/npm.exe` → `build/Release/nnpm.exe`

### ✅ ไฟล์ที่อัปเดต:
1. **`src/nnpm_main.cpp`**
   - เปลี่ยน Usage: `npm` → `nnpm`
   - เปลี่ยน Examples: `npm install` → `nnpm install`
   - เปลี่ยน Output prefix: `[npm]` → `[nnpm]`

2. **`CMakeLists.txt`**
   - เปลี่ยน target: `npm` → `nnpm`
   - เปลี่ยน source: `npm_main.cpp` → `nnpm_main.cpp`
   - อัปเดต Windows-specific config
   - อัปเดต install targets

3. **`src/novac_main.cpp`**
   - เปลี่ยน help text: `npm <command>` → `nnpm <command>`

4. **`RUNTIME_COMPILER_GUIDE.md`**
   - เปลี่ยนทุก reference จาก `npm` → `nnpm`

5. **`SEPARATION_COMPLETE.md`**
   - เปลี่ยนทุก reference จาก `npm` → `nnpm`

---

## 🎯 Executables ใหม่:

```
build/Release/
├── nova.exe (24 MB)    - Runtime
├── novac.exe (24 MB)   - Compiler
└── nnpm.exe (323 KB)   - Package Manager ★ ชื่อใหม่!
```

---

## 📋 การใช้งานใหม่:

### Package Management:
```bash
# Initialize project
nnpm init
nnpm init ts

# Install packages
nnpm install
nnpm install lodash
nnpm i express

# Dev dependencies
nnpm install --save-dev typescript
nnpm i -D @types/node

# Global packages
nnpm install -g typescript
nnpm i -g nodemon

# Update packages
nnpm update
nnpm u lodash

# Remove packages
nnpm uninstall lodash
nnpm un express

# Run scripts
nnpm run dev
nnpm run build
nnpm test
```

---

## ✅ การทดสอบ:

### ✅ nnpm.exe
```bash
$ build/Release/nnpm.exe --help
✅ แสดง help ถูกต้อง
✅ ชื่อคำสั่งเป็น "nnpm"
✅ Examples ใช้ "nnpm"
```

### ✅ novac.exe
```bash
$ build/Release/novac.exe --help | grep package
✅ อ้างถึง "nnpm <command>"
```

### ✅ ไฟล์เก่า
```bash
$ ls build/Release/npm.exe
✅ ถูกลบแล้ว
```

---

## 🎉 สรุป:

**เปลี่ยนชื่อเสร็จสมบูรณ์แล้ว!**

✅ เปลี่ยนชื่อจาก `npm` → `nnpm`
✅ อัปเดตโค้ดทั้งหมด
✅ อัปเดต CMakeLists.txt
✅ อัปเดตเอกสารทั้งหมด
✅ Build และทดสอบสำเร็จ
✅ ลบไฟล์เก่าแล้ว

**ชื่อใหม่: `nnpm` (Nova npm)**
**ไม่ซ้ำกับ npm ของ Node.js อีกต่อไป!** 🚀

---

วันที่: 2025-12-07
Nova Version: 1.4.0
สถานะ: ✅ เปลี่ยนชื่อสำเร็จ
