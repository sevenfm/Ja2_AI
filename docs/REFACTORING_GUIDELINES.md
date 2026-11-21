# Legacy Code Refactoring Guidelines (C++03 to C++14)

This document provides strict safety rules for contributors and AI assistants performing **limited, targeted refactoring** of legacy C/C++03 code toward ISO C++14 (MSVC `v143`) to improve readability and stability without introducing regressions.

> **Core Rule:** Never refactor working legacy code purely for cosmetic style. Refactor only when actively modifying a function for a bugfix or feature, or when isolating a fragile subsystem.

---

## 1. High-Risk "Never Touch" Zones (Hard Prohibitions)

Legacy game engines rely on raw memory layouts and global state. **Do NOT apply C++14 modernization to the following:**

1. **Serialized & POD Structs (`memset` / `memcpy` / Savegames):**
   - Never add non-trivial C++ types (`std::vector`, `std::string`, `std::unique_ptr`, virtual functions, or non-trivial constructors) to existing legacy structs.
   - Legacy code frequently initializes or copies structs via `memset(&obj, 0, sizeof(obj))`, `memcpy`, or raw binary file reads/writes (`FileRead`/`FileWrite`). Adding non-POD members corrupts memory and breaks savegame compatibility.
2. **Global Array Layouts & Indexing:**
   - Do not replace fixed-size global C arrays (e.g., entity pools indexed by fixed IDs or sentinel values) with dynamic `std::vector` containers, as legacy code assumes stable pointer addresses and fixed `MAX_*` bounds.
3. **Massive Monolithic Functions in One Pass:**
   - Never ask an AI or contributor to "modernize" an entire 1,000+ line procedural function in a single commit. Extract small, self-contained helper functions first, verify behavior, and refactor incrementally.

---

## 2. Safe C++14 Refactoring Patterns (Green-Light Improvements)

When refactoring the **internal body** of a legacy function (or extracting logic into a new helper class), the following C++14 upgrades are safe and encouraged:

| Legacy Pattern | Safe C++14 Replacement | Why It Improves Safety |
| :--- | :--- | :--- |
| `NULL` / `0` for pointers | `nullptr` | Eliminates integer-vs-pointer overload ambiguity. |
| Raw `new` / `delete` with multiple `return` paths | `std::unique_ptr<T>` (or `std::vector<T>`) | Guarantees automatic cleanup on early returns (RAII). |
| Verbose STL iterator loops (`std::vector<T>::iterator it = ...`) | Range-based `for (const auto& item : container)` | Prevents iterator mismatch and off-by-one errors. |
| Out-of-line functor structs for `std::sort` / `std::find_if` | Local C++14 lambdas (`[](const A& a, const B& b) { ... }`) | Keeps sorting/filtering criteria right next to the call site. |
| Magic integer state constants (`int iState = 0, 1, 2`) | Internal `enum class` | Adds strong type safety for internal state machines. |
| C-style casts `(int)fVal` on non-trivial types | `static_cast<int>(fVal)` | Makes narrowing or type conversions explicit and searchable. |

---

## 3. Hidden Traps When Modernizing Legacy C/C++03

1. **Signed vs. Unsigned Conversion Bugs (`INT32` vs. `size_t`):**
   - Legacy code uses signed integers (`INT16`, `INT32`) for counts and loop indices, while `std::vector::size()` returns unsigned `size_t`.
   - Comparing signed loop counters with `.size()` can trigger compiler warnings or infinite underflow loops (`i >= 0` when `i` is `size_t`). Always use range-based `for` loops or explicit `static_cast<INT32>(vec.size())`.
2. **Changing Integer Widths (`INT8` / `UINT8` / `INT16`):**
   - Do not casually widen or narrow legacy integer types in existing calculations. Intentional overflow, bit-masking, or sentinel comparisons (e.g., `-1` vs `255` on `UINT8`) may be relied upon by callers.
3. **Boolean Macro Collisions (`BOOLEAN` vs. `bool`):**
   - Legacy functions return `BOOLEAN` (`UINT8` / `int` with `TRUE`/`FALSE`), whereas modern C++ uses `bool` (`true`/`false`).
   - Keep `BOOLEAN` in existing function signatures so callers do not break; use `bool` only for new internal helpers and variables.
4. **Loop Variable Mutation & Early Exits:**
   - Before converting a legacy `for (INT32 i = 0; i < count; ++i)` loop into a range-based `for`, verify that the loop body does **not** modify `i` mid-loop, skip indices, or mutate the underlying container while iterating.
5. **Macro Collisions (`min` / `max`):**
   - Always write `(std::min)(a, b)` and `(std::max)(a, b)` with parentheses to prevent `<windows.h>` / legacy macro expansion errors.

---

## 4. Recommended Incremental Workflow ("Extract, Verify, Modernize")

1. **Isolate First:** Copy the target legacy logic into a dedicated C++14 helper class or private method while preserving exact legacy inputs and outputs.
2. **Verify Parity:** Compile (`msbuild`) and verify in-game that the extracted code behaves identically to the original before changing logic.
3. **Modernize Internals:** Replace raw pointers/arrays, magic numbers, and manual loops inside the isolated helper using safe C++14 idioms.
4. **Document & Keep Diffs Small:** Keep refactoring commits **separate** from new feature commits, and mark any modified legacy hook points with `// author: description`.
