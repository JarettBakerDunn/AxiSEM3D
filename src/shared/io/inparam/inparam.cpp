// -----------------------------------------------------------------------------
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2019 - 2026 by the AxiSEM3D authors
//
// This file is part of the AxiSEM3D library. See the LICENSE file for details.
//
// -----------------------------------------------------------------------------

//  global input parameters

#include "inparam.hpp"
#include "io.hpp"
#include "parameterHandler.hpp"

namespace inparam {
  // global input parameters
  InparamYAML gInparamModel("Model");
  InparamYAML gInparamNr("Nr");
  InparamYAML gInparamSource("Source");
  InparamYAML gInparamOutput("Output");
  InparamYAML gInparamAdvanced("Advanced");
  

  // setup
  void
  setup() {
    // parse
    gInparamModel.parse(io::gInputDirectory + "/inparam.model.yaml");
    gInparamNr.parse(io::gInputDirectory + "/inparam.nr.yaml");
    gInparamSource.parse(io::gInputDirectory + "/inparam.source.yaml");
    gInparamOutput.parse(io::gInputDirectory + "/inparam.output.yaml");
    gInparamAdvanced.parse(io::gInputDirectory + "/inparam.advanced.yaml");
    // parameterHandler only exists to write all possible parameters to YAML
    ParameterHandler parameterHandler;
    parameterHandler.writeAllParameters();
  }

  // verbose
  std::string
  verbose() {
    std::stringstream ss;
    ss << bstring::boxTitle("Parameters");
    ss << gInparamModel.verbose();
    ss << gInparamNr.verbose();
    ss << gInparamSource.verbose();
    ss << gInparamOutput.verbose();
    ss << gInparamAdvanced.verbose();
    ss << bstring::boxBaseline() << "\n\n";
    return ss.str();
  }
} // namespace inparam
