#include "MsgToTbl.h"

#include <LuaIntf/LuaIntf.h>
#include <LuaIntf/LuaState.h>  // for LuaException
#include <google/protobuf/message.h>  // for Message

using namespace LuaIntf;
using namespace google::protobuf;

MsgToTbl::MsgToTbl(lua_State& rLuaState, const Message& msg)
    : m_rLuaState(rLuaState),
    m_msg(msg),
    // DEL m_pDesc(msg.GetDescriptor()),
    m_pRefl(msg.GetReflection())
{
    // DEL
    //if (!m_pDesc)
    //    throw LuaException("Message " + msg.GetTypeName() + " has no descriptor.");
    if (!m_pRefl)
        throw LuaException("Message " + msg.GetTypeName() + " has no reflection.");
}

LuaRef MsgToTbl::ToTbl() const
{
    std::vector<const FieldDescriptor*> vFields;
    m_pRefl->ListFields(m_msg, &vFields);
    LuaRef tbl = LuaRef::createTable(&m_rLuaState);
    for (const FieldDescriptor* pField : vFields)
    {
        assert(pField);
        tbl[pField->name()] = GetField(*pField);
    }

    return tbl;
}

LuaRef MsgToTbl::GetField(const FieldDescriptor& field) const
{
    if (field.is_repeated())
    {
        // returns (TableRef, "") or (nil, error_string)
        return GetRepeatedField(field);
    }

    using Fd = FieldDescriptor;
    lua_State* L = &m_rLuaState;
    Fd::CppType eCppType = field.cpp_type();
    switch (eCppType)
    {
    // Scalar field always has a default value.
    case Fd::CPPTYPE_INT32:
        return LuaRefValue(L, m_pRefl->GetInt32(m_msg, &field));
    case Fd::CPPTYPE_INT64:
        return LuaRefValue(L, m_pRefl->GetInt64(m_msg, &field));
    case Fd::CPPTYPE_UINT32:
        return LuaRefValue(L, m_pRefl->GetUInt32(m_msg, &field));
    case Fd::CPPTYPE_UINT64:
        return LuaRefValue(L, m_pRefl->GetUInt64(m_msg, &field));
    case Fd::CPPTYPE_DOUBLE:
        return LuaRefValue(L, m_pRefl->GetDouble(m_msg, &field));
    case Fd::CPPTYPE_FLOAT:
        return LuaRefValue(L, m_pRefl->GetFloat(m_msg, &field));
    case Fd::CPPTYPE_BOOL:
        return LuaRefValue(L, m_pRefl->GetBool(m_msg, &field));
    case Fd::CPPTYPE_ENUM:
        return LuaRefValue(L, m_pRefl->GetEnumValue(m_msg, &field));
    case Fd::CPPTYPE_STRING:
        return LuaRefValue(L, m_pRefl->GetString(m_msg, &field));
    case Fd::CPPTYPE_MESSAGE:
        // For message field, the default value is null.
        if (m_pRefl->HasField(m_msg, &field))
        {
            const Message& subMsg = m_pRefl->GetMessage(m_msg, &field);
            return MsgToTbl(*L, subMsg).ToTbl();
        }
        return LuaRef(L, nullptr);
    default:
        break;
    }
    // Unknown field type CPPTYPE_UNKNOWN of Message.Field
    throw LuaException(string("Unknown field type ") +
        field.CppTypeName(eCppType) + " of " + field.full_name());
}

// Return a lua table. Map is supported.
LuaRef MsgToTbl::GetRepeatedField(const FieldDescriptor& field) const
{
    assert(field.is_repeated());

    LuaRef tbl = LuaRef::createTable(&m_rLuaState);
    int nFldSize = m_pRefl->FieldSize(m_msg, &field);
    for (int index = 0; index < nFldSize; ++index)
    {
        // XXX support map
        tbl[index + 1] = GetRepeatedFieldElement(field, index);
    }
    return tbl;
}

// index starts from 0.
LuaRef MsgToTbl::GetRepeatedFieldElement(
    const FieldDescriptor& field, int index) const
{
    assert(field.is_repeated());
    assert(index >= 0);
    assert(index < m_pRefl->FieldSize(m_msg, &field));

    lua_State* L = &m_rLuaState;
    using Fd = FieldDescriptor;
    Fd::CppType eCppType = field.cpp_type();
    switch (eCppType)
    {
    case Fd::CPPTYPE_INT32:
        return LuaRefValue(L, m_pRefl->GetRepeatedInt32(m_msg, &field, index));
    case Fd::CPPTYPE_INT64:
        return LuaRefValue(L, m_pRefl->GetRepeatedInt64(m_msg, &field, index));
    case Fd::CPPTYPE_UINT32:
        return LuaRefValue(L, m_pRefl->GetRepeatedUInt32(m_msg, &field, index));
    case Fd::CPPTYPE_UINT64:
        return LuaRefValue(L, m_pRefl->GetRepeatedUInt64(m_msg, &field, index));
    case Fd::CPPTYPE_DOUBLE:
        return LuaRefValue(L, m_pRefl->GetRepeatedDouble(m_msg, &field, index));
    case Fd::CPPTYPE_FLOAT:
        return LuaRefValue(L, m_pRefl->GetRepeatedFloat(m_msg, &field, index));
    case Fd::CPPTYPE_BOOL:
        return LuaRefValue(L, m_pRefl->GetRepeatedBool(m_msg, &field, index));
    case Fd::CPPTYPE_ENUM:
        return LuaRefValue(L, m_pRefl->GetRepeatedEnumValue(m_msg, &field, index));
    case Fd::CPPTYPE_STRING:
        return LuaRefValue(L, m_pRefl->GetRepeatedString(m_msg, &field, index));
    case Fd::CPPTYPE_MESSAGE:
        // Support map entry element.
        return LuaRefValue(L, m_pRefl->GetRepeatedMessage(m_msg, &field, index));
    default:
        break;
    }
    // Unknown repeated field type CPPTYPE_UNKNOWN of Message.Field
    throw LuaException(string("Unknown repeated field type ") +
        field.CppTypeName(eCppType) + " of " + field.full_name());
}

