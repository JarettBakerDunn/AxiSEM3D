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
#include "inparam_defaults.hpp"

namespace inparam {
  // global input parameters
  InparamYAML gInparamModel("Model");
  InparamYAML gInparamNr("Nr");
  InparamYAML gInparamSource("Source");
  InparamYAML gInparamOutput("Output");
  InparamYAML gInparamAdvanced("Advanced");

  // global default input parameters
  InparamYAML gInparamModelDefault("Model_Default");
  InparamYAML gInparamNrDefault("Nr_Default");
  InparamYAML gInparamSourceDefault("Source_Default");
  InparamYAML gInparamOutputDefault("Output_Default");
  InparamYAML gInparamAdvancedDefault("Advanced_Default");

  // setup
  void
  setup() {
    // parse
    gInparamModel.parse(io::gInputDirectory + "/inparam.model.yaml");
    gInparamNr.parse(io::gInputDirectory + "/inparam.nr.yaml");
    gInparamSource.parse(io::gInputDirectory + "/inparam.source.yaml");
    gInparamOutput.parse(io::gInputDirectory + "/inparam.output.yaml");
    gInparamAdvanced.parse(io::gInputDirectory + "/inparam.advanced.yaml");

    // load defaults from inparam_defaults.hpp
    gInparamModelDefault.loadFromNode(modelDefault::build());
    gInparamNrDefault.loadFromNode(nrDefault::build());
    gInparamSourceDefault.loadFromNode(sourceDefault::build());
    gInparamOutputDefault.loadFromNode(outputDefault::build());
    gInparamAdvancedDefault.loadFromNode(advancedDefault::build());

    // fill missing keys in the model inparam with their defaults
    gInparamModel.fillFrom(gInparamModelDefault);
    gInparamNr.fillFrom(gInparamNrDefault);
    gInparamSource.fillFrom(gInparamSourceDefault);
    gInparamOutput.fillFrom(gInparamOutputDefault);
    gInparamAdvanced.fillFrom(gInparamAdvancedDefault);

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
