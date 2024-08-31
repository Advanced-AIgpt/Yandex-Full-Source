#include "protobuf.h"

using namespace NYdb;

namespace NYdbHelpers {

namespace {

EPrimitiveType GetType(const google::protobuf::FieldDescriptor& field) {
    Y_ENSURE(!field.is_repeated());

    switch (field.cpp_type()) {
        case TProtobufType::CPPTYPE_INT32:
            return EPrimitiveType::Int32;
        case TProtobufType::CPPTYPE_INT64:
            return EPrimitiveType::Int64;
        case TProtobufType::CPPTYPE_UINT32:
            return EPrimitiveType::Uint32;
        case TProtobufType::CPPTYPE_UINT64:
            return EPrimitiveType::Uint64;
        case TProtobufType::CPPTYPE_DOUBLE:
            return EPrimitiveType::Double;
        case TProtobufType::CPPTYPE_FLOAT:
            return EPrimitiveType::Float;
        case TProtobufType::CPPTYPE_BOOL:
            return EPrimitiveType::Bool;
        case TProtobufType::CPPTYPE_STRING:
            return EPrimitiveType::String;

        // Following types are not supported.
        case TProtobufType::CPPTYPE_ENUM:
        case TProtobufType::CPPTYPE_MESSAGE:
            Y_ENSURE(false);
    }

    Y_UNREACHABLE();
}

} // namespace

TStringBuf ProtobufTypeToString(TProtobufType type) {
    switch (type) {
        case TProtobufType::CPPTYPE_BOOL:
            return "Bool";
        case TProtobufType::CPPTYPE_DOUBLE:
            return "Double";
        case TProtobufType::CPPTYPE_ENUM:
            return "Enum";
        case TProtobufType::CPPTYPE_FLOAT:
            return "Float";
        case TProtobufType::CPPTYPE_INT32:
            return "Int32";
        case TProtobufType::CPPTYPE_INT64:
            return "Int64";
        case TProtobufType::CPPTYPE_MESSAGE:
            return "Message";
        case TProtobufType::CPPTYPE_STRING:
            return "String";
        case TProtobufType::CPPTYPE_UINT32:
            return "Uint32";
        case TProtobufType::CPPTYPE_UINT64:
            return "Uint64";
    }
}

TStringBuf YdbTypeToString(TYdbType type) {
    switch (type) {
        case TYdbType::Bool:
            return "Bool";
        case TYdbType::Int8:
            return "Int8";
        case TYdbType::Uint8:
            return "Uint8";
        case TYdbType::Int16:
            return "Int16";
        case TYdbType::Uint16:
            return "Uint16";
        case TYdbType::Int32:
            return "Int32";
        case TYdbType::Uint32:
            return "Uint32";
        case TYdbType::Int64:
            return "Int64";
        case TYdbType::Uint64:
            return "Uint64";
        case TYdbType::Float:
            return "Float";
        case TYdbType::Double:
            return "Double";
        case TYdbType::Date:
            return "Date";
        case TYdbType::Datetime:
            return "Datetime";
        case TYdbType::Timestamp:
            return "Timestamp";
        case TYdbType::Interval:
            return "Interval";
        case TYdbType::TzDate:
            return "TzDate";
        case TYdbType::TzDatetime:
            return "TzDatetime";
        case TYdbType::TzTimestamp:
            return "TzTimestamp";
        case TYdbType::String:
            return "String";
        case TYdbType::Utf8:
            return "Utf8";
        case TYdbType::Yson:
            return "Yson";
        case TYdbType::Json:
            return "Json";
        case TYdbType::Uuid:
            return "Uuid";
        case TYdbType::JsonDocument:
            return "JsonDocument";
        case TYdbType::DyNumber:
            return "DyNumber";
    }
}

TTableSchema::TTableSchema(const google::protobuf::Descriptor& descriptor) {
    for (int i = 0; i < descriptor.field_count(); ++i) {
        const auto* field = descriptor.field(i);
        Y_ASSERT(field);
        Columns.emplace_back(field->name(), field->cpp_type(), GetType(*field), i, field->is_optional());
    }
}

} // namespace NYdbHelpers

template <>
void Out<NYdbHelpers::TProtobufType>(IOutputStream& out, NYdbHelpers::TProtobufType type) {
    out << NYdbHelpers::ProtobufTypeToString(type);
}
