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
          number, // Cannot use an actual typename like "double"
          boolean,
          stringArray,
          numberArray,
          integer,
          filename,
          objectArray, // List of arbitrary keys, e.g., examples/02_3d_crust_S362ANI_regional/input_with_3d_crust
        };

        ParameterHandler(){
          Yaml::Node & ref = rootModel;


          // Testing with inparam.yaml

          // 1D model
          declareParameter(std::string("model1D:exodus_mesh"),
                          "Exodus mesh file created by salvus mesher",
                          parameterTypes::filename,
                          "",
                          "global_mesh__prem_ani__50s.e",
                          "",
                          rootModel);

          // Geodesy
          declareParameter(std::string("geodesy:lat_lon_north_pole_mesh"),
                          "geographic location of the north pole in the mesh",
                          std::vector<parameterTypes>{parameterTypes::numberArray,parameterTypes::string},
                          std::vector<std::string>{"", "SOURCE"},
                          "SOURCE",
                          "1) ellipticity is ignored for a Cartesian mesh "
                          "2) 0 for a perfect sphere; ~0.0033 for the Earth "
                          "3) ellipticity will be used in the transformation between the geographic and the geocentric co-latitudes; "
                          "see eq. (14.32) in Dahlen & Tromp, 1998 4) to actually deform the entire mesh, "
                          "add 3D model \"Ellipticity\" to list_of_3D_models",
                          rootModel);    

          declareParameter(std::string("geodesy:flattening_on_surface"),
                          "flattening on the surface",
                          std::vector<parameterTypes>{parameterTypes::string,parameterTypes::number},
                          std::vector<std::string>{"SPHERE / WGS84 / GRS80 / SPECFEM3D_GLOBE", "SOURCE"},
                          "WGS84",
                          "1) ellipticity is ignored for a Cartesian mesh "
                          "2) 0 for a perfect sphere; ~0.0033 for the Earth "
                          "3) ellipticity will be used in the transformation between the geographic and the geocentric co-latitudes; "
                          "see eq. (14.32) in Dahlen & Tromp, 1998 4) to actually deform the entire mesh, "
                          "add 3D model \"Ellipticity\" to list_of_3D_models",
                          rootModel);    
          
          // Absorbing Boundary
          declareParameter(std::string("absorbing_boundary:boundaries"),
                          "model boundaries regarded as absorbing boundaries",
                          parameterTypes::stringArray,
                          "RIGHT/BOTTOM/TOP",
                          "RIGHT,BOTTOM",
                          "1) an AxiSEM3D mesh may contain four outer boundaries: left (axial), right, bottom and top (surface); the right, bottom and top ones can be absorbing boundaries (the left or axial one is non-physical) 2) use [] to disable absorbing boundary condition(so that all model boundaries will be stress-free) 3) the most common case in seismology is [RIGHT, BOTTOM]",
                          rootModel);
       
          declareParameter(std::string("absorbing_boundary:enable_Clayton_Enquist"),
                          "enable the Clayton-Enquist approach",
                          parameterTypes::boolean,
                          "", //acceptableValues doesn't mean anything here since the type is a boolean
                          "true",
                          "the simplest linear approach by Clayton & Engquist (1977)",
                          rootModel);
          
          declareParameter(std::string("absorbing_boundary:Kosloff_Kosloff:relative_spans"),
                          "relative spans of the sponge layers",
                          parameterTypes::numberArray,
                          "", //acceptableValues doesn't mean anything here since the type is a boolean
                          "[0.05,0.05]",
                          "must be presented in the same order as absorbing_boundaries "
                          "2) to use Kosloff-Kosloff, the mesh should be a little larger "
                          "than the required computational domain; for example, given "
                          "a required domain spans from 0 to 100 km in depth, one can "
                          "generate a mesh from 0 to 110 km and set the relative span "
                          "to 0.05, so the thickness of the sponge layer at the mesh "
                          "bottom will be determined as 110 * 0.05 = 5.5 km, leaving "
                          "an unaffected depth range from 0 to 104.5 km for normal "
                          "wave propagation and analysis. "
                          "3) allowed range: .01 ~ 0.25 ",
                          rootModel);

          declareParameter(std::string("absorbing_boundary:gamma_expr_solid"),
                          "expression of γ-factor in solid domain",
                          parameterTypes::mathEquation,
                          "",
                          "1.1 / T0 * (VS / VP)^2 * exp(-0.04 * SPAN / (VP * T0))",
                          "1) γ-factor represents the absorbing strength at a point "
                          "2) allowed arguments include (case sensitive): "
                          "   - VP, VS: P- and S- wave velocities at the point "
                          "   - RHO   : density at the point "
                          "   - SPAN  : span of the sponge layer "
                          "   - T0    : mesh period "
                          "    * VP, VS and RHO are the 1D values in the Exodus mesh "
                          "3) this expression will be further multiplied by a pattern "
                          "function that equals to 1 on the outermost edge of the "
                          "sponge layer (i.e., on the mesh boundary) and gradually "
                          "decreases to 0 on the the innermost edge; such a decreasing "
                          "pattern is automatically handled by the solver "
                          "4) the default is an empirical expression from "
                          "Haindl et al., 2020",
                          rootModel);

          declareParameter(std::string("absorbing_boundary:enable_Clayton_Enquist"),
                          "expression of γ-factor in fluid domain",
                          parameterTypes::mathEquation,
                          "",
                          "0.88 / T0 * exp(-0.04 * SPAN / (VP * T0))",
                          "same as gamma_expr_solid but without VS dependency ",
                          rootModel);

          // Attenuation
          declareParameter(std::string("attenuation"),
                          "attenuation mode",
                          parameterTypes::string,
                          "NONE / FULL / CG4",
                          "CG4",
                          "# note: 1) NONE: turn off attenuation "
                          "2) FULL: compute attenuation on all GLL points "
                          "3) CG4:  compute attenuation on 4 GLL points per element; "
                          "CG4 is mostly as accurate as FULL but more efficient "
                          "than FULL, see van Driel & Nissen-​Meyer, 2014; "
                          "CG4 requires NPOL_SEM set to 4 via CMake.",
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
         * inparam.yaml file. Separate the acceptable values using the "/" character, e.g., "RIGHT/TOP/BOTTOM". 
         * If there are multiple acceptable input types, then fill out
         * the acceptableValues for each type in the same order that the types are listed.
         * @param defaultValue The default value or values of the parameter. If there are multiple
         * acceptable input types, then fill out the default value for each in the same order
         * that the input types were specified.
         * @param note Any notes about this input parameter. There is a corresponding section
         * in each inparam.yaml file.
         * @param node The root yaml node for which to declare the parameter. Corresponds to one of the 
         * inparam.yaml files: advanced, model, nr, output, or source.
         */
        

        // These versions are overloaded in order to pass keywords as a string separated by :.
        // should later overload for parameters which only have 1 type/acceptableValue/defaultValue.
        // for now these will all be vectors 
        void declareParameter(std::string keyword, 
                              std::string description, 
                              std::vector<parameterTypes> type, 
                              std::vector<std::string> acceptableValues, 
                              std::string defaultValue, 
                              std::string note, 
                              Yaml::Node& node);
        void declareParameter(std::vector<std::string> keyword, 
                              std::string description, 
                              std::vector<parameterTypes> type, 
                              std::vector<std::string> acceptableValues, 
                              std::string defaultValue, 
                              std::string note, 
                              Yaml::Node& node);

        // Overload for when parameters only have 1 acceptable type 
        // (And thus only 1 acceptable type of input value)
        void declareParameter(std::string keyword, 
                              std::string description, 
                              parameterTypes type, 
                              std::string acceptableValues, 
                              std::string defaultValue, 
                              std::string note, 
                              Yaml::Node& node);


        /**
         * Write all of the possible input parameters to the parameters.yaml file in the outout directory.
         * parameters.yaml is used to generate the markdown files for AxiSEM3D's parameter documentation.
         */
        void writeAllParameters();

        std::string typeEnumToString(parameterTypes type);

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


# parameters for verbosity
verbose:
    # what: verbose to stdout or a file
    # type: string
    # only: STDOUT / filename
    channel: STDOUT

    # what: verbosity level
    # type: string
    # only: NONE / ESSENTIAL / DETAILED
    level: DETAILED


*/





#endif /* inparam_hpp */
