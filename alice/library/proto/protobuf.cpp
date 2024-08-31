#include "protobuf.h"

#include <alice/protos/extensions/extensions.pb.h>

#include <google/protobuf/descriptor.pb.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/stream/str.h>


namespace NAlice {

using namespace google::protobuf;

namespace NImpl {
bool DoesProtoHaveForbiddenType(const google::protobuf::Descriptor* descriptor,
                                const THashSet<google::protobuf::FieldDescriptor::CppType>& forbiddenTypes,
                                THashSet<const google::protobuf::Descriptor*>& visited) {
    if (visited.contains(descriptor)) {
        return false;
    }
    visited.insert(descriptor);
    for (int i = 0; i < descriptor->field_count(); ++i) {
        const auto& field = descriptor->field(i);
        const auto& type = field->cpp_type();
        if (forbiddenTypes.contains(type)) {
            return true;
        }
        if (type == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE &&
            DoesProtoHaveForbiddenType(field->message_type(), forbiddenTypes, visited)) {
            return true;
        }
    }
    return false;
}
} // namespace NImpl

TString ProtoToJsonString(const google::protobuf::Message& message,
                          const google::protobuf::util::JsonOptions& options) {
    TString buffer;
    const auto status = google::protobuf::util::MessageToJsonString(message, &buffer, options);
    Y_ENSURE(status.ok(), "Failed to convert provided Proto to Json: " << status.ToString());
    return buffer;
}

// TProtoListBuilder ---------------------------------------------------------
google::protobuf::ListValue TProtoListBuilder::Build() const {
    return List;
}

TProtoListBuilder& TProtoListBuilder::Add(const TString& value) {
    google::protobuf::Value v;
    v.set_string_value(value);
    return Add(v);
}

TProtoListBuilder& TProtoListBuilder::AddInt(int value) {
    return AddDouble(static_cast<double>(value));
}

TProtoListBuilder& TProtoListBuilder::AddUInt(unsigned int value) {
    return AddDouble(static_cast<double>(value));
}

TProtoListBuilder& TProtoListBuilder::AddInt64(i64 value) {
    return AddDouble(static_cast<double>(value));
}

TProtoListBuilder& TProtoListBuilder::AddUInt64(ui64 value) {
    return AddDouble(static_cast<double>(value));
}

TProtoListBuilder& TProtoListBuilder::AddFloat(float value) {
    return AddDouble(static_cast<double>(value));
}

TProtoListBuilder& TProtoListBuilder::AddDouble(double value) {
    google::protobuf::Value v;
    v.set_number_value(value);
    return Add(v);
}

TProtoListBuilder& TProtoListBuilder::AddBool(bool value) {
    google::protobuf::Value v;
    v.set_bool_value(value);
    return Add(v);
}

TProtoListBuilder& TProtoListBuilder::Add(google::protobuf::ListValue value) {
    google::protobuf::Value v;
    *v.mutable_list_value() = std::move(value);
    return Add(v);
}

TProtoListBuilder& TProtoListBuilder::Add(google::protobuf::Struct value) {
    google::protobuf::Value v;
    *v.mutable_struct_value() = std::move(value);
    return Add(v);
}

TProtoListBuilder& TProtoListBuilder::Add(const google::protobuf::Value& value) {
    *List.add_values() = value;
    return *this;
}

TProtoListBuilder& TProtoListBuilder::AddNull() {
    google::protobuf::Value v;
    v.set_null_value(google::protobuf::NullValue::NULL_VALUE);
    return Add(v);
}

bool TProtoListBuilder::IsEmpty() const {
    return List.values_size() == 0;
}

// TProtoStructBuilder ---------------------------------------------------------
TProtoStructBuilder::TProtoStructBuilder(google::protobuf::Struct initialStruct)
    : Struct(std::move(initialStruct)) {
}

google::protobuf::Struct TProtoStructBuilder::Build() const {
    return Struct;
}

TProtoStructBuilder& TProtoStructBuilder::Set(const TString& key, const TString& value) {
    google::protobuf::Value v;
    v.set_string_value(value);
    return Set(key, v);
}

TProtoStructBuilder& TProtoStructBuilder::SetInt(const TString& key, int value) {
    return SetDouble(key, static_cast<double>(value));
}

TProtoStructBuilder& TProtoStructBuilder::SetUInt(const TString& key, unsigned int value) {
    return SetDouble(key, static_cast<double>(value));
}

TProtoStructBuilder& TProtoStructBuilder::SetInt64(const TString& key, i64 value) {
    return SetDouble(key, static_cast<double>(value));
}

TProtoStructBuilder& TProtoStructBuilder::SetUInt64(const TString& key, ui64 value) {
    return SetDouble(key, static_cast<double>(value));
}

TProtoStructBuilder& TProtoStructBuilder::SetFloat(const TString& key, float value) {
    return SetDouble(key, static_cast<double>(value));
}

TProtoStructBuilder& TProtoStructBuilder::SetDouble(const TString& key, double value) {
    google::protobuf::Value v;
    v.set_number_value(value);
    return Set(key, v);
}

TProtoStructBuilder& TProtoStructBuilder::SetBool(const TString& key, bool value) {
    google::protobuf::Value v;
    v.set_bool_value(value);
    return Set(key, v);
}

TProtoStructBuilder& TProtoStructBuilder::Set(const TString& key, google::protobuf::ListValue value) {
    google::protobuf::Value v;
    *v.mutable_list_value() = std::move(value);
    return Set(key, v);
}

TProtoStructBuilder& TProtoStructBuilder::Set(const TString& key, google::protobuf::Struct value) {
    google::protobuf::Value v;
    *v.mutable_struct_value() = std::move(value);
    return Set(key, v);
}

TProtoStructBuilder& TProtoStructBuilder::Set(const TString& key, const google::protobuf::Value& value) {
    (*Struct.mutable_fields())[key] = value;
    return *this;
}

TProtoStructBuilder& TProtoStructBuilder::SetNull(const TString& key) {
    google::protobuf::Value v;
    v.set_null_value(google::protobuf::NullValue::NULL_VALUE);
    return Set(key, v);
}

TProtoStructBuilder& TProtoStructBuilder::Drop(const TString& key) {
    (*Struct.mutable_fields()).erase(key);
    return *this;
}

ListValue RepeatedMessageToList(const google::protobuf::Message& msg, const FieldDescriptor* field,
                                const Reflection* reflection, const TBuilderOptions& options) {
    TProtoListBuilder builder;
    for (int i = 0, e = reflection->FieldSize(msg, field); i < e; ++i) {
        switch (field->cpp_type()) {
            case FieldDescriptor::CPPTYPE_BOOL:
                builder.AddBool(reflection->GetRepeatedBool(msg, field, i));
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                builder.AddDouble(reflection->GetRepeatedDouble(msg, field, i));
                break;
            case FieldDescriptor::CPPTYPE_ENUM: {
                const auto number = reflection->GetRepeatedEnumValue(msg, field, i);
                if (options.PreferNumberInEnums) {
                    builder.AddInt(number);
                } else if (const auto* value = field->enum_type()->FindValueByNumber(number)) {
                    builder.Add(value->name());
                }
                break;
            }
            case FieldDescriptor::CPPTYPE_FLOAT:
                builder.AddDouble(static_cast<double>(reflection->GetRepeatedFloat(msg, field, i)));
                break;
            case FieldDescriptor::CPPTYPE_INT32:
                builder.AddInt(reflection->GetRepeatedInt32(msg, field, i));
                break;
            case FieldDescriptor::CPPTYPE_INT64:
                builder.AddInt64(reflection->GetRepeatedInt64(msg, field, i));
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE: {
                const auto& innerMsg = reflection->GetRepeatedMessage(msg, field, i);
                if (const auto* value = dynamic_cast<const google::protobuf::Value*>(&innerMsg)) {
                    builder.Add(*value);
                } else if (const auto* value = dynamic_cast<const google::protobuf::StringValue*>(&innerMsg)) {
                    builder.Add(value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::BytesValue*>(&innerMsg)) {
                    builder.Add(value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::Int32Value*>(&innerMsg)) {
                    builder.AddInt(value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::UInt32Value*>(&innerMsg)) {
                    builder.AddUInt(value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::Int64Value*>(&innerMsg)) {
                    builder.AddInt64(value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::UInt64Value*>(&innerMsg)) {
                    builder.AddUInt64(value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::FloatValue*>(&innerMsg)) {
                    builder.AddFloat(value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::DoubleValue*>(&innerMsg)) {
                    builder.AddDouble(value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::BoolValue*>(&innerMsg)) {
                    builder.AddBool(value->value());
                } else {
                    builder.Add(MessageToStruct(innerMsg));
                }
                break;
            }
            case FieldDescriptor::CPPTYPE_STRING:
                builder.Add(reflection->GetRepeatedString(msg, field, i));
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                builder.AddUInt(reflection->GetRepeatedUInt32(msg, field, i));
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                builder.AddUInt64(reflection->GetRepeatedUInt64(msg, field, i));
                break;
        }
    }
    return builder.Build();
}

Struct StringKeyMapMessageToStruct(const google::protobuf::Message& msg, const FieldDescriptor* field,
                                   const Reflection* reflection, const TBuilderOptions& options) {
    Y_ENSURE(field->message_type()->map_key()->cpp_type() == FieldDescriptor::CPPTYPE_STRING);
    Y_ENSURE(field->is_map());

    TProtoStructBuilder builder;

    for (int i = 0, e = reflection->FieldSize(msg, field); i < e; ++i) {
        const auto& fieldMessage = reflection->GetRepeatedMessage(msg, field, i);
        const auto* fieldMessageReflection = fieldMessage.GetReflection();
        const auto* fieldMessageDescriptor = fieldMessage.GetDescriptor();

        const auto key = fieldMessageReflection->GetString(fieldMessage, fieldMessageDescriptor->map_key());

        const auto* mapValueDescriptor = fieldMessage.GetDescriptor()->map_value();
        const auto cppType = mapValueDescriptor->cpp_type();

        switch (cppType) {
            case FieldDescriptor::CPPTYPE_BOOL:
                builder.SetBool(key, fieldMessageReflection->GetBool(fieldMessage, mapValueDescriptor));
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                builder.SetDouble(key, fieldMessageReflection->GetDouble(fieldMessage, mapValueDescriptor));
                break;
            case FieldDescriptor::CPPTYPE_ENUM: {
                const auto number = fieldMessageReflection->GetEnumValue(fieldMessage, mapValueDescriptor);
                if (options.PreferNumberInEnums) {
                    builder.SetInt(key, number);
                } else if (const auto* value = mapValueDescriptor->enum_type()->FindValueByNumber(number)) {
                    builder.Set(key, value->name());
                }
                break;
            }
            case FieldDescriptor::CPPTYPE_FLOAT:
                builder.SetDouble(key, static_cast<double>(fieldMessageReflection->GetFloat(fieldMessage, mapValueDescriptor)));
                break;
            case FieldDescriptor::CPPTYPE_INT32:
                builder.SetInt(key, fieldMessageReflection->GetInt32(fieldMessage, mapValueDescriptor));
                break;
            case FieldDescriptor::CPPTYPE_INT64:
                builder.SetInt64(key, fieldMessageReflection->GetInt64(fieldMessage, mapValueDescriptor));
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE: {
                const auto& innerMsg = fieldMessageReflection->GetMessage(fieldMessage, mapValueDescriptor);
                if (const auto* value = dynamic_cast<const google::protobuf::Value*>(&innerMsg)) {
                    builder.Set(key, *value);
                } else if (const auto* value = dynamic_cast<const google::protobuf::StringValue*>(&innerMsg)) {
                    builder.Set(key, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::BytesValue*>(&innerMsg)) {
                    builder.Set(key, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::Int32Value*>(&innerMsg)) {
                    builder.SetInt(key, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::UInt32Value*>(&innerMsg)) {
                    builder.SetUInt(key, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::Int64Value*>(&innerMsg)) {
                    builder.SetInt64(key, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::UInt64Value*>(&innerMsg)) {
                    builder.SetUInt64(key, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::FloatValue*>(&innerMsg)) {
                    builder.SetFloat(key, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::DoubleValue*>(&innerMsg)) {
                    builder.SetDouble(key, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::BoolValue*>(&innerMsg)) {
                    builder.SetBool(key, value->value());
                } else {
                    builder.Set(key, MessageToStruct(innerMsg));
                }
                break;
            }
            case FieldDescriptor::CPPTYPE_STRING:
                builder.Set(key, fieldMessageReflection->GetString(fieldMessage, mapValueDescriptor));
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                builder.SetUInt(key, fieldMessageReflection->GetUInt32(fieldMessage, mapValueDescriptor));
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                builder.SetUInt64(key, fieldMessageReflection->GetUInt64(fieldMessage, mapValueDescriptor));
                break;
        }
    }

    return builder.Build();
}

TProtoStructBuilder& MessageToStruct(TProtoStructBuilder& builder, const google::protobuf::Message& msg,
                                     const TBuilderOptions& options) {
    const auto* descr = msg.GetDescriptor();
    Y_ENSURE(descr, "Cannot get a proto descriptor!");
    const auto* reflection = msg.GetReflection();
    Y_ENSURE(reflection, "Cannot get a proto reflection!");
    for (int fieldIdx = 0; fieldIdx < descr->field_count(); ++fieldIdx) {
        const FieldDescriptor* field = descr->field(fieldIdx);

        const TString jsonName = field->json_name();
        if (field->is_map() && field->message_type()->map_key()->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
            if (reflection->FieldSize(msg, field) != 0) {
                builder.Set(jsonName, StringKeyMapMessageToStruct(msg, field, reflection, options));
            }
            continue;
        }
        if (field->is_repeated()) {
            if (reflection->FieldSize(msg, field) != 0) {
                builder.Set(jsonName, RepeatedMessageToList(msg, field, reflection, options));
            }
            continue;
        }
        const auto isFieldSet = reflection->HasField(msg, field);
        const auto cppType = field->cpp_type();
        const bool shouldProcessIfUnset = options.ProcessDefaultFields &&
                                          !field->options().HasExtension(SkipDefaultSerialization) &&
                                          !field->containing_oneof();
        if (!shouldProcessIfUnset && !isFieldSet) {
            continue;
        }

        switch (cppType) {
            case FieldDescriptor::CPPTYPE_BOOL:
                builder.SetBool(jsonName, reflection->GetBool(msg, field));
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                builder.SetDouble(jsonName, reflection->GetDouble(msg, field));
                break;
            case FieldDescriptor::CPPTYPE_ENUM: {
                const auto number = reflection->GetEnumValue(msg, field);
                if (options.PreferNumberInEnums) {
                    builder.SetInt(jsonName, number);
                } else if (const auto* value = field->enum_type()->FindValueByNumber(number)) {
                    builder.Set(jsonName, value->name());
                }
                break;
            }
            case FieldDescriptor::CPPTYPE_FLOAT:
                builder.SetDouble(jsonName, static_cast<double>(reflection->GetFloat(msg, field)));
                break;
            case FieldDescriptor::CPPTYPE_INT32:
                builder.SetInt(jsonName, reflection->GetInt32(msg, field));
                break;
            case FieldDescriptor::CPPTYPE_INT64:
                builder.SetInt64(jsonName, reflection->GetInt64(msg, field));
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE: {
                const auto& innerMsg = reflection->GetMessage(msg, field);
                // TODO: handle StringValue and other wrappers
                if (options.ProcessDefaultFields && !isFieldSet) {
                    google::protobuf::Value nullValue{};
                    nullValue.set_null_value({});
                    builder.Set(jsonName, nullValue);
                    break;
                }
                if (const auto* value = dynamic_cast<const google::protobuf::Value*>(&innerMsg)) {
                    builder.Set(jsonName, *value);
                } else if (const auto* value = dynamic_cast<const google::protobuf::StringValue*>(&innerMsg)) {
                    builder.Set(jsonName, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::BytesValue*>(&innerMsg)) {
                    builder.Set(jsonName, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::Int32Value*>(&innerMsg)) {
                    builder.SetInt(jsonName, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::UInt32Value*>(&innerMsg)) {
                    builder.SetUInt(jsonName, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::Int64Value*>(&innerMsg)) {
                    builder.SetInt64(jsonName, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::UInt64Value*>(&innerMsg)) {
                    builder.SetUInt64(jsonName, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::FloatValue*>(&innerMsg)) {
                    builder.SetFloat(jsonName, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::DoubleValue*>(&innerMsg)) {
                    builder.SetDouble(jsonName, value->value());
                } else if (const auto* value = dynamic_cast<const google::protobuf::BoolValue*>(&innerMsg)) {
                    builder.SetBool(jsonName, value->value());
                } else {
                    builder.Set(jsonName, MessageToStruct(innerMsg));
                }
                break;
            }
            case FieldDescriptor::CPPTYPE_STRING:
                builder.Set(jsonName, reflection->GetString(msg, field));
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                builder.SetUInt(jsonName, reflection->GetUInt32(msg, field));
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                builder.SetUInt64(jsonName, reflection->GetUInt64(msg, field));
                break;
        }
    }
    return builder;
}

Struct MessageToStruct(const Message& msg, const TBuilderOptions& options) {
    if (const auto* value = dynamic_cast<const Struct*>(&msg)) {
        return *value;
    }
    TProtoStructBuilder builder;
    return MessageToStruct(builder, msg, options).Build();
}

TProtoStructBuilder MessageToStructBuilder(const google::protobuf::Message& message, const TBuilderOptions& options) {
    TProtoStructBuilder builder;
    MessageToStruct(builder, message, options);
    return builder;
}

TString ProtoToBase64String(const google::protobuf::Message& proto) {
    TString s;
    TStringOutput output(s);
    if (!proto.SerializeToArcadiaStream(&output)) {
        ythrow yexception() << "Failed to serialize protobuf";
    }
    return Base64Encode(s);
}

void ProtoFromBase64String(TStringBuf s, google::protobuf::Message& proto) {
    const auto decoded = Base64Decode(s);
    TStringInput input(decoded);
    if (!proto.ParseFromArcadiaStream(&input)) {
        ythrow yexception() << "Failed to parse protobuf";
    }
}

namespace {

void ConvertInnerStructToMessage(const google::protobuf::Value& value, google::protobuf::Message& innerMsg) {
    const auto& innerMsgTypeName = innerMsg.GetTypeName();
    if (innerMsgTypeName == "google.protobuf.Value") {
        innerMsg.CopyFrom(value);
    } else if (auto* stringValue = dynamic_cast<google::protobuf::StringValue*>(&innerMsg)) {
        stringValue->set_value(value.string_value());
    } else if (auto* bytesValue = dynamic_cast<google::protobuf::BytesValue*>(&innerMsg)) {
        bytesValue->set_value(value.string_value());
    } else if (auto* int32Value = dynamic_cast<google::protobuf::Int32Value*>(&innerMsg)) {
        int32Value->set_value(value.number_value());
    } else if (auto* uint32Value = dynamic_cast<google::protobuf::UInt32Value*>(&innerMsg)) {
        uint32Value->set_value(value.number_value());
    } else if (auto* int64Value = dynamic_cast<google::protobuf::Int64Value*>(&innerMsg)) {
        int64Value->set_value(value.number_value());
    } else if (auto* uint64Value = dynamic_cast<google::protobuf::UInt64Value*>(&innerMsg)) {
        uint64Value->set_value(value.number_value());
    } else if (auto* floatValue = dynamic_cast<google::protobuf::FloatValue*>(&innerMsg)) {
        floatValue->set_value(value.number_value());
    } else if (auto* doubleValue = dynamic_cast<google::protobuf::DoubleValue*>(&innerMsg)) {
        doubleValue->set_value(value.number_value());
    } else if (auto* boolValue = dynamic_cast<google::protobuf::BoolValue*>(&innerMsg)) {
        boolValue->set_value(value.bool_value());
    } else if (innerMsgTypeName == "google.protobuf.Struct") {
        innerMsg.CopyFrom(value.struct_value());
    } else if (value.has_struct_value()) {
        StructToMessage(value.struct_value(), innerMsg);
    }
}

void SetRepeatedFieldFromStruct(const google::protobuf::Value& original, google::protobuf::Message& msg,
                                const FieldDescriptor* field, const Reflection* reflection) {
    const auto& list = original.list_value();
    for (int i = 0; i < list.values_size(); ++i) {
        const auto& value = list.values(i);
        switch (field->cpp_type()) {
            case FieldDescriptor::CPPTYPE_BOOL:
                reflection->AddBool(&msg, field, value.bool_value());
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                reflection->AddDouble(&msg, field, value.number_value());
                break;
            case FieldDescriptor::CPPTYPE_ENUM: {
                if (value.kind_case() == google::protobuf::Value::kNumberValue) {
                    reflection->AddEnumValue(&msg, field, static_cast<int>(value.number_value()));
                } else {
                    const auto* enumValue = field->enum_type()->FindValueByName(value.string_value());
                    if (enumValue) {
                        reflection->AddEnum(&msg, field, enumValue);
                    }
                }
                break;
            }
            case FieldDescriptor::CPPTYPE_FLOAT:
                reflection->AddFloat(&msg, field, static_cast<float>(value.number_value()));
                break;
            case FieldDescriptor::CPPTYPE_INT32:
                reflection->AddInt32(&msg, field, static_cast<int32>(value.number_value()));
                break;
            case FieldDescriptor::CPPTYPE_INT64:
                reflection->AddInt64(&msg, field, static_cast<int64>(value.number_value()));
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE: {
                auto* innerMsg = reflection->AddMessage(&msg, field);
                if (innerMsg) {
                    ConvertInnerStructToMessage(value, *innerMsg);
                }
                break;
            }
            case FieldDescriptor::CPPTYPE_STRING:
                reflection->AddString(&msg, field, value.string_value());
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                reflection->AddUInt32(&msg, field, static_cast<uint32>(value.number_value()));
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                reflection->AddUInt64(&msg, field, static_cast<uint64>(value.number_value()));
                break;
        }
    }
}

void SetStringKeyMapFromStruct(const google::protobuf::Value& original, google::protobuf::Message& msg,
                               const FieldDescriptor* field, const Reflection* reflection) {
    Y_ENSURE(field->message_type()->map_key()->cpp_type() == FieldDescriptor::CPPTYPE_STRING);
    Y_ENSURE(field->is_map());

    const auto& structValue = original.struct_value();
    const auto* keyFieldDescriptor = field->message_type()->map_key();
    const auto* valueFieldDescriptor = field->message_type()->map_value();
    const auto cppType = valueFieldDescriptor->cpp_type();
    for (const auto& [k, v] : structValue.fields()) {
        auto* sourceMapEntry = reflection->AddMessage(&msg, field);
        auto* sourceMapEntryReflection = sourceMapEntry->GetReflection();
        sourceMapEntryReflection->SetString(sourceMapEntry, keyFieldDescriptor, k);
        switch (cppType) {
            case FieldDescriptor::CPPTYPE_BOOL:
                sourceMapEntryReflection->SetBool(sourceMapEntry, valueFieldDescriptor, v.bool_value());
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                sourceMapEntryReflection->SetDouble(sourceMapEntry, valueFieldDescriptor, v.number_value());
                break;
            case FieldDescriptor::CPPTYPE_ENUM: {
                if (v.kind_case() == google::protobuf::Value::kNumberValue) {
                    sourceMapEntryReflection->SetEnumValue(sourceMapEntry, valueFieldDescriptor, static_cast<int>(v.number_value()));
                } else {
                    const auto* enumValue = valueFieldDescriptor->enum_type()->FindValueByName(v.string_value());
                    if (enumValue) {
                        sourceMapEntryReflection->SetEnum(sourceMapEntry, valueFieldDescriptor, enumValue);
                    }
                }
                break;
            }
            case FieldDescriptor::CPPTYPE_FLOAT:
                sourceMapEntryReflection->SetFloat(sourceMapEntry, valueFieldDescriptor, static_cast<float>(v.number_value()));
                break;
            case FieldDescriptor::CPPTYPE_INT32:
                sourceMapEntryReflection->SetInt32(sourceMapEntry, valueFieldDescriptor, static_cast<int32>(v.number_value()));
                break;
            case FieldDescriptor::CPPTYPE_INT64:
                sourceMapEntryReflection->SetInt64(sourceMapEntry, valueFieldDescriptor, static_cast<int64>(v.number_value()));
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE: {
                auto* innerMsg = sourceMapEntryReflection->MutableMessage(sourceMapEntry, valueFieldDescriptor);
                if (innerMsg) {
                    ConvertInnerStructToMessage(v, *innerMsg);
                }
                break;
            }
            case FieldDescriptor::CPPTYPE_STRING:
                sourceMapEntryReflection->SetString(sourceMapEntry, valueFieldDescriptor, v.string_value());
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                sourceMapEntryReflection->SetUInt32(sourceMapEntry, valueFieldDescriptor, static_cast<uint32>(v.number_value()));
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                sourceMapEntryReflection->SetUInt64(sourceMapEntry, valueFieldDescriptor, static_cast<uint64>(v.number_value()));
                break;
        }
    }
}

} // namespace

void StructToMessage(const google::protobuf::Struct& original, google::protobuf::Message& msg) {
    const auto* descr = msg.GetDescriptor();
    Y_ENSURE(descr, "Cannot get a proto descriptor!");
    const auto* reflection = msg.GetReflection();
    Y_ENSURE(reflection, "Cannot get a proto reflection!");

    msg.Clear();

    for (int fieldIdx = 0; fieldIdx < descr->field_count(); ++fieldIdx) {
        const FieldDescriptor* field = descr->field(fieldIdx);
        const TString& jsonName = field->json_name();
        const auto* value = MapFindPtr(original.fields(), jsonName);
        if (!value) {
            continue;
        }

        if (field->is_map() && field->message_type()->map_key()->cpp_type() == FieldDescriptor::CPPTYPE_STRING && value->has_struct_value()) {
            SetStringKeyMapFromStruct(*value, msg, field, reflection);
            continue;
        }

        if (field->is_repeated() && value->has_list_value()) {
            SetRepeatedFieldFromStruct(*value, msg, field, reflection);
            continue;
        }

        const auto cppType = field->cpp_type();

        switch (cppType) {
            case FieldDescriptor::CPPTYPE_BOOL:
                reflection->SetBool(&msg, field, value->bool_value());
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                reflection->SetDouble(&msg, field, value->number_value());
                break;
            case FieldDescriptor::CPPTYPE_ENUM: {
                if (value->kind_case() == google::protobuf::Value::kNumberValue) {
                    reflection->SetEnumValue(&msg, field, static_cast<int>(value->number_value()));
                } else {
                    const auto* enumValue = field->enum_type()->FindValueByName(value->string_value());
                    if (enumValue) {
                        reflection->SetEnum(&msg, field, enumValue);
                    }
                }
                break;
            }
            case FieldDescriptor::CPPTYPE_FLOAT:
                reflection->SetFloat(&msg, field, static_cast<float>(value->number_value()));
                break;
            case FieldDescriptor::CPPTYPE_INT32:
                reflection->SetInt32(&msg, field, static_cast<int32>(value->number_value()));
                break;
            case FieldDescriptor::CPPTYPE_INT64:
                reflection->SetInt64(&msg, field, static_cast<int64>(value->number_value()));
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE: {
                auto* innerMsg = reflection->MutableMessage(&msg, field);
                if (innerMsg) {
                    ConvertInnerStructToMessage(*value, *innerMsg);
                }
                break;
            }
            case FieldDescriptor::CPPTYPE_STRING:
                reflection->SetString(&msg, field, value->string_value());
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                reflection->SetUInt32(&msg, field, static_cast<uint32>(value->number_value()));
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                reflection->SetUInt64(&msg, field, static_cast<uint64>(value->number_value()));
                break;
        }
    }
}

} // namespace NAlice
