# Plan: npm/framework matrix 1→8 + Symbol.split + commit

## ขอบเขต
1. **npm matrix** (1→8): แก้ runtime semantics ของ 7 packages ที่ fail
2. **Symbol.split** (Test262 +1): สร้าง `nova_regex_split`
3. **Commit** ทุกอย่างที่ทำไว้

## npm matrix analysis (สำคัญ)

ทุก package **require() สำเร็จแล้ว** (พิมพ์ `-ok` ออกมา) ปัญหาคือ **runtime API คืนค่าผิด**:

| Package | Root cause | ความยาก |
|---|---|---|
| lodash | `lodash.chunk([1,2,3],2)` → length 0 (17,259 บรรทัด IIFE) | ปานกลาง |
| zod | `.parse("nova")` → 0 (TS-compiled, ใช้ `__createBinding` ฯลฯ) | ยาก |
| rxjs | `.subscribe()` → total 0 (callback ไม่ทำงาน) | ปานกลาง |
| react | `createElement` → `.type` undefined | ปานกลาง |
| react-dom | `renderToString` → undefined | ยาก |
| vue | `ref`/`computed` → `.value` undefined (CJS conditional require) | ยาก |
| vite | `defineConfig` → `.build.target` = 0 (ESM import) | ปานกลาง |

## แนวทาง
- **แต่ละ package ต้อง debug แยก** — ดูว่า Nova runtime คืนค่าผิดตรงไหน
- **ไม่ใช่ bug เดียว** — แต่ละ package ใช้ pattern ต่างกัน (IIFE, CommonJS conditional require, ESM import, class/prototype, reactivity)
- **เสี่ยงต่อ gate**: แก้ runtime อาจกระทบ 135/135

## ลำดับการทำ (ตาม ROI)
1. **lodash** — น่าจะเป็นปัญหา IIFE exports; แก้ได้ = +1 ง่ายสุด
2. **react** — createElement คืนค่าผิด; น่าจะเป็น prototype/class issue
3. **rxjs** — subscribe callback; อาจเป็น generator/async issue
4. **zod/vite** — TS-compiled patterns; ยากกว่า
5. **vue/react-dom** — reactivity/SSR; ยากที่สุด

## Verification
- Build Debug + Release
- **135/135 gate** ทุกขั้น
- **npm matrix rerun** แต่ละ package หลังแก้
- **Test262 probe** หลังแก้ Symbol.split