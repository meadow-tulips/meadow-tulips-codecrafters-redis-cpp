#include "dataEntity.h"

DataEntity::DataEntity(std::string key, std::string value, std::string exp)
{
    dataKey = key;
    dataValue = value;

    if (exp.length())
    {
        std::chrono::system_clock::duration now = std::chrono::system_clock::now().time_since_epoch();
        expiry = new std::chrono::system_clock::duration(std::chrono::milliseconds(stoi(exp)) + now);
    }
}

DataEntity::DataEntity(std::string key)
{
    dataKey = key;
}

bool DataEntity::operator==(const DataEntity &t) const
{
    return dataKey == t.dataKey;
}

bool DataEntity::operator!=(const DataEntity &other) const
{
    return !operator==(other);
}

bool DataEntity::isEntityExpired() const
{
    if (expiry == NULL)
        return false;

    std::chrono::system_clock::duration currentTime = std::chrono::system_clock::now().time_since_epoch();
    return currentTime > (*expiry);
}

std::string DataEntity::getValue() const { return dataValue; }
