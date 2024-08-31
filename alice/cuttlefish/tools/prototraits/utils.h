#pragma once
#include <google/protobuf/descriptor.h>
#include <util/string/subst.h>
#include <util/string/cast.h>
#include <map>


namespace NProtoTraits {

using namespace ::google::protobuf;
using namespace ::google::protobuf::compiler;
using TPrinterVariables = std::map<TProtoStringType, TProtoStringType>;

template <typename T>
inline TProtoStringType FullTypeName(const T& desc) {
    return "::" + SubstGlobalCopy(desc.full_name(), ".", "::");
}

template <typename T>
inline TProtoStringType FullTypeName(const T* desc) {
    return desc == nullptr ? "" : FullTypeName(*desc);
}

template <typename T>
inline TProtoStringType FullEnclosingNamespaceName(const T& desc) {
    TStringBuf fullName(desc.full_name());
    TStringBuf namespaceName, typeName;
    if (fullName.TryRSplit('.', namespaceName, typeName))
        return SubstGlobalCopy(ToString(namespaceName), ".", "::");
    return "";
}

TProtoStringType ProtobufHeader(const FileDescriptor* file);

TProtoStringType ProtobufTraitsHeader(const FileDescriptor* file);

TString GetFieldCppTypeName(const ::google::protobuf::FieldDescriptor& desc);

TPrinterVariables GetFieldVariables(const FieldDescriptor& desc);

}  // namespace NProtoTraits
