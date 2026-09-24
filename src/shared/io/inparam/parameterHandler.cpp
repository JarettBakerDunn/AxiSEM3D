// -----------------------------------------------------------------------------
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2019 - 2026 by the AxiSEM3D authors
//
// This file is part of the AxiSEM3D library. See the LICENSE file for details.
//
// -----------------------------------------------------------------------------

#include "parameterHandler.hpp"

void
inparam::ParameterHandler::writeAllParameters() {
    
    Yaml::Node & rootReference = rootModel;
    
    std::ofstream fout("output/parameters.yaml");
    std::string contents;
    Yaml::Serialize(rootReference, contents);

    if (fout.is_open()) {
        fout << contents;
        fout.close();
    }
}

/***
 * This is to handle chained yaml accessors for example: "this:that:other:true"
  * 
 * 
 */
void
inparam::ParameterHandler::declareParameter(std::vector<std::string> keywords, 
                              std::string description, 
                              std::vector<parameterTypes> type, 
                              std::vector<std::string> acceptableValues, 
                              std::string defaultValue, 
                              std::string note, 
                              Yaml::Node& node) {
    /*
        Declare child node, then pop off the first string in the keywords vector,
        call the function again until the vector is empty, then set the value of the node.
        You can see a similar recursive pattern on line 1852 of external/yaml/Yaml.cpp in the 
        ParseMap function or other parsing functions.

        If keywords.size==1, then write the values
    */
    if (keywords.size()==1){

        // Description and Note are always simple strings.
        node[keywords.front()]["Description:"] = description;
        node[keywords.front()]["Note:"] = note;

        // PARAMETER TYPES
        // This is to handle type values that are arrays.
        if (type.size()>1){
            for(int i=0; i < type.size();i++){
                node[keywords.front()]["Type:"].PushBack();
                node[keywords.front()]["Type:"][i] = typeEnumToString(type[i]);
            }
        } else {
            // Write all the information about this parameter into YAML
            node[keywords.front()]["Type:"] = typeEnumToString(type.front());
        }


        // DEFAULT VALUES
        // This is to handle default values which are arrays. boundary default=RIGHT,BOTTOM.
        std::vector<std::string> defaultValues = bstring::split(defaultValue,",");
        if (defaultValues.size()>1){
            for(int i=0; i < defaultValues.size();i++){
                node[keywords.front()]["Default Value:"].PushBack();
                node[keywords.front()]["Default Value:"][i] = defaultValues[i];
            }
        } else {
            // Write all the information about this parameter into YAML
            node[keywords.front()]["Default Value:"] = defaultValue;
        }

        // There will be multiple acceptable inputs if there are multiple
        // acceptable input types.
        for(int i=0; i < acceptableValues.size();i++){
            std::vector<std::string> acceptableValuesVector = bstring::split(acceptableValues[i],"/");
            if (acceptableValuesVector.size()>1){
                for(int j=0; j < acceptableValuesVector.size();j++){
                    node[keywords.front()]["Acceptable Values:"].PushBack();
                    node[keywords.front()]["Acceptable Values:"][j] = acceptableValuesVector[j];
                }
            } else {
                // Write all the information about this parameter into YAML
                node[keywords.front()]["Acceptable Values:"] = acceptableValuesVector.front();
            }
        }
        return;
    } else {
        Yaml::Node & childNode = node[keywords.front()];
        keywords.erase(keywords.begin());
        declareParameter(keywords,description,type,acceptableValues,defaultValue,note,childNode);
    }
}

void
inparam::ParameterHandler::declareParameter(std::string keyword, 
                              std::string description, 
                              std::vector<parameterTypes> type,
                              std::vector<std::string> acceptableValues, 
                              std::string defaultValue, 
                              std::string note, 
                              Yaml::Node& node) {
    declareParameter(bstring::split(keyword, ":"),description,type,acceptableValues,defaultValue,note,node);
}

void 
inparam::ParameterHandler::declareParameter(std::string keyword, 
                        std::string description, 
                        parameterTypes type, 
                        std::string acceptableValues, 
                        std::string defaultValue, 
                        std::string note, 
                        Yaml::Node& node){
   declareParameter(keyword,description,std::vector<parameterTypes>{type},std::vector<std::string>{acceptableValues},defaultValue,note,node);
}




std::string 
inparam::ParameterHandler::typeEnumToString(parameterTypes type){
    switch(type){
        case parameterTypes::boolean:
            return "Boolean";
            break;
        case parameterTypes::mathEquation:
            return "Math Equation";
            break;
        case parameterTypes::string:
            return "String";
            break;
        case parameterTypes::number:
            return "Double";
            break;
        case parameterTypes::stringArray:
            return "Array of Strings, e.g. [String1, String2]";
            break;
        case parameterTypes::numberArray:
            return "Array of Doubles, e.g. [1.0, 1.5]";
            break;
        case parameterTypes::integer:
            return "Integer";
            break;
        case parameterTypes::filename:
            return "Filename, e.g. inputmesh.e";
            break;
        case parameterTypes::objectArray:
            return "Array of arbitrary YAML Objects";
            break; // e.g., examples/02_3d_crust_S362ANI_regional/input_with_3d_crust
    }
}
