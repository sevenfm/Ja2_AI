# JA2_AI Coding Guidelines & AI Agent Rules

This document defines the architectural, C++ dialect, and safety standards for developing new features in `ja2_AI`. All contributors and AI coding assistants must follow these rules to preserve legacy stability while keeping new code clean and maintainable.

---

## 1. Core Architectural Principle: Isolate & Inject

Do not rewrite, reformat, or refactor existing legacy logic unless explicitly instructed. Build new features using **strict encapsulation and minimal seam injection**:

1. **Dedicated Files for New Features:** Create separate `.h` and `.cpp` files for every new feature.
2. **Class Encapsulation:** Encapsulate all new data structures, evaluation logic, and state machines inside a dedicated C++ class.
3. **Minimal Legacy Hooks:** Modify existing legacy `.cpp`/`.h` files *only* to inject minimal hook calls into your new class (typically 1–5 lines per hook). Never embed complex new conditional logic directly inside large legacy procedural functions.
4. **No New Globals:** Do **not** introduce new global variables or free global functions. Keep internal helper methods `private` within your class or inside an anonymous `namespace` in your new `.cpp` file.

---

## 2. Dual-Dialect C++ Rules (MSVC v143 / VS2022)

### A. New Feature Files (`.h` / `.cpp`) — ISO C++14
- **Target Standard:** ISO C++14 (MSVC `v143` default).
- **Forbidden (No C++17/20):** Do not use `std::optional`, `std::string_view`, `std::clamp`, structured bindings (`auto [a, b]`), `if constexpr`, or `std::filesystem`.
- **Encouraged for Readability & Safety:** `nullptr`, `auto` (when the type is obvious), range-based `for`, simple lambdas (e.g., for `std::sort` or `std::find_if`), `std::vector`, `std::array`, `std::unique_ptr`, `enum class`, and small plain `struct`s.
- **Public Header Hygiene:** Keep the public interface of new `.h` files clean, minimal, and free of heavy STL template bloat so legacy files including them remain fast to compile and easy to read.

### B. Existing Legacy Files — Strict ISO C++03 Style
- Whenever editing an existing file to insert a hook, strictly match the surrounding **C / C++03** style.
- **Never** introduce `nullptr`, `auto`, lambdas, `enum class`, or modern STL headers into existing legacy files. Always use `NULL`, legacy boolean/integer typedefs, and raw pointers matching the surrounding code.

---

## 3. Working Safely with Legacy Code, Macros & Global State

This codebase relies heavily on preprocessor macros, shared global arrays, and stateful C-style functions. Follow these defensive rules:

1. **Assume Legacy Functions Have Hidden Side Effects:**
   - Many legacy functions silently mutate global scratch buffers, path caches, or entity state as side effects.
   - Never call stateful legacy functions inside candidate-evaluation loops where they could clobber active state belonging to the caller.
2. **Macro Collision Protection:**
   - Windows and legacy project headers define global macros (such as `min`, `max`, `TRUE`, `FALSE`, `NEAR`, `FAR`, and `small`).
   - Always parenthesize standard min/max calls—`(std::min)(a, b)` and `(std::max)(a, b)`—and avoid naming variables or methods after common Windows/legacy macro identifiers.
3. **Header Include Order:**
   - In new `.cpp` files, include standard C++ headers (`<vector>`, `<algorithm>`, etc.) **before** legacy project headers so legacy macros do not corrupt standard library internals.
4. **Legacy Types & Sentinels at Boundaries:**
   - Pass and return standard project types and sentinel constants at all hook boundaries.
   - Always validate pointers and sentinel bounds before invoking legacy functions.
5. **Respect Session & Savegame Lifecycles:**
   - Because the engine serializes specific structures for save/load, any stateful member data inside a new class must either be stateless across turns/frames or explicitly reset at well-defined session/turn lifecycle boundaries.

---

## 4. Mandatory Documentation of Legacy Edits

Every single modification made to an existing legacy file **must** be documented inline with a comment identifying the author/feature and purpose:

```cpp
// author: description of change
```

For multi-line hook blocks, wrap the insertion clearly:
```cpp
// author: begin - route evaluation to CNewFeature
if ( CNewFeature::Evaluate( pEntity ) )
{
    return ACTION_RESULT;
}
// author: end
```

*(Replace `author` with your contributor handle or feature tag so all legacy hooks can be located via a single text search.)*

---

## 5. Project & Build File Rules

- **Minimal `.vcxproj` Edits:** Do **not** alter compiler flags, preprocessor definitions, `<LanguageStandard>`, or build configurations in project files.
- **File Registration Only:** The *only* permitted modification to `.vcxproj` and `.vcxproj.filters` files is registering newly created `.h` and `.cpp` feature files.
