#ifndef DATA_ENTITY_H
#define DATA_ENTITY_H

#include <string>
#include <chrono>

class DataEntity
{
    std::string dataKey;
    std::string dataValue;
    std::chrono::system_clock::duration *expiry = NULL;

public:
    DataEntity(std::string key);
    DataEntity(std::string key, std::string value, std::string exp = "");
    bool operator==(const DataEntity &t) const;
    bool operator!=(const DataEntity &other) const;
    bool isEntityExpired() const;
    friend class DataEntityHashFunction;
    std::string getValue() const;
};

class DataEntityHashFunction
{
public:
    size_t operator()(const DataEntity &t) const
    {
        const std::hash<std::string> hasher;
        return hasher(t.dataKey);
    }
};

#endif
