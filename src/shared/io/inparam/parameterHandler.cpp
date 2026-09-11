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
                              parameterTypes type, 
                              std::string acceptableValues, 
                              std::string defaultValue, 
                              std::string note, 
                              Yaml::Node& node) {
    /*
        Declare child node, then pop off the first string in the keywords vector,
        call the function again until the vector is empty, then set the value of the node.
        You can see a similar recursive pattern on line 1852 of external/yaml/Yaml.cpp in the 
        ParseMap function or other parsing functions.
    */
    if (keywords.size()==1){
        // This is to handle default values which are arrays. For example. absorbing
        // boundary default being RIGHT and BOTTOM.
        std::vector<std::string> defaultValues = bstring::split(defaultValue,",");
        // Need to figure out a way to actually write these to the yaml
        // in a way that makes it easier to generate documentation.
        if (defaultValues.size()>1){
            for(int i=0; i < defaultValues.size();i++){
                node[keywords.front()].PushBack();
                node[keywords.front()][i] = defaultValues[i];
            }
        } else {
            node[keywords.front()] = defaultValue;
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
                              parameterTypes type, 
                              std::string acceptableValues, 
                              std::string defaultValue, 
                              std::string note, 
                              Yaml::Node& node) {
    declareParameter(bstring::split(keyword, ":"),description,type,acceptableValues,defaultValue,note,node);
}
