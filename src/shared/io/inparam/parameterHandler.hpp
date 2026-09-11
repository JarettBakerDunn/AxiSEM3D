// -----------------------------------------------------------------------------
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2019 - 2026 by the AxiSEM3D authors
//
// This file is part of the AxiSEM3D library. See the LICENSE file for details.
//
// -----------------------------------------------------------------------------

// Class which tracks all input parameters to AxiSEM3D.
// When adding new functionality to AxiSEM3D, any new parameters should
// be documented here. This will also set the defaults

#ifndef parameterHandler_hpp
#define parameterHandler_hpp

#include "bstring.hpp"
#include <map>
#include "yaml/Yaml.hpp"
#include "io.hpp"

namespace inparam {
    class ParameterHandler {
      public: 
        Yaml::Node rootModel;

        // Since the parameter types from the "type" seection in the inparam.yaml
        // files don't always correspond to c++ types, an enum seems appropriate
        enum class parameterTypes {
          mathEquation,
          string,
          number, //cannot use an actual typename like "double"
          boolean,
          stringArray,
          numberArray,
          integer,
          filename
        };

        ParameterHandler(){
          Yaml::Node & ref = rootModel;


          // Tests
          declareParameter(std::string("absorbing_boundary:boundaries"),
                          "model boundaries regarded as absorbing boundaries",
                          parameterTypes::stringArray,
                          "RIGHT,BOTTOM,TOP",
                          "RIGHT,BOTTOM",
                          "1) an AxiSEM3D mesh may contain four outer boundaries: left (axial), right, bottom and top (surface); the right, bottom and top ones can be absorbing boundaries (the left or axial one is non-physical) 2) use [] to disable absorbing boundary condition(so that all model boundaries will be stress-free) 3) the most common case in seismology is [RIGHT, BOTTOM]",
                          rootModel);
          declareParameter(std::string("absorbing_boundary:Kosloff_Kosloff:enable"),
                          "enable the Kosloff-Kosloff approach",
                          parameterTypes::boolean,
                          "boolean", //acceptableValues doesn't mean anything here since the type is a boolean
                          "true",
                          "Clayton-Enquist and Kosloff-Kosloff can be used together, but one of them has to be enabled at least",
                          rootModel);

        }
        /**
         * Declare an input parameter to let AxiSEM3D know to generate the documentation for 
         * that parameter.
         * @param keyword The name of the parameter to declare. Use a : as a delimeter between
         * different subsections and the parameter name. 
         * For example: "subsection1:subsection2:parameterName"
         * @param description A short description of the parameter. The "what" comment description
         * in an inparam.yaml file.
         * @param type The input type or types of the parameter.
         * @param acceptableValues corresponds to the "only" comment description in an 
         * inparam.yaml file. If there are multiple acceptable input types, then fill out
         * the acceptableValues for each type in the same order that the types are listed.
         * @param defaultValue The default value or values of the parameter. If there are multiple
         * acceptable input types, then fill out the default value for each in the same order
         * that the input types were specified.
         * @param note Any notes about this input parameter. There is a corresponding section
         * in each inparam.yaml file.
         * @param node The root yaml node for which to declare the parameter. Corresponds to one of the 
         * inparam.yaml files: advanced, model, nr, output, or source.
         */
        

        // These versions are overloaded to make it simpler when a parameter only has a single type.
        void declareParameter(std::string keyword, 
                              std::string description, 
                              parameterTypes type, 
                              std::string acceptableValues, 
                              std::string defaultValue, 
                              std::string note, 
                              Yaml::Node& node);
        void declareParameter(std::vector<std::string> keyword, 
                              std::string description, 
                              parameterTypes type, 
                              std::string acceptableValues, 
                              std::string defaultValue, 
                              std::string note, 
                              Yaml::Node& node);
        /**
         * 
         */
        void writeAllParameters();

  };
}
/*

Model___________________________________________________________
  absorbing_boundary: 
    Kosloff_Kosloff: 
      enable: true
      gamma_expr_fluid: "0.88 / T0 * exp(-0.04 * SPAN / (VP * T0))"
      gamma_expr_solid: "1.1 / T0 * (VS / VP)^2 * exp(-0.04 * SPAN / (VP * T0))"
      relative_spans: 
        - .05
        - .05
    boundaries: 
      - RIGHT
      - BOTTOM
    enable_Clayton_Enquist: true
  attenuation: NONE
  geodesy: 
    flattening_on_surface: WGS84
    lat_lon_north_pole_mesh: SOURCE
  list_of_3D_models: YAML_EMPTY_SEQUENCE
  model1D: 
    exodus_mesh: global_mesh__prem_ani__50s.e

*/





#endif /* inparam_hpp */
