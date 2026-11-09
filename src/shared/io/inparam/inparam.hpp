// -----------------------------------------------------------------------------
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2019 - 2026 by the AxiSEM3D authors
//
// This file is part of the AxiSEM3D library. See the LICENSE file for details.
//
// -----------------------------------------------------------------------------

//  global input parameters

#ifndef inparam_hpp
#define inparam_hpp

#include "InparamYAML.hpp"

namespace inparam {
  // global input parameters
  extern InparamYAML gInparamModel;
  extern InparamYAML gInparamNr;
  extern InparamYAML gInparamSource;
  extern InparamYAML gInparamOutput;
  extern InparamYAML gInparamAdvanced;

  // global default input parameters
  extern InparamYAML gInparamModelDefault;
  extern InparamYAML gInparamNrDefault;
  extern InparamYAML gInparamSourceDefault;
  extern InparamYAML gInparamOutputDefault;
  extern InparamYAML gInparamAdvancedDefault;

  // setup
  void
  setup();

  // verbose
  std::string
  verbose();
} // namespace inparam

#endif /* inparam_hpp */
