#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "../DataEntity/dataEntity.h"
#include <unordered_set>

class Parser
{

    std::vector<std::string> tokens;
    std::string supportedKeywords[8] = {"ping", "echo", "set", "get", "config", "dir", "dbfilename", "keys"};
    std::vector<unsigned char> fileData;

public:
    Parser();
    Parser(std::string);
    Parser(std::string, std::string, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection);
    std::vector<std::string> getTokens();
    void setTokens(std::string);
    std::string recursivelyParseTokens(int index, std::string message, std::string foundKeyword, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection);

    void readFileAndGatherInformation(std::string filePath, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection);

    std::string parseGetCommand(std::string entityKey, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection);
    std::string parseSetCommand(int startIndex, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection);
};

#endif