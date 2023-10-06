#include "parser.h"

std::string parseSetEntity(std::vector<std::string> tokens, int startIndex, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection);
std::string parseGetEntity(std::vector<std::string> tokens, int startIndex, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection);

Parser::Parser() {}

Parser::Parser(std::string statement) { setTokens(statement); }
std::vector<std::string> Parser::getTokens() { return tokens; }

void Parser::setTokens(std::string statement)
{
    std::string delimiter = "\r\n";
    auto delimiterLen = delimiter.length();

    size_t endIndex = 0;
    size_t startIndex = 0;
    while ((endIndex = statement.find(delimiter, startIndex)) != std::string::npos)
    {

        std::string token = statement.substr(startIndex, endIndex - startIndex);
        tokens.push_back(token);
        startIndex = endIndex + delimiterLen;
    }

    tokens.push_back(statement.substr(startIndex));
}

std::string Parser::recursivelyParseTokens(int index, std::string message, std::string foundKeyword, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection)
{
    if (index >= tokens.size() - 1)
        return message;
    if (foundKeyword != "")
    {
        if (foundKeyword == supportedKeywords[2])
            return parseSetEntity(tokens, index, entityCollection);
        else if (foundKeyword == supportedKeywords[3])
            return parseGetEntity(tokens, index, entityCollection);
        else
            return recursivelyParseTokens(index + 1, message + tokens[index] + "\r\n", foundKeyword, entityCollection);
    }
    else
    {
        if (tokens[index].length() > 0 && ((tokens[index][0] == '$') || tokens[index][0] == '*'))
            return recursivelyParseTokens(index + 1, message, foundKeyword, entityCollection);
        else if (tokens[index] == supportedKeywords[0])
            return recursivelyParseTokens(index + 1, message + "$4\r\nPONG\r\n", supportedKeywords[0], entityCollection);
        else if (tokens[index] == supportedKeywords[1] || tokens[index] == supportedKeywords[2] || tokens[index] == supportedKeywords[3])
            return recursivelyParseTokens(index + 1, message,
                                          tokens[index] == supportedKeywords[1] ? supportedKeywords[1] : tokens[index] == supportedKeywords[2] ? supportedKeywords[2]
                                                                                                                                               : supportedKeywords[3],
                                          entityCollection);
        else
            return recursivelyParseTokens(index + 1, message, "", entityCollection);
    }
}

std::string parseSetEntity(std::vector<std::string> tokens, int startIndex, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection)
{
    if (startIndex + 7 < tokens.size())
    {
        std::string entityKey = tokens[startIndex + 1];
        std::string entityValue = tokens[startIndex + 3];
        std::string entityExpiryType = tokens[startIndex + 5];
        std::string entityExpiry = tokens[startIndex + 7];
        const DataEntity entity = DataEntity(entityKey, entityValue, entityExpiry);
        entityCollection.insert(entity);
        return "$2\r\nOK\r\n";
    }
    else if (startIndex + 3 < tokens.size())
    {
        std::string entityKey = tokens[startIndex + 1];
        std::string entityValue = tokens[startIndex + 3];
        const DataEntity entity = DataEntity(entityKey, entityValue);
        entityCollection.insert(entity);
        return "$2\r\nOK\r\n";
    }
    return "";
}

std::string parseGetEntity(std::vector<std::string> tokens, int startIndex, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection)
{
    if (startIndex + 1 < tokens.size())
    {
        std::string entityKey = tokens[startIndex + 1];
        std::unordered_set<DataEntity, DataEntityHashFunction>::iterator foundEntity = entityCollection.find(DataEntity(entityKey));
        if (foundEntity != entityCollection.end() && !foundEntity->isEntityExpired())
        {
            std::string msg = foundEntity->getValue();
            return "$" + std::to_string(msg.length()) + "\r\n" + msg + "\r\n";
        }
        return "$-1\r\n";
    }
    return "";
}