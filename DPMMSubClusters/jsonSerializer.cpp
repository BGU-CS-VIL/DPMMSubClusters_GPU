#include "jsonSerializer.h"

bool CJsonSerializer::serialize(IJsonSerializable* pObj, std::string& output)
{
    if (pObj == NULL)
        return false;

    Json::Value serializeRoot;
    pObj->serialize(serializeRoot);

    Json::StreamWriterBuilder builder;
    output = Json::writeString(builder, serializeRoot);

    return true;
}