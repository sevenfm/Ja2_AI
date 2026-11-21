# Workspace Instructions for AI Agents

## 1. Mandatory Coding Standard
- Before writing or modifying any code, read and strictly follow `CODING_GUIDELINES.md` in the repository root.
- **Summary of non-negotiable rules:**
  - Isolate all new feature logic in dedicated `.h` and `.cpp` files inside a new class (ISO C++14, MSVC `v143`).
  - Keep existing legacy files strictly ISO C++03 (`NULL`, legacy typedefs, no `auto`/`nullptr`/lambdas).
  - Never add new global variables or free global functions.
  - Mark every edit in an existing legacy file with `// author: description` (or `// author: begin` / `// author: end`).
  - Only modify `.vcxproj` and `.vcxproj.filters` to register newly created `.h` and `.cpp` files—never change compiler or project settings.

---

## 2. Repository Search & Context Boundaries
This repository is a large (~1.12M LOC) legacy C/C++ project. To avoid wasting context tokens and slowing down searches:
- **Always scope file searches (`grep`, symbol lookup, directory listing)** to the specific module directories relevant to the active task or feature specification.
- **Do NOT scan or read unrelated subsystems** (such as UI/front-end screens, map/world editors, multiplayer networking, low-level platform/graphics wrappers, or third-party external libraries) unless explicitly instructed by the user.
- When inspecting simulation headers for struct layouts or function signatures, read only the specific declarations needed rather than dumping entire large files into context.

---

## 3. Execution & Build Workflow
1. **Plan First, Code in Phases:** Break implementation into small, logical phases (e.g., class interface & scoring -> state execution -> legacy hook injection).
2. **Verify Compilation Continuously:** After completing each phase, run `msbuild` via the terminal on the target project/solution and resolve all compiler errors and warnings before moving to the next phase.
3. **Never Guess Legacy Signatures:** Always verify exact struct fields, enum values, macro definitions, and function signatures in existing headers before writing hook calls.
4. **Preserve Side-Effect Safety:** Verify whether existing legacy functions mutate global buffers or entity state before calling them inside evaluation loops.
