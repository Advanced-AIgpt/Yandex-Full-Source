#pragma once

#include <util/generic/hash_set.h>

#include <google/protobuf/struct.pb.h>
#include <google/protobuf/wrappers.pb.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

#include <util/generic/hash.h>
#include <util/system/types.h>

#include <utility>

namespace NAlice {

namespace NImpl {
bool DoesProtoHaveForbiddenType(const google::protobuf::Descriptor* descriptor,
                                const THashSet<google::protobuf::FieldDescriptor::CppType>& forbiddenTypes,
                                THashSet<const google::protobuf::Descriptor*>& visited);
} // namespace NImpl

template <typename TProtoMessage>
bool DoesProtoHaveForbiddenType(const THashSet<google::protobuf::FieldDescriptor::CppType>& forbiddenTypes) {
    THashSet<const google::protobuf::Descriptor*> visited;
    return NImpl::DoesProtoHaveForbiddenType(TProtoMessage::descriptor(), forbiddenTypes, visited);
}

template <typename TProtoMessage>
bool DoesProtoHave64BitNumber() {
    return DoesProtoHaveForbiddenType<TProtoMessage>(
        {google::protobuf::FieldDescriptor::CPPTYPE_INT64, google::protobuf::FieldDescriptor::CPPTYPE_UINT64});
}

TString ProtoToJsonString(const google::protobuf::Message& message,
                          const google::protobuf::util::JsonOptions& options = {});

class TProtoListBuilder final {
public:
    [[nodiscard]] google::protobuf::ListValue Build() const;

    TProtoListBuilder& Add(const TString& value);
    TProtoListBuilder& AddInt(int value);
    TProtoListBuilder& AddUInt(unsigned int value);
    TProtoListBuilder& AddInt64(i64 value);
    TProtoListBuilder& AddUInt64(ui64 value);
    TProtoListBuilder& AddFloat(float value);
    TProtoListBuilder& AddDouble(double value);
    TProtoListBuilder& AddBool(bool value);
    TProtoListBuilder& Add(google::protobuf::ListValue value);
    TProtoListBuilder& Add(google::protobuf::Struct value);
    TProtoListBuilder& AddNull();
    TProtoListBuilder& Add(const google::protobuf::Value& value);
    bool IsEmpty() const;

private:
    google::protobuf::ListValue List;
};

class TProtoStructBuilder final {
public:
    TProtoStructBuilder() = default;
    explicit TProtoStructBuilder(google::protobuf::Struct initialStruct);

    [[nodiscard]] google::protobuf::Struct Build() const;

    TProtoStructBuilder& Set(const TString& key, const TString& value);
    TProtoStructBuilder& SetInt(const TString& key, int value);
    TProtoStructBuilder& SetUInt(const TString& key, unsigned int value);
    TProtoStructBuilder& SetInt64(const TString& key, i64 value);
    TProtoStructBuilder& SetUInt64(const TString& key, ui64 value);
    TProtoStructBuilder& SetFloat(const TString& key, float value);
    TProtoStructBuilder& SetDouble(const TString& key, double value);
    TProtoStructBuilder& SetBool(const TString& key, bool value);
    TProtoStructBuilder& Set(const TString& key, google::protobuf::ListValue value);
    TProtoStructBuilder& Set(const TString& key, google::protobuf::Struct value);
    TProtoStructBuilder& SetNull(const TString& key);
    TProtoStructBuilder& Drop(const TString& key);
    TProtoStructBuilder& Set(const TString& key, const google::protobuf::Value& value);

private:
    google::protobuf::Struct Struct;
};

struct TBuilderOptions final {
    bool PreferNumberInEnums = false;
    bool ProcessDefaultFields = false;
};

TProtoStructBuilder& MessageToStruct(TProtoStructBuilder& builder, const google::protobuf::Message& msg,
                                     const TBuilderOptions& options = {});
google::protobuf::Struct MessageToStruct(const google::protobuf::Message& msg, const TBuilderOptions& options = {});
TProtoStructBuilder MessageToStructBuilder(const google::protobuf::Message& message,
                                           const TBuilderOptions& options = {});

TString ProtoToBase64String(const google::protobuf::Message& proto);
void ProtoFromBase64String(TStringBuf s, google::protobuf::Message& proto);

void StructToMessage(const google::protobuf::Struct& original, google::protobuf::Message& msg);

template <typename TProtoMessage>
TProtoMessage StructToMessage(const google::protobuf::Struct& original) {
    TProtoMessage msg;
    StructToMessage(original, msg);
    return msg;
}

template <class TKey, class TValue>
THashMap<TKey, TValue> ParseProtoMap(const google::protobuf::Map<TKey, TValue>& actions) {
    return THashMap<TKey, TValue>(actions.begin(), actions.end());
}

} // namespace NAlice
