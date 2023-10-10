#include "dataEntity.h"

DataEntity::DataEntity(std::string key)
{
    dataKey = key;
}

bool DataEntity::operator==(const DataEntity &t) const
{
    return dataKey == t.dataKey;
}

DataEntity::DataEntity(std::string key, std::string value, bool fromFile, std::string exp)
{
    dataKey = key;
    dataValue = value;
    isFileEntity = fromFile;
    if (exp.length())
    {
        std::chrono::system_clock::duration now = std::chrono::system_clock::now().time_since_epoch();
        expiry = new std::chrono::system_clock::duration(std::chrono::milliseconds(stoll(exp)) + now);
    }
}

bool DataEntity::operator!=(const DataEntity &other) const
{
    return !operator==(other);
}

bool DataEntity::isEntityFileRelated() const
{
    return isFileEntity;
}

bool DataEntity::isEntityExpired() const
{
    if (expiry == NULL)
        return false;

    std::chrono::system_clock::duration currentTime = std::chrono::system_clock::now().time_since_epoch();
    return currentTime > (*expiry);
}

std::string DataEntity::getValue() const { return dataValue; }
std::string DataEntity::getKey() const { return dataKey; }
