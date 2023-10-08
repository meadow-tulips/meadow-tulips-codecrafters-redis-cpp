#include "parser.h"
#include <fstream>
#include <cctype>

std::string parseSetEntity(std::vector<std::string> tokens, int startIndex, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection);
std::string parseGetEntity(std::string entityKey, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection, std::string supportedKeywords[]);
std::string readFileAndGetKeys(std::string fileName);

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
            return parseSetEntity(tokens, index, entityCollection);
        else if (foundKeyword == supportedKeywords[3] || foundKeyword == supportedKeywords[4] || foundKeyword == supportedKeywords[7])
        {
            if ((!tokens[index].starts_with("$") && tokens[index] != supportedKeywords[3]))
                return parseGetEntity(tokens[index], entityCollection, supportedKeywords);
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

std::string parseGetEntity(std::string key, std::unordered_set<DataEntity, DataEntityHashFunction> &entityCollection, std::string supportedKeywords[])
{
    std::string entityKey = key == "*" ? "dir" : key;
    std::unordered_set<DataEntity, DataEntityHashFunction>::iterator foundEntity = entityCollection.find(DataEntity(entityKey));
    if (foundEntity != entityCollection.end() && !foundEntity->isEntityExpired())
    {
        std::string msg = foundEntity->getValue();
        if (key == "*")
        {
            std::unordered_set<DataEntity, DataEntityHashFunction>::iterator rdbFile = entityCollection.find(DataEntity("dbfilename"));
            if (rdbFile != entityCollection.end())
            {
                std::string fullPath = foundEntity->getValue() + "/" + rdbFile->getValue();
                return readFileAndGetKeys(fullPath);
            }
            else
            {
                return "";
            }
        }
        else if (entityKey != supportedKeywords[5] && entityKey != supportedKeywords[6])
            return "$" + std::to_string(msg.length()) + "\r\n" + msg + "\r\n";
        else
            return "*2\r\n$" + std::to_string(entityKey.length()) + "\r\n" + entityKey + "\r\n" + "$" + std::to_string(msg.length()) + "\r\n" + msg + "\r\n";
    }
    else
        return "$-1\r\n";
}

std::string readFileAndGetKeys(std::string filePath)
{
    std::fstream fs;
    fs.open(filePath, fs.binary | fs.in);

    if (fs.is_open())
    {
        fs.seekg(0, std::ios::end);
        auto fileSize = fs.tellg();
        fs.seekg(0, std::ios::beg);

        // read the data:
        std::vector<unsigned char> fileData(fileSize);
        fs.read((char *)&fileData[0], fileSize);

        char delimiter = '@';
        int delimiterIndex = -1;
        std::string value;

        for (int i = 0; i < fileData.size(); i++)
        {
            if (delimiterIndex < 0)
            {
                if (fileData[i] == '@')
                    delimiterIndex = i;
                else
                    continue;
            }
            else
            {
                if (isalpha(fileData[i]))
                    value += fileData[i];
                else
                {
                    if (value == "")
                        continue;
                    break;
                }
            }
        }

        if (value != "")
            return "*1\r\n$" + std::to_string(value.length()) + "\r\n" + value + "\r\n";
        return "";
    }
    return "";
}