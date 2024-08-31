#include "utils.h"
#include <util/generic/yexception.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>


namespace NProtoTraits {

TProtoStringType ProtobufHeader(const FileDescriptor* file) {
    return cpp::StripProto(file->name()) + ".pb.h";
}


TProtoStringType ProtobufTraitsHeader(const FileDescriptor* file) {
    return cpp::StripProto(file->name()) + ".traits.pb.h";
}


TString GetFieldCppTypeName(const FieldDescriptor& desc)
{
    if (desc.is_map()) {
        const FieldDescriptor* keyDesc = desc.message_type()->FindFieldByLowercaseName("key");
        const FieldDescriptor* valueDesc = desc.message_type()->FindFieldByLowercaseName("value");
        Y_ENSURE(keyDesc != nullptr && valueDesc != nullptr);
        return "::google::protobuf::Map<" + GetFieldCppTypeName(*keyDesc) + ", " + GetFieldCppTypeName(*valueDesc) + ">";
    }

    switch(desc.cpp_type()) {
        case FieldDescriptor::CPPTYPE_STRING:
            return "TString";
        case FieldDescriptor::CPPTYPE_MESSAGE:
            return FullTypeName(*desc.message_type());
        case FieldDescriptor::CPPTYPE_INT32:
            return "int32_t";
        case FieldDescriptor::CPPTYPE_INT64:
            return "int64_t";
        case FieldDescriptor::CPPTYPE_UINT32:
            return "uint32_t";
        case FieldDescriptor::CPPTYPE_UINT64:
            return "uint64_t";
        case FieldDescriptor::CPPTYPE_DOUBLE:
            return "double";
        case FieldDescriptor::CPPTYPE_FLOAT:
            return "float";
        case FieldDescriptor::CPPTYPE_ENUM:
            return FullTypeName(*desc.enum_type());
        case FieldDescriptor::CPPTYPE_BOOL:
            return "bool";
        default:
            Y_ENSURE(!"Unknown type");
    }
}


TPrinterVariables GetFieldVariables(const FieldDescriptor& desc)
{
    return {
        {"MESSAGE_TYPE", FullTypeName(desc.containing_type())},
        {"FIELD_TYPE", GetFieldCppTypeName(desc)},
        {"FIELD_NAME", desc.name()}
    };
}

}  // namespace NProtoTraits
