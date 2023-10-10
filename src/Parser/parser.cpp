#include "parser.h"
#include <iterator>
#include <cctype>

std::string findAllFilesRelatedEntityCollection(std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection);

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
        if (foundKeyword == supportedKeywords[1])
            return recursivelyParseTokens(index + 1, message + tokens[index] + "\r\n", foundKeyword, entityCollection);
        else if (foundKeyword == supportedKeywords[2])
            return parseSetCommand(index, entityCollection);
        else if (foundKeyword == supportedKeywords[3] || foundKeyword == supportedKeywords[4] || foundKeyword == supportedKeywords[7])
        {
            if ((!tokens[index].starts_with("$") && tokens[index] != supportedKeywords[3]))
                return parseGetCommand(tokens[index], entityCollection);
        }
        return recursivelyParseTokens(index + 1, message, foundKeyword, entityCollection);
    }
    else
    {
        if (tokens[index].starts_with("$") || tokens[index].starts_with("*"))
            return recursivelyParseTokens(index + 1, message, foundKeyword, entityCollection);

        std::string retrievedKeyword = tokens[index] == supportedKeywords[0]   ? supportedKeywords[0]
                                       : tokens[index] == supportedKeywords[1] ? supportedKeywords[1]
                                       : tokens[index] == supportedKeywords[2] ? supportedKeywords[2]
                                       : tokens[index] == supportedKeywords[3] ? supportedKeywords[3]
                                       : tokens[index] == supportedKeywords[4] ? supportedKeywords[4]
                                       : tokens[index] == supportedKeywords[7] ? supportedKeywords[7]
                                                                               : foundKeyword;
        std::string newMessage = tokens[index] == supportedKeywords[0] ? message + "$4\r\nPONG\r\n" : message;

        return recursivelyParseTokens(index + 1, newMessage, retrievedKeyword, entityCollection);
    }
}

std::string Parser::parseSetCommand(int startIndex, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection)
{
    if (startIndex + 7 < tokens.size())
    {
        std::string entityKey = tokens[startIndex + 1];
        std::string entityValue = tokens[startIndex + 3];
        std::string entityExpiryType = tokens[startIndex + 5];
        std::string entityExpiry = tokens[startIndex + 7];
        const DataEntity entity = DataEntity(entityKey, entityValue, false, entityExpiry);
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

std::string Parser::parseGetCommand(std::string entityKey, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection)
{
    if (entityKey == "*")
        return findAllFilesRelatedEntityCollection(entityCollection);
    else
    {
        std::unordered_set<DataEntity, DataEntityHashFunction>::iterator foundEntity = entityCollection.find(DataEntity(entityKey));
        if (foundEntity != entityCollection.end() && !foundEntity->isEntityExpired())
        {
            std::string msg = foundEntity->getValue();
            if (entityKey != supportedKeywords[5] && entityKey != supportedKeywords[6])
                return "$" + std::to_string(msg.length()) + "\r\n" + msg + "\r\n";
            else
                return "*2\r\n$" + std::to_string(entityKey.length()) + "\r\n" + entityKey + "\r\n" + "$" + std::to_string(msg.length()) + "\r\n" + msg + "\r\n";
        }
        else
            return "$-1\r\n";
    }
}

std::string findAllFilesRelatedEntityCollection(std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection)
{
    std::string response;
    int count = 0;
    for (std::unordered_set<DataEntity, DataEntityHashFunction>::iterator it = entityCollection.begin(); it != entityCollection.end(); it++)
    {
        if (it->isEntityFileRelated())
        {
            count++;

            std::string key = it->getKey();
            std::string val = it->getValue();
            response += "$" + std::to_string(key.length()) + "\r\n" + key + "\r\n";
        }
    }

    if (count > 0)
    {
        return "*" + std::to_string(count) + "\r\n" + response;
    }
    else
        return "";
}