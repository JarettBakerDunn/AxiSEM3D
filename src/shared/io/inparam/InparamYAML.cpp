// -----------------------------------------------------------------------------
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2019 - 2026 by the AxiSEM3D authors
//
// This file is part of the AxiSEM3D library. See the LICENSE file for details.
//
// -----------------------------------------------------------------------------

//  YAML parser for input parameters
//  based on mini-yaml:
//  https://github.com/jimmiebergmann/mini-yaml
#include <cctype>

#include "InparamYAML.hpp"
#include "mpi.hpp"

// Internal helper functions for InparamYAML
namespace {

  // check if a yaml node has a key but no value
  bool
  isUnset(const Yaml::Node& node) {
    if (node.IsNone()) {
      return true;
    }
    if (!node.IsScalar()) {
      return false;
    }
    // if the node's value contains something other than a space,
    // it must be set to something
    for (unsigned char c : node.As<std::string>()) {
      if (!std::isspace(c)) {
        return false;
      }
    }
    return true;
  }

  // find the child of a map node which corresponds to the given key, if it exists
  Yaml::Node*
  findChild(Yaml::Node& mapNode, const std::string& key) {
    for (auto it = mapNode.Begin(); it != mapNode.End(); it++) {
      if ((*it).first == key) {
        return &((*it).second);
      }
    }
    return nullptr;
  }

  // Fill a node in inputParameter with values from defaultParameter.
  // Recurses if the node has child nodes.
  void
  fillNode(const Yaml::Node& defaultParameter, Yaml::Node& inputParameter) {
    // If there is no default for this parameter, there is nothing to write into
    // the inputParameter.
    if (isUnset(defaultParameter)) {
      return;
    }

    if (defaultParameter.IsScalar()) {
      if (isUnset(inputParameter)) {
        inputParameter = defaultParameter.As<std::string>();
      }
      return;
    }

    if (defaultParameter.IsMap()) {
      // if the default parameter is a map and the input parameter is unset,
      // set the input parameter to the default
      if (isUnset(inputParameter)) {
        inputParameter = defaultParameter;
        return;
      }
      // if the default is a map, but the input for some reason is not, assume
      // the user knows what they're doing and don't replace anything.
      if (!inputParameter.IsMap()) {
        return;
      }
      // iterate through the defaultParameter, replacing missing nodes in inputParameter
      for (auto it = defaultParameter.Begin(); it != defaultParameter.End(); it++) {
        const std::string& key = (*it).first;
        Yaml::Node* found = findChild(inputParameter, key);
        // if the key exists in inputParameter, recurse
        if (found) {
          fillNode((*it).second, *found);
        } 
        // if the key does not exist in inputParameter, 
        // and is not empty in the defaultParameter, copy from the default
        else if (!isUnset((*it).second)) {
          inputParameter[key] = (*it).second;
        }
      }
      return;
    }
    // if the sequence in inputParameter is unset, copy from default
    if (defaultParameter.IsSequence() && isUnset(inputParameter)) {
        inputParameter = defaultParameter;
        return;
    }
  }
} // namespace

// parse
void
InparamYAML::parse(const std::string& fname) {
  // read input file
  std::vector<std::string> lines;
  if (mpi::root()) {
    lines = bstring::readLines(fname, "InparamYAML::parse");
  }
  mpi::bcast(lines);

  // clean up text before parsing it
  const std::string& strArr = preprocess(lines, fname);

  // parse input
  try {
    Yaml::Parse(mRoot, strArr);
  } catch (...) {
    throw std::runtime_error("InparamYAML::parse || "
                             "Error parsing YAML by mini-yaml || "
                             "Input yaml file: || " +
        fname);
  }
}

// Clean up input YAML text
std::string
InparamYAML::preprocess(std::vector<std::string>& lines, const std::string& label) {
  // remove comments
  for (std::string& line : lines) {
    line = line.substr(0, line.find('#'));
  }

  // change array [] into "-" because mini-yaml does not support []
  for (std::string& line : lines) {
    int front = (int)line.find('[');
    if (front == std::string::npos) {
      continue;
    }
    // find ']'
    int back = (int)line.find(']', front);
    if (back == std::string::npos) {
      throw std::runtime_error("InparamYAML::parse || "
                               "Array brackets [] does not match. || "
                               "Input yaml file: || " +
          label);
    }
    // split
    const std::string& str = line.substr(front + 1, back - front - 1);
    const std::vector<std::string>& vec = bstring::split(str, ",");
    if (vec.front() == "") {
      // empty sequence
      line = line.substr(0, front) + "YAML_EMPTY_SEQUENCE";
    } else {
      // write header in this line
      std::string converted = line.substr(0, front) + "\n";
      // write elements with -
      for (const std::string& elem : vec) {
        if (elem.find(":") != std::string::npos) {
          throw std::runtime_error("InparamYAML::parse || "
                                   "Array given in [] must contain "
                                   "non-object elements. || "
                                   "Input yaml file: || " +
              label);
        }
        // use location of '[' as indent
        converted += bstring::filled(front) + "- " + elem + "\n";
      }
      line = converted;
    }
  }

  // merge lines
  std::string strArr;
  for (const std::string& line : lines) {
    strArr += line + "\n";
  }

  return strArr;
}

// fill entires from another YAML parameter tree, typically from the default 
void
InparamYAML::fillFrom(const InparamYAML& src) {
  fillNode(src.mRoot, mRoot);
}

// verbose
std::string
InparamYAML::verbose() const {
  // get contents
  std::string contents;
  Yaml::Serialize(mRoot, contents);

  // add indent
  contents = "  " + bstring::replace(contents, "\n", "\n  ");
  contents = contents.substr(0, contents.size() - 2);

  // add subtitle
  std::stringstream ss;
  ss << bstring::boxSubTitle(0, mName);
  ss << contents;
  return ss.str();
}
