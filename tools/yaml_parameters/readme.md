# yaml_parameters

Compiles the default inparam yaml files into a single generated C++ header
(`inparam_defaults.hpp`) at build time. Each default is embedded as a string
constant (its preprocessed text) and re-parsed in memory at startup, so the
parameter tree is reconstructed with no yaml read, no file I/O, and no MPI
broadcast at runtime.

## Why embed the text rather than emit tree-construction code

The bytes embedded in the header are produced by `InparamYAML::preprocess` —
the same function the runtime `InparamYAML::parse` calls — and the header feeds
them to the same `Yaml::Parse`. There is no second representation to keep in
sync, so build-time and runtime parsing cannot diverge. This is the invariant
the whole design exists to guarantee.

## Files

- `tools/yaml_parameters/main.cpp` — codegen utility. Lives outside `src/` so
  the `src/*.cpp` glob cannot sweep its `main()` into the executables. It links
  `InparamYAML.cpp` + `Yaml.cpp` + `bstring.cpp`, is compiled with
  `AXISEM3D_SERIAL_BUILD`, and provides a no-op for the one MPI symbol
  (`mpi::bcast(vector<string>&, int)`) that `InparamYAML::parse` references.
- `src/shared/io/inparam/InparamYAML.hpp` / `.cpp` — added
  `static std::string preprocess(std::vector<std::string>&, const std::string&)`,
  factored out of `parse` (comment strip, `[]`→`-` with the `YAML_EMPTY_SEQUENCE`
  sentinel, merge). `parse` now calls it, so the tool and the runtime share one
  source of truth. The `label` argument appears in error messages.
- `src/shared/io/inparam/inparam.cpp` — `#include "inparam_defaults.hpp"`; in
  `setup()` each of the 5 defaults is loaded from its own namespace
  (`modelDefault::build()`, `nrDefault::build()`, `sourceDefault::build()`,
  `outputDefault::build()`, `advancedDefault::build()`), then `fillFrom` fills
  the user's inparam. Loading each default from its own file is what fixes the
  earlier bug where all 5 were loaded from the model tree.
- `CMakeLists.txt` — compiles all 5 `inparam.*.yaml` into one header.

## Usage

```
axisem3d_yaml_parameters <output.hpp> <input.yaml>...
```
processes every input, verifies each, and writes a single header containing one
`namespace <stem>Default { inline Yaml::Node build() { ... } }` per input, all
inside `namespace inparam`. The namespace is derived from the file name:
`inparam.<stem>.yaml` → `<stem>Default`.

```
axisem3d_yaml_parameters --verify <input.yaml>...
```
processes every input and verifies only, writing no file (the ctest entry point).

## What the generated header looks like

```cpp
#pragma once
#include "yaml/Yaml.hpp"

namespace inparam {
namespace modelDefault {
  inline Yaml::Node
  build() {
    Yaml::Node root;
    Yaml::Parse(root, std::string(R"axisem3d_delim(
<preprocessed text of inparam.model.yaml, verbatim>
)axisem3d_delim"));
    return root;
  }
} // namespace modelDefault
// ... nrDefault, sourceDefault, outputDefault, advancedDefault
} // namespace inparam
```

The embedded text is a raw string literal, so it appears in the generated header
verbatim (newlines included) rather than as an escaped single line. The
`std::string(...)` wrap forces `Yaml::Parse` to use its string-content overload
instead of the filename overload.

## In-tool verification

For each input, before any output is written:

1. **Parse validity** — the preprocessed text must `Yaml::Parse` to a non-None
   root. The raw literal embeds its content verbatim, so parsing the text here
   checks the exact bytes `build()` will parse at runtime. A failure here would
   also fail at startup, so it is surfaced at build time instead.
2. **Raw-literal closing sequence** — the content must not contain the literal's
   closing sequence `)axisem3d_delim"`, which would terminate the string early and break
   the build. Checked so a collision fails at codegen time (and in `--verify`
   under ctest) with a clear message, rather than as an opaque compile error.

Because the embedded text is a raw string literal, it is lossless by construction
(the compiler reads the bytes verbatim), so there is no separate losslessness
check the way a quoted-escaped literal would need.

A preprocess *bug* (valid but wrong YAML) is not catchable in the tool, because
the tool and the runtime share the same `preprocess`; it is guarded by the
runtime integration tests (screen-output diff). The in-tool checks guard only
the codegen.

## CMake

```cmake
set(INPARAM_DEFAULT_YAMLS ... 5 absolute paths ...)
set(INPARAM_DEFAULT_HPP ${CMAKE_BINARY_DIR}/generated/inparam_defaults.hpp)

add_custom_command(
  OUTPUT ${INPARAM_DEFAULT_HPP}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/generated
  COMMAND axisem3d_yaml_parameters ${INPARAM_DEFAULT_HPP} ${INPARAM_DEFAULT_YAMLS}
  DEPENDS axisem3d_yaml_parameters ${INPARAM_DEFAULT_YAMLS}
)
add_test(NAME yaml_parameters.verify
  COMMAND axisem3d_yaml_parameters --verify ${INPARAM_DEFAULT_YAMLS})
```

The header is added to the source lists of both `axisem3d` and `axisem3d-debug`,
and `${CMAKE_BINARY_DIR}/generated` is in their include paths, so the header
rebuilds when any input yaml changes and recompiles `inparam.cpp` on change.

## Verification

- Build; the header compiles into both executables.
- `ctest -R yaml_parameters.verify`.
- Full integration suite: `inparam::verbose()` is unchanged in shape, so screen
  output matches the reference (defaults only change what the user omitted).
