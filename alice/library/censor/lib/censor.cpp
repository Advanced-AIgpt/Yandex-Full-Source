#include "censor.h"

#include <google/protobuf/descriptor.pb.h>

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>

namespace NAlice {

using google::protobuf::Reflection;

namespace {

const int DEFAULT_PRIVATE_NUM = 0;
const int DEFAULT_NUM = 0;
const TString DEFAULT_PRIVATE_STR = "***";
const TString DEFAULT_STR = "";
const TString DEFAULT_PRIVATE_BYTES = "***";
const TString DEFAULT_BYTES = "";
const bool DEFAULT_BOOL = false;

template <typename TAdder, typename TSetter, typename T>
void CensorField(const FieldDescriptor* fieldDescriptor, const Reflection* reflection, Message& message,
                 const T& defaultValue, const TAdder add, const TSetter set) {
    if (fieldDescriptor->is_repeated()) {
        if (reflection->FieldSize(message, fieldDescriptor) > 0) {
            reflection->ClearField(&message, fieldDescriptor);
            add(reflection, message, fieldDescriptor, defaultValue);
        }
    } else if (reflection->HasField(message, fieldDescriptor)) {
        set(reflection, message, fieldDescriptor, defaultValue);
    }
}

bool ShouldCensor(const TCensor::TFlags candidate, const TCensor::TFlags mode) {
    return candidate & mode;
}

} // namespace

void TCensor::ProcessMessage(const TFlags mode, Message& message) {
    const auto* descriptor = message.GetDescriptor();
    const auto* reflection = message.GetReflection();

    if (CensorMessages.contains(descriptor)) {
        message.Clear();
    }
    if (!RequiredFields[descriptor].empty()) {
        InitializeRequiredFields(message);
    }

    const auto fieldKey = std::make_pair(mode, descriptor);

    if (!TraverseFields.contains(fieldKey)) {
        ScanDescriptor(mode, *descriptor);
    }

    for (const auto& [fieldDescriptor, mayContainPrivateFields] : TraverseFields[fieldKey]) {
        switch (fieldDescriptor->type()) {
            case FieldDescriptor::TYPE_DOUBLE:
                CensorField(
                    fieldDescriptor, reflection, message, DEFAULT_PRIVATE_NUM,
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const double defaultValue) { reflection->AddDouble(&message, fieldDescriptor, defaultValue); },
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const double defaultValue) { reflection->SetDouble(&message, fieldDescriptor, defaultValue); });
                break;
            case FieldDescriptor::TYPE_FLOAT:
                CensorField(
                    fieldDescriptor, reflection, message, DEFAULT_PRIVATE_NUM,
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const float defaultValue) { reflection->AddFloat(&message, fieldDescriptor, defaultValue); },
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const float defaultValue) { reflection->SetFloat(&message, fieldDescriptor, defaultValue); });
                break;
            case FieldDescriptor::TYPE_SFIXED64:
                [[fallthrough]];
            case FieldDescriptor::TYPE_SINT64:
                [[fallthrough]];
            case FieldDescriptor::TYPE_INT64:
                CensorField(
                    fieldDescriptor, reflection, message, DEFAULT_PRIVATE_NUM,
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const i64 defaultValue) { reflection->AddInt64(&message, fieldDescriptor, defaultValue); },
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const i64 defaultValue) { reflection->SetInt64(&message, fieldDescriptor, defaultValue); });
                break;
            case FieldDescriptor::TYPE_FIXED64:
                [[fallthrough]];
            case FieldDescriptor::TYPE_UINT64:
                CensorField(
                    fieldDescriptor, reflection, message, DEFAULT_PRIVATE_NUM,
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const ui64 defaultValue) { reflection->AddUInt64(&message, fieldDescriptor, defaultValue); },
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const ui64 defaultValue) { reflection->SetUInt64(&message, fieldDescriptor, defaultValue); });
                break;
            case FieldDescriptor::TYPE_SFIXED32:
                [[fallthrough]];
            case FieldDescriptor::TYPE_SINT32:
                [[fallthrough]];
            case FieldDescriptor::TYPE_INT32:
                CensorField(
                    fieldDescriptor, reflection, message, DEFAULT_PRIVATE_NUM,
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const i32 defaultValue) { reflection->AddInt32(&message, fieldDescriptor, defaultValue); },
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const i32 defaultValue) { reflection->SetInt32(&message, fieldDescriptor, defaultValue); });
                break;
            case FieldDescriptor::TYPE_BOOL:
                CensorField(
                    fieldDescriptor, reflection, message, DEFAULT_PRIVATE_NUM,
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const bool defaultValue) { reflection->AddInt32(&message, fieldDescriptor, defaultValue); },
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const bool defaultValue) { reflection->SetInt32(&message, fieldDescriptor, defaultValue); });
                break;
            case FieldDescriptor::TYPE_STRING:
                CensorField(
                    fieldDescriptor, reflection, message, DEFAULT_PRIVATE_STR,
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const TString& defaultValue) {
                        reflection->AddString(&message, fieldDescriptor, defaultValue);
                    },
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const TString& defaultValue) {
                        reflection->SetString(&message, fieldDescriptor, defaultValue);
                    });
                break;
            case FieldDescriptor::TYPE_GROUP:
                break;
            case FieldDescriptor::TYPE_MESSAGE: {
                if (mayContainPrivateFields) {
                    if (fieldDescriptor->is_repeated()) {
                        for (int j = 0; j < reflection->FieldSize(message, fieldDescriptor); j++) {
                            ProcessMessage(mode, *reflection->MutableRepeatedMessage(&message, fieldDescriptor, j));
                        }
                    } else if (reflection->HasField(message, fieldDescriptor)) {
                        ProcessMessage(mode, *reflection->MutableMessage(&message, fieldDescriptor));
                    }
                } else {
                    CensorField(
                        fieldDescriptor, reflection, message, nullptr,
                        [this](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                               Message* /* defaultValue */) {
                            Message* nestedMessage = google::protobuf::MessageFactory::generated_factory()
                                                         ->GetPrototype(fieldDescriptor->message_type())
                                                         ->New();
                            InitializeRequiredFields(*nestedMessage);
                            reflection->AddAllocatedMessage(&message, fieldDescriptor, nestedMessage);
                        },
                        [this](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                               Message* /* defaultValue */) {
                            Message* nestedMessage = google::protobuf::MessageFactory::generated_factory()
                                                         ->GetPrototype(fieldDescriptor->message_type())
                                                         ->New();
                            InitializeRequiredFields(*nestedMessage);
                            reflection->SetAllocatedMessage(&message, nestedMessage, fieldDescriptor);
                        });
                }
                break;
            }
            case FieldDescriptor::TYPE_BYTES:
                CensorField(
                    fieldDescriptor, reflection, message, DEFAULT_PRIVATE_BYTES,
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const TString& defaultValue) {
                        reflection->AddString(&message, fieldDescriptor, defaultValue);
                    },
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const TString& defaultValue) {
                        reflection->SetString(&message, fieldDescriptor, defaultValue);
                    });
                break;
            case FieldDescriptor::TYPE_FIXED32:
                [[fallthrough]];
            case FieldDescriptor::TYPE_UINT32:
                CensorField(
                    fieldDescriptor, reflection, message, DEFAULT_PRIVATE_NUM,
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const ui32 defaultValue) { reflection->AddUInt32(&message, fieldDescriptor, defaultValue); },
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const ui32 defaultValue) { reflection->SetUInt32(&message, fieldDescriptor, defaultValue); });
                break;
            case FieldDescriptor::TYPE_ENUM:
                CensorField(
                    fieldDescriptor, reflection, message, fieldDescriptor->default_value_enum(),
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const google::protobuf::EnumValueDescriptor* defaultValue) {
                        reflection->AddEnum(&message, fieldDescriptor, defaultValue);
                    },
                    [](const Reflection* reflection, Message& message, const FieldDescriptor* fieldDescriptor,
                       const google::protobuf::EnumValueDescriptor* defaultValue) {
                        reflection->SetEnum(&message, fieldDescriptor, defaultValue);
                    });
                break;
        }
    }
}
bool TCensor::ScanDescriptor(const TFlags mode, const Descriptor& descriptor) {
    const auto messageFlags = GenerateFlags(descriptor.options().GetRepeatedExtension(MessageAccess));
    const auto keyField = std::make_pair(mode, &descriptor);

    bool traverseFurther = false;
    if (ShouldCensor(messageFlags, mode)) {
        traverseFurther = true;
        CensorMessages.insert(&descriptor);
    }

    if (const auto* traverseFields = TraverseFields.FindPtr(keyField)) {
        traverseFurther |= !traverseFields->empty();
        return traverseFurther;
    }

    auto& requiredFieldsValue = RequiredFields[&descriptor];
    auto& traverseFieldsValue = TraverseFields[keyField];
    for (int i = 0; i < descriptor.field_count(); i++) {
        const auto* fieldDescriptor = descriptor.field(i);
        const auto fieldFlags = GenerateFlags(fieldDescriptor->options().GetRepeatedExtension(FieldAccess));
        if (fieldDescriptor->is_required()) {
            requiredFieldsValue.push_back(fieldDescriptor);
        }
        const auto canTraverse = [](const FieldDescriptor& fieldDescriptor) {
            return fieldDescriptor.type() == FieldDescriptor::TYPE_MESSAGE &&
                   !TStringBuf{fieldDescriptor.full_name()}.StartsWith("google.protobuf");
        };
        if (ShouldCensor(fieldFlags, mode)) {
            traverseFieldsValue.push_back({fieldDescriptor, false});
            traverseFurther = true;
            if (canTraverse(*fieldDescriptor)) {
                ScanDescriptor(mode, *fieldDescriptor->message_type());
            }
        } else {
            if (canTraverse(*fieldDescriptor)) {
                if (ScanDescriptor(mode, *fieldDescriptor->message_type())) {
                    traverseFieldsValue.push_back({fieldDescriptor, true});
                    traverseFurther = true;
                }
            }
        }
    }
    return traverseFurther;
}
void TCensor::InitializeRequiredFields(Message& message) {
    const auto* descriptor = message.GetDescriptor();
    const auto* reflection = message.GetReflection();

    for (const auto* fieldDescriptor : RequiredFields[descriptor]) {
        switch (fieldDescriptor->type()) {
            case FieldDescriptor::TYPE_DOUBLE:
                reflection->SetDouble(&message, fieldDescriptor, DEFAULT_NUM);
                break;
            case FieldDescriptor::TYPE_FLOAT:
                reflection->SetFloat(&message, fieldDescriptor, DEFAULT_NUM);
                break;
            case FieldDescriptor::TYPE_SFIXED64:
                [[fallthrough]];
            case FieldDescriptor::TYPE_SINT64:
                [[fallthrough]];
            case FieldDescriptor::TYPE_INT64:
                reflection->SetInt64(&message, fieldDescriptor, DEFAULT_NUM);
                break;
            case FieldDescriptor::TYPE_FIXED64:
                [[fallthrough]];
            case FieldDescriptor::TYPE_UINT64:
                reflection->SetUInt64(&message, fieldDescriptor, DEFAULT_NUM);
                break;
            case FieldDescriptor::TYPE_SFIXED32:
                [[fallthrough]];
            case FieldDescriptor::TYPE_SINT32:
                [[fallthrough]];
            case FieldDescriptor::TYPE_INT32:
                reflection->SetInt32(&message, fieldDescriptor, DEFAULT_NUM);
                break;
            case FieldDescriptor::TYPE_BOOL:
                reflection->SetBool(&message, fieldDescriptor, DEFAULT_BOOL);
                break;
            case FieldDescriptor::TYPE_STRING:
                reflection->SetString(&message, fieldDescriptor, DEFAULT_STR);
                break;
            case FieldDescriptor::TYPE_GROUP:
                break;
            case FieldDescriptor::TYPE_MESSAGE: {
                auto* nestedMessage = reflection->MutableMessage(&message, fieldDescriptor);
                if (!RequiredFields[nestedMessage->GetDescriptor()].empty()) {
                    InitializeRequiredFields(*nestedMessage);
                }
            } break;
            case FieldDescriptor::TYPE_BYTES:
                reflection->SetString(&message, fieldDescriptor, DEFAULT_BYTES);
                break;
            case FieldDescriptor::TYPE_FIXED32:
                [[fallthrough]];
            case FieldDescriptor::TYPE_UINT32:
                reflection->SetUInt32(&message, fieldDescriptor, DEFAULT_NUM);
                break;
            case FieldDescriptor::TYPE_ENUM:
                reflection->SetEnum(&message, fieldDescriptor, fieldDescriptor->default_value_enum());
                break;
        }
    }
}

} // namespace NAlice
