#include "jsonSerializer.h"

bool CJsonSerializer::serialize(IJsonSerializable* pObj, std::string& output)
{
    if (pObj == NULL)
        return false;

    Json::Value serializeRoot;
    pObj->serialize(serializeRoot);

    Json::StyledWriter writer;
    output = writer.write(serializeRoot);

    return true;
}

bool CJsonSerializer::deserialize(IJsonSerializable* pObj, std::string& input)
{
    if (pObj == NULL)
        return false;

    Json::Value deserializeRoot;
    Json::Reader reader;

    if (!reader.parse(input, deserializeRoot))
        return false;

    pObj->deserialize(deserializeRoot);

    return true;
}
