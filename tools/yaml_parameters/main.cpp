// -----------------------------------------------------------------------------
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2019 - 2026 by the AxiSEM3D authors
//
// This file is part of the AxiSEM3D library. See the LICENSE file for details.
//
// -----------------------------------------------------------------------------

//  yaml to C++ header code generator
//
//  Embeds the default inparam yaml files into a single generated C++ header
//  that re-parses them in memory at startup. The header stores each file as a
//  string constant (the preprocessed text) and calls Yaml::Parse on it, so
//  the parameter tree is reconstructed without reading any yaml at runtime.
//
//  why embed the text instead of emitting tree-construction code: the bytes
//  embedded here are produced by InparamYAML::preprocess, the same function
//  the runtime InparamYAML::parse calls, and the header feeds them to the same
//  Yaml::Parse. There is no second representation to keep in sync, so
//  build-time and runtime parsing cannot diverge.
//
//  usage:
//    axisem3d_yaml_parameters <output.hpp> <input.yaml>...
//      process every input, write a single header with one build() per input
//    axisem3d_yaml_parameters --verify <input.yaml>...
//      process every input and verify only, write no file
//
//  this tool is a single process, so it is compiled with
//  AXISEM3D_SERIAL_BUILD and provides a no-op for the one MPI symbol
//  that InparamYAML::parse references.

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "InparamYAML.hpp"
#include "bstring.hpp"
#include "mpi.hpp"
#include "yaml/Yaml.hpp"

// InparamYAML::parse references this MPI symbol; the utility is a
// single process, so provide a no-op definition instead of linking
// mpi.cpp (which would drag in io:: and friends)
namespace mpi {
  void
  bcast(std::vector<std::string>&, int) {
    // nothing to broadcast in a single process
  }
} // namespace mpi

namespace {

  // ****** escape ******

  // escape a string into a C++ string literal. Every byte that would be
  // ambiguous inside a literal is escaped; all other bytes pass through
  // unchanged, so the result is lossless.
  std::string
  escapeLiteral(const std::string& in) {
    std::string out = "\"";
    for (unsigned char c : in) {
      switch (c) {
        case '"':
          out += "\\\"";
          break;
        case '\\':
          out += "\\\\";
          break;
        case '\n':
          out += "\\n";
          break;
        case '\t':
          out += "\\t";
          break;
        case '\r':
          out += "\\r";
          break;
        default:
          if (c < 0x20) {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\x%02x", c);
            out += buf;
          } else {
            out += (char)c;
          }
          break;
      }
    }
    out += "\"";
    return out;
  }

  // reverse of escapeLiteral. Used only to verify that escapeLiteral is
  // lossless: unescape(escape(text)) must equal text byte-for-byte.
  int
  hexValue(char c) {
    if (c >= '0' && c <= '9') {
      return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
      return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
      return c - 'A' + 10;
    }
    throw std::runtime_error("axisem3d_yaml_parameters || "
        "invalid hex digit in unescapeLiteral");
  }

  std::string
  unescapeLiteral(const std::string& lit) {
    if (lit.size() < 2 || lit.front() != '"' || lit.back() != '"') {
      throw std::runtime_error("axisem3d_yaml_parameters || "
          "expected a quoted string literal in unescapeLiteral");
    }
    const std::string& body = lit.substr(1, lit.size() - 2);
    std::string out;
    for (size_t i = 0; i < body.size(); i++) {
      if (body[i] != '\\') {
        out += body[i];
        continue;
      }
      // an escape sequence follows
      if (i + 1 >= body.size()) {
        throw std::runtime_error("axisem3d_yaml_parameters || "
            "trailing backslash in unescapeLiteral");
      }
      i++;
      const char e = body[i];
      switch (e) {
        case 'n':
          out += '\n';
          break;
        case 't':
          out += '\t';
          break;
        case 'r':
          out += '\r';
          break;
        case '"':
          out += '"';
          break;
        case '\\':
          out += '\\';
          break;
        case 'x': {
          if (i + 2 >= body.size()) {
            throw std::runtime_error("axisem3d_yaml_parameters || "
                "incomplete \\x escape in unescapeLiteral");
          }
          const int hi = hexValue(body[i + 1]);
          const int lo = hexValue(body[i + 2]);
          out += (char)((hi * 16 + lo) & 0xff);
          i += 2;
          break;
        }
        default:
          // escapeLiteral never emits these; reaching here means a
          // round-trip failure
          throw std::runtime_error("axisem3d_yaml_parameters || "
              "unexpected escape in unescapeLiteral");
      }
    }
    return out;
  }

  // ****** namespace ******

  // derive the C++ namespace from the input file name:
  //   inparam.<stem>.yaml  ->  <stem>Default
  // (model -> modelDefault, nr -> nrDefault, source -> sourceDefault,
  // output -> outputDefault, advanced -> advancedDefault)
  std::string
  makeNamespace(const std::string& path) {
    // basename
    const std::string::size_type slash = path.find_last_of("/\\");
    const std::string base =
        (slash == std::string::npos) ? path : path.substr(slash + 1);
    // expect inparam.<stem>.yaml
    const std::string prefix = "inparam.";
    const std::string suffix = ".yaml";
    if (base.size() <= prefix.size() + suffix.size() ||
        base.compare(0, prefix.size(), prefix) != 0 ||
        base.compare(base.size() - suffix.size(), suffix.size(), suffix) != 0) {
      throw std::runtime_error("axisem3d_yaml_parameters || "
          "input file must be named inparam.<stem>.yaml || " + path);
    }
    const std::string stem =
        base.substr(prefix.size(), base.size() - prefix.size() - suffix.size());
    return stem + "Default";
  }

  // ****** process ******

  // read one input file, run the shared preprocessor, verify the result, and
  // return the escaped literal of the preprocessed text. The preprocessed
  // text is what the runtime InparamYAML::parse produces before Yaml::Parse,
  // so embedding it guarantees the generated header parses identically.
  std::string
  processFile(const std::string& fname, const std::string& namespaceName) {
    // read lines (single process; no broadcast needed)
    std::vector<std::string> lines = bstring::readLines(fname, "axisem3d_yaml_parameters");

    // preprocess with the shared function (the same bytes the runtime makes)
    const std::string& text = InparamYAML::preprocess(lines, fname);

    // verify 1: the escape round-trips byte-for-byte. This guards the one real
    // risk of embedding text, a lossy escape, so a future bug fails the build
    // instead of silently corrupting the parameters.
    const std::string escaped = escapeLiteral(text);
    if (unescapeLiteral(escaped) != text) {
      throw std::runtime_error("axisem3d_yaml_parameters || "
          "escape round-trip FAILED || " + fname);
    }

    // verify 2: the bytes the header embeds parse to a non-None root, exactly
    // as they will at runtime. We feed the unescaped embedded content (std::string,
    // so it binds the same Yaml::Parse overload the runtime and build() use),
    // not the raw pre-escape text, so this checks the actual code path. A parse
    // failure here would also fail at startup, so it is surfaced at build time.
    Yaml::Node root;
    try {
      Yaml::Parse(root, std::string(unescapeLiteral(escaped)));
    } catch (const std::exception& e) {
      throw std::runtime_error("axisem3d_yaml_parameters || "
          "embedded text does not parse || " + fname + " || " + e.what());
    }
    if (root.IsNone()) {
      throw std::runtime_error("axisem3d_yaml_parameters || "
          "parsed root is None (empty yaml?) || " + fname);
    }

    (void)namespaceName;
    return escaped;
  }

  // ****** generate ******

  // generate the complete header: one build() per input, each re-parsing its
  // embedded text with Yaml::Parse. The inputs are processed (and verified)
  // before any of them is written, so a failure aborts with no partial file.
  std::string
  generateHeader(
      const std::vector<std::string>& paths,
      const std::vector<std::string>& namespaces,
      const std::vector<std::string>& literals) {
    std::ostringstream os;
    os << "// -----------------------------------------------------------------------------\n"
       << "//\n"
       << "// SPDX-License-Identifier: MIT\n"
       << "// Copyright (C) 2019 - 2026 by the AxiSEM3D authors\n"
       << "//\n"
       << "// This file is part of the AxiSEM3D library. See the LICENSE file for details.\n"
       << "//\n"
       << "// -----------------------------------------------------------------------------\n"
       << "//\n"
       << "//  auto-generated by axisem3d_yaml_parameters from\n";
    for (const std::string& path : paths) {
      os << "//  " << path << "\n";
    }
    os << "//  do not edit this file by hand; it is regenerated at build time\n"
       << "//\n"
       << "//  each default is the embedded preprocessed text of its source file,\n"
       << "//  re-parsed in memory at startup. the embedded bytes are produced by\n"
       << "//  the same InparamYAML::preprocess the runtime uses, so the build-time\n"
       << "//  and runtime trees are identical.\n"
       << "//\n"
       << "\n"
       << "#pragma once\n"
       << "\n"
       << "#include \"yaml/Yaml.hpp\"\n"
       << "\n"
       << "namespace inparam {\n"
       << "\n";
    for (size_t i = 0; i < namespaces.size(); i++) {
      os << "namespace " << namespaces[i] << " {\n"
         << "\n"
         << "  //  the parameter tree of the source yaml, re-parsed in memory\n"
         << "  inline Yaml::Node\n"
         << "  build() {\n"
         << "    Yaml::Node root;\n"
         // wrap the literal in std::string: a bare const char* would bind to
         // Yaml::Parse(Node&, const char* filename) (open a file) instead of
         // the string overload, so this cast is what makes build() use the
         // same overload the runtime InparamYAML::parse uses
         << "    Yaml::Parse(root, std::string(" << literals[i] << "));\n"
         << "    return root;\n"
         << "  }\n"
         << "\n"
         << "} // namespace " << namespaces[i] << "\n"
         << "\n";
    }
    os << "} // namespace inparam\n";
    return os.str();
  }

  int
  doGenerate(
      const std::string& outFname,
      const std::vector<std::string>& inFnames) {
    // process every input first (each verifies the round-trip and the parse),
    // so a single bad file aborts before any output is written
    std::vector<std::string> paths;
    std::vector<std::string> namespaces;
    std::vector<std::string> literals;
    for (const std::string& fname : inFnames) {
      paths.push_back(fname);
      const std::string ns = makeNamespace(fname);
      namespaces.push_back(ns);
      literals.push_back(processFile(fname, ns));
    }

    // generate
    const std::string header = generateHeader(paths, namespaces, literals);

    // write
    std::ofstream out(outFname);
    if (!out) {
      throw std::runtime_error("axisem3d_yaml_parameters || "
          "Cannot open output file for writing || " + outFname);
    }
    out << header;
    out.close();
    if (!out) {
      throw std::runtime_error("axisem3d_yaml_parameters || "
          "Error writing output file || " + outFname);
    }

    std::cout << "axisem3d_yaml_parameters || generated " << outFname << std::endl;
    return 0;
  }

  int
  doVerify(const std::vector<std::string>& inFnames) {
    int rc = 0;
    for (const std::string& fname : inFnames) {
      try {
        const std::string ns = makeNamespace(fname);
        processFile(fname, ns);
        std::cout << "axisem3d_yaml_parameters || verify OK || Input yaml file: "
            << fname << std::endl;
      } catch (const std::exception& e) {
        std::cerr << "axisem3d_yaml_parameters || verify FAILED || "
            "Input yaml file: " << fname << " || " << e.what() << std::endl;
        rc = 1;
      }
    }
    return rc;
  }

} // anonymous namespace

int
main(int argc, char* argv[]) {
  try {
    // usage:
    //   axisem3d_yaml_parameters <output.hpp> <input.yaml>...
    //   axisem3d_yaml_parameters --verify <input.yaml>...
    if (argc < 3) {
      std::cerr << "Usage:\n"
          << "  axisem3d_yaml_parameters <output.hpp> <input.yaml>...\n"
          << "  axisem3d_yaml_parameters --verify <input.yaml>...\n";
      return 1;
    }
    if (std::string(argv[1]) == "--verify") {
      std::vector<std::string> inputs;
      for (int i = 2; i < argc; i++) {
        inputs.push_back(argv[i]);
      }
      return doVerify(inputs);
    }
    const std::string outFname = argv[1];
    std::vector<std::string> inputs;
    for (int i = 2; i < argc; i++) {
      inputs.push_back(argv[i]);
    }
    return doGenerate(outFname, inputs);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
