#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "../DataEntity/dataEntity.h"
#include <unordered_set>

class Parser
{

    std::vector<std::string> tokens;
    std::string supportedKeywords[7] = {"ping", "echo", "set", "get", "config", "dir", "dbfilename"};

public:
    Parser();
    Parser(std::string);
    std::vector<std::string> getTokens();
    void setTokens(std::string);
    std::string recursivelyParseTokens(int index, std::string message, std::string foundKeyword, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection);
};

#endif