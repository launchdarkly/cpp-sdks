# CLAUDE.md

LaunchDarkly C++ SDKs monorepo: client SDK, server SDK, server SDK integrations (Redis, OTel), plus shared libraries. See `README.md` for the build/install reference and `CONTRIBUTING.md` for the style basics (Google C++ style, C++17, C99).

## Build & test operational notes

Full reference: `README.md`. A few things that aren't obvious from it:

- **`cmake -B build -S .`** — always pass an out-of-source build directory with `-B`. Don't run cmake with `.` as the build directory; it doesn't work cleanly here.
- **Multiple build configurations can coexist as sibling build directories**, e.g. one with sanitizers on, one with `LD_CURL_NETWORKING=ON`. A common local convention is `build-nosan/` and `build-curl/`, but don't assume those directories exist — list the directory first before using one.
- **If test binaries hang at startup with no output**, that's an intermittent ASAN issue seen on some platforms. Try a no-sanitizer build (`-DLD_TESTING_SANITIZERS=OFF`) to confirm. Not universal — don't assume it's always the cause.
- **When SSE or HTTP networking code changes**, validate against both backends: Boost.Beast/Foxy (default) and CURL (`-DLD_CURL_NETWORKING=ON`).
- **Always wrap test runs with `timeout`**: e.g. `timeout 15 <build>/gtest_launchdarkly-cpp-internal`. Deadlocks don't fail GTest — they hang forever.
- **After adding a brand-new test file, re-run `cmake -S . -B <build-dir>`** before `cmake --build`. The test CMakeLists use `file(GLOB)`, which only re-globs at configure time; without a re-configure, the new file is silently skipped.
- Test target names: `gtest_launchdarkly-cpp-{common,internal,client,server,sse-client}`. Note the `-cpp-server` form — NOT `gtest_launchdarkly-server-sdk`.
- Test binaries land at the build root (`<build>/gtest_…`), not nested under the source tree's `tests/` directory.

## Client vs server SDKs

`libs/client-sdk` and `libs/server-sdk` are distinct SDKs sharing libraries (`libs/common`, `libs/internal`, `libs/server-sent-events`). Public APIs and data models differ. Always state which SDK(s) a change applies to — the answer is rarely "both," even when the feature name is shared.

## Project layout

- `libs/` — SDK and shared libraries.
- `architecture/` — design docs for major subsystems; read before non-trivial changes in those areas.
- `vendor/` — third-party source with project-specific patches; don't edit casually.
- `contract-tests/`, `examples/`, `scripts/`, `cmake/`, `cmake-tests/` — as named.

## Coding conventions

Basics (Google C++ style, naming, `#pragma once`, `.cpp/.hpp` for C++, `.c/.h` for C) are in `CONTRIBUTING.md`. Repo-specific rules below.

### Includes

- `""` for headers in this library's own source tree (sibling headers, private internal headers).
- `<>` for the library's installed public API (`<launchdarkly/...>`), third-party (Boost, etc.), and standard library.
- Group order: std/system, then third-party, then `<launchdarkly/...>`, then `"local"`. Blank line between groups. (Matches existing code.)

### Public APIs

- Anything under `libs/*/include/launchdarkly/...` is public. Avoid breaking changes. If a public signature must change, call it out explicitly in the PR description.
- Shared-library builds export only the C API. Don't change symbol visibility without understanding why.

### Naming

- Public methods returning `bool` are named `IsX()` / `HasX()`, not `X()`.

### Parameters

- For mutable (out / in-out) parameters, use `T*`, not `T&`. The pointer makes mutation visible at the call site (`&foo` vs `foo`). Const references (`const T&`) for read-only are fine.
- For literal arguments (`nullptr`, `true`, `false`, integer/duration literals), prefix with `/* name= */` when the parameter's role isn't obvious. Skip when the function name + literal type already convey it (`WaitForResult(1s)`, `unique_ptr(nullptr)`, `vec.reserve(100)`).

### Comments & doc comments

- Default to one-line comments. If you're writing a paragraph, either the name needs to do more work or no comment is needed.
- Non-trivial classes need a doc comment that explicitly states the **thread-safety contract** (thread-safe / not thread-safe / partial, with specifics on which methods are safe from which contexts).
- Public methods on non-trivial classes need a doc comment explaining how callers should use them: preconditions, side effects, threading constraints, return semantics. Document **what callers need**, not implementation details (don't expose `shared_ptr`, backends, internal resource management).
- No Java references. American spellings.
- No session/roadmap labels in comments ("Step 7", "phase 2"). Use `TODO:` plus a description.

### Memory & ownership

- `shared_ptr` is **not** the default. Each use needs a concrete justification (genuine shared ownership, async callbacks outliving the owner, type-erased callable storage). Default to value, `unique_ptr`, or non-owning `T*` with a documented lifetime contract.
- Don't take a constructor parameter as `T&` and store it as a member reference. Use `shared_ptr` (with justification), `T*` plus a constructor doc comment on lifetime, or take ownership.

### Thread safety

- `boost::asio::io_context` does **not** serialize callbacks by itself — it can be run from multiple threads. When a thread-safety comment relies on serialization, name the actual mechanism: strand, mutex, single-thread invariant, or sequential `.Then()` chaining.
- Inside thread-safe classes, group member variables by what protects them (`const`, `protected by mutex_`, etc.) and annotate each group.
- Default lock scope is the **whole function** with one `std::lock_guard` at the top. Only narrow with a stated reason (re-entrant callback, blocking I/O, lock ordering) — write the reason in a comment.
- A pointer/reference derived from mutex-protected state inherits the protection. Don't copy a raw pointer out under the lock and use it after releasing.

### Futures

- When chaining with `Future::Then`, use the flattening overload. Don't capture a `Promise` in the continuation to resolve manually.

### Source organization

- Method definitions in `.cpp` follow the order of declarations in the `.hpp`.

## Tests

- GoogleTest. Match the style of the adjacent test file you're modifying.
- Default to inline construction of test data. Helpers are acceptable when they *substantially* reduce boilerplate repeated across many tests — not just for "I might call this twice."
- Don't sleep then assert a negative ("future didn't fire", "callback not called"). If nothing else can resolve the future, check state synchronously.
- Skip `// Act` / `// Assert` labels — write a short descriptive comment of what the block does or expects.
- Don't pad timer durations "for CI safety" in drain-then-assert tests; 5ms is fine when there's no race to lose.

## Docs

Doxygen, configured per-library via a `Doxyfile`. Build with `scripts/build-docs.sh <library-dir>`. Public APIs get Doxygen comments describing behavior callers depend on — not implementation details.

## Formatting

`clang-format -i <file>` (config is in the repo).

## Process

- **Commit messages:** single-line conventional commits (`feat:`, `fix:`, `refactor:`, `docs:`, `chore:`, `test:`). No body unless asked.
- **PR descriptions:** terse. One-sentence summary, a few bullets for what's in, design decisions only for non-obvious choices. Add a "not in scope" line only when a reviewer would reasonably expect that thing to be in this PR — don't manufacture exclusions.
