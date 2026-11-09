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
    Yaml::Parse(root, "<preprocessed text of inparam.model.yaml>");
    return root;
  }
} // namespace modelDefault
// ... nrDefault, sourceDefault, outputDefault, advancedDefault
} // namespace inparam
```

## In-tool verification

For each input, before any output is written:

1. **Escape round-trip** — `unescape(escape(text)) == text`, byte-for-byte.
   This guards the one real risk of embedding text (a lossy escape).
2. **Parse validity** — the preprocessed text must `Yaml::Parse` to a non-None
   root. A parse failure here would also fail at startup, so it is surfaced at
   build time instead.

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
