#include "field_differ.h"

#include <alice/library/json/json.h>

#include <google/protobuf/util/message_differencer.h>

#include <util/generic/vector.h>
#include <util/string/cast.h>

namespace {
using namespace NAlice;

constexpr TStringBuf DELIM = ".";

using google::protobuf::EnumValueDescriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Message;

int GetSize(const Message& message, const FieldDescriptor& fieldDescriptor) {
    return message.GetReflection()->FieldSize(message, &fieldDescriptor);
}

template <typename T>
T GetValue(const Message& message, const FieldDescriptor& fieldDescriptor);

template <>
i64 GetValue(const Message& message, const FieldDescriptor& fieldDescriptor) {
    return message.GetReflection()->GetInt64(message, &fieldDescriptor);
}

template <>
i32 GetValue(const Message& message, const FieldDescriptor& fieldDescriptor) {
    return message.GetReflection()->GetInt32(message, &fieldDescriptor);
}

template <>
double GetValue(const Message& message, const FieldDescriptor& fieldDescriptor) {
    return message.GetReflection()->GetDouble(message, &fieldDescriptor);
}

template <>
float GetValue(const Message& message, const FieldDescriptor& fieldDescriptor) {
    return message.GetReflection()->GetFloat(message, &fieldDescriptor);
}

template <>
ui64 GetValue(const Message& message, const FieldDescriptor& fieldDescriptor) {
    return message.GetReflection()->GetUInt64(message, &fieldDescriptor);
}

template <>
ui32 GetValue(const Message& message, const FieldDescriptor& fieldDescriptor) {
    return message.GetReflection()->GetUInt32(message, &fieldDescriptor);
}

template <>
const Message& GetValue(const Message& message, const FieldDescriptor& fieldDescriptor) {
    return message.GetReflection()->GetMessage(message, &fieldDescriptor);
}

template <>
TString GetValue(const Message& message, const FieldDescriptor& fieldDescriptor) {
    return message.GetReflection()->GetString(message, &fieldDescriptor);
}

template <>
const EnumValueDescriptor& GetValue(const Message& message, const FieldDescriptor& fieldDescriptor) {
    return *message.GetReflection()->GetEnum(message, &fieldDescriptor);
}

template <>
bool GetValue(const Message& message, const FieldDescriptor& fieldDescriptor) {
    return message.GetReflection()->GetBool(message, &fieldDescriptor);
}

template <typename T>
T GetRepeatedValue(const Message& message, const FieldDescriptor& fieldDescriptor, int i);

template <>
i64 GetRepeatedValue(const Message& message, const FieldDescriptor& fieldDescriptor, int i) {
    return message.GetReflection()->GetRepeatedInt64(message, &fieldDescriptor, i);
}

template <>
i32 GetRepeatedValue(const Message& message, const FieldDescriptor& fieldDescriptor, int i) {
    return message.GetReflection()->GetRepeatedInt32(message, &fieldDescriptor, i);
}

template <>
double GetRepeatedValue(const Message& message, const FieldDescriptor& fieldDescriptor, int i) {
    return message.GetReflection()->GetRepeatedDouble(message, &fieldDescriptor, i);
}

template <>
float GetRepeatedValue(const Message& message, const FieldDescriptor& fieldDescriptor, int i) {
    return message.GetReflection()->GetRepeatedFloat(message, &fieldDescriptor, i);
}

template <>
ui32 GetRepeatedValue(const Message& message, const FieldDescriptor& fieldDescriptor, int i) {
    return message.GetReflection()->GetRepeatedUInt32(message, &fieldDescriptor, i);
}

template <>
ui64 GetRepeatedValue(const Message& message, const FieldDescriptor& fieldDescriptor, int i) {
    return message.GetReflection()->GetRepeatedUInt64(message, &fieldDescriptor, i);
}

template <>
const Message& GetRepeatedValue(const Message& message, const FieldDescriptor& fieldDescriptor, int i) {
    return message.GetReflection()->GetRepeatedMessage(message, &fieldDescriptor, i);
}

template <>
TString GetRepeatedValue(const Message& message, const FieldDescriptor& fieldDescriptor, int i) {
    return message.GetReflection()->GetRepeatedString(message, &fieldDescriptor, i);
}

template <>
const EnumValueDescriptor& GetRepeatedValue(const Message& message, const FieldDescriptor& fieldDescriptor, int i) {
    return *message.GetReflection()->GetRepeatedEnum(message, &fieldDescriptor, i);
}

template <>
bool GetRepeatedValue(const Message& message, const FieldDescriptor& fieldDescriptor, int i) {
    return message.GetReflection()->GetRepeatedBool(message, &fieldDescriptor, i);
}

bool HasValue(const Message& message, const FieldDescriptor& fieldDescriptor) {
    return message.GetReflection()->HasField(message, &fieldDescriptor);
}

template <typename T>
auto ToJsonPrimitive(const T& value) {
    return value;
}

template <>
auto ToJsonPrimitive(const Message& value) {
    return NAlice::JsonFromProto(value);
}

template <>
auto ToJsonPrimitive(const EnumValueDescriptor& value) {
    return value.name();
}

template <typename T>
TString ToString(const T& value) {
    return ::ToString(ToJsonPrimitive<T>(value));
}

template <>
TString ToString(const Message& value) {
    return NAlice::JsonStringFromProto(value);
}

template <>
TString ToString(const EnumValueDescriptor& value) {
    return ToJsonPrimitive(value);
}

bool operator==(const Message& lhs, const Message& rhs) {
    return google::protobuf::util::MessageDifferencer::Equals(lhs, rhs);
}

bool operator==(const EnumValueDescriptor& lhs, const EnumValueDescriptor& rhs) {
    return lhs.number() == rhs.number();
}

template <typename T>
void Check(const FieldDescriptor& fieldDescriptor, const Message& lhs, const Message& rhs, TDifferReport& differReport,
           const TString& path, const EImportantFieldCheck importantFieldCheck) {
    const auto generateDiffFromRepeatedField = [&](const int lenLhs, const int lenRhs) {
        auto& diff = *differReport.AddDiffs();
        diff.SetPath(path);
        diff.SetImportantFieldCheck(importantFieldCheck);
        NJson::TJsonValue jsonLhs = NJson::JSON_ARRAY;
        NJson::TJsonValue jsonRhs = NJson::JSON_ARRAY;
        for (const auto& [from, to, len] : {std::tie(lhs, jsonLhs, lenLhs), std::tie(rhs, jsonRhs, lenRhs)}) {
            for (int i = 0; i < len; ++i) {
                const auto& value = GetRepeatedValue<T>(from, fieldDescriptor, i);
                to.AppendValue(ToJsonPrimitive(value));
            }
        }
        diff.SetFirstValue(NAlice::JsonToString(jsonLhs));
        diff.SetSecondValue(NAlice::JsonToString(jsonRhs));
    };

    const auto generateDiffFromField = [&](const auto& firstValue, const auto& secondValue) {
        auto& diff = *differReport.AddDiffs();
        diff.SetPath(path);
        diff.SetImportantFieldCheck(importantFieldCheck);
        diff.SetFirstValue(ToString(firstValue));
        diff.SetSecondValue(ToString(secondValue));
    };

    const auto generateDiffFromFieldPresence = [&](const auto& firstValue, const auto& secondValue,
                                                   const bool firstHas, const bool secondHas) {
        auto& diff = *differReport.AddDiffs();
        diff.SetPath(path);
        diff.SetImportantFieldCheck(importantFieldCheck);
        if (firstHas) {
            diff.SetFirstValue(ToString(firstValue));
        }
        if (secondHas) {
            diff.SetSecondValue(ToString(secondValue));
        }
    };

    switch (importantFieldCheck) {
        case IFC_NOTHING:
            break;
        case IFC_DIFF:
            if (fieldDescriptor.is_repeated()) {
                const auto lenLhs = GetSize(lhs, fieldDescriptor);
                const auto lenRhs = GetSize(rhs, fieldDescriptor);
                bool equal = lenLhs == lenRhs;
                for (int i = 0; i < lenLhs && equal; ++i) {
                    equal &=
                        GetRepeatedValue<T>(lhs, fieldDescriptor, i) == GetRepeatedValue<T>(rhs, fieldDescriptor, i);
                }
                if (!equal) {
                    generateDiffFromRepeatedField(lenLhs, lenRhs);
                }
            } else {
                const auto& firstValue = GetValue<T>(lhs, fieldDescriptor);
                const auto& secondValue = GetValue<T>(rhs, fieldDescriptor);
                const auto hasFirst = HasValue(lhs, fieldDescriptor);
                const auto hasSecond = HasValue(rhs, fieldDescriptor);
                // the first condition is proto2 specific
                if (hasFirst != hasSecond) {
                    generateDiffFromFieldPresence(firstValue, secondValue, hasFirst, hasSecond);
                } else if (firstValue != secondValue) {
                    generateDiffFromField(firstValue, secondValue);
                }
            }
            break;
        case IFC_PRESENCE:
            if (fieldDescriptor.is_repeated()) {
                const auto lenLhs = GetSize(lhs, fieldDescriptor);
                const auto lenRhs = GetSize(rhs, fieldDescriptor);
                if ((lenLhs > 0) != (lenRhs > 0)) {
                    generateDiffFromRepeatedField(lenLhs, lenRhs);
                }
            } else {
                const auto hasFirst = HasValue(lhs, fieldDescriptor);
                const auto hasSecond = HasValue(rhs, fieldDescriptor);
                if (hasFirst != hasSecond) {
                    generateDiffFromFieldPresence(GetValue<T>(lhs, fieldDescriptor), GetValue<T>(rhs, fieldDescriptor),
                                                  hasFirst, hasSecond);
                }
            }
            break;
        case EImportantFieldCheck_INT_MIN_SENTINEL_DO_NOT_USE_:
            break;
        case EImportantFieldCheck_INT_MAX_SENTINEL_DO_NOT_USE_:
            break;
    }
}

} // namespace

namespace NAlice {

TDifferReport TFieldDiffer::FindDiffs(const Message& lhs, const Message& rhs) {
    TDifferReport differReport;
    TString path;
    FindDiffsImpl(lhs, rhs, differReport, path);
    return differReport;
}

bool TFieldDiffer::ScanDescriptor(const Descriptor& descriptor) {
    bool traverseFurther = false;

    if (TraverseFields.contains(&descriptor)) {
        return MayContainImportantFields.contains(&descriptor);
    }

    auto& traverseFields = TraverseFields[&descriptor];
    for (int i = 0; i < descriptor.field_count(); i++) {
        const auto& fieldDescriptor = *descriptor.field(i);

        const auto importantFieldCheck = fieldDescriptor.options().GetExtension(ImportantFieldCheck);
        const bool importantField = importantFieldCheck != EImportantFieldCheck::IFC_NOTHING;

        bool containsImportantFields = fieldDescriptor.type() == FieldDescriptor::TYPE_MESSAGE &&
                                       !TStringBuf{fieldDescriptor.full_name()}.StartsWith("google.protobuf") &&
                                       ScanDescriptor(*fieldDescriptor.message_type());

        if (importantField || containsImportantFields) {
            traverseFields.push_back({.FieldDescriptor = &fieldDescriptor,
                                      .MayContainImportantFields = containsImportantFields,
                                      .ImportantFieldCheck = importantFieldCheck});
        }

        traverseFurther |= containsImportantFields || importantField;
    }

    if (traverseFurther) {
        MayContainImportantFields.insert(&descriptor);
    }

    return traverseFurther;
}

void TFieldDiffer::FindDiffsImpl(const Message& lhs, const Message& rhs, TDifferReport& differReport,
                                 const TString& path) {
    const auto& descriptor = *lhs.GetDescriptor();
    const auto& reflection = *lhs.GetReflection();

    if (!TraverseFields.contains(&descriptor)) {
        ScanDescriptor(descriptor);
    }

    for (const auto& fieldValue : TraverseFields[&descriptor]) {
        const auto* fieldDescriptor = fieldValue.FieldDescriptor;
        const auto importantFieldCheck = fieldValue.ImportantFieldCheck;
        const auto mayContainImportantFields = fieldValue.MayContainImportantFields;

        const auto newPath = path + (path.empty() ? "" : DELIM) + fieldDescriptor->name();
        if (importantFieldCheck != EImportantFieldCheck::IFC_NOTHING) {
            switch (fieldDescriptor->type()) {
                case FieldDescriptor::TYPE_DOUBLE: {
                    Check<double>(*fieldDescriptor, lhs, rhs, differReport, newPath, importantFieldCheck);
                    break;
                }
                case FieldDescriptor::TYPE_FLOAT: {
                    Check<float>(*fieldDescriptor, lhs, rhs, differReport, newPath, importantFieldCheck);
                    break;
                }
                case FieldDescriptor::TYPE_SFIXED64:
                    [[fallthrough]];
                case FieldDescriptor::TYPE_SINT64:
                    [[fallthrough]];
                case FieldDescriptor::TYPE_INT64: {
                    Check<i64>(*fieldDescriptor, lhs, rhs, differReport, newPath, importantFieldCheck);
                    break;
                }
                case FieldDescriptor::TYPE_FIXED64:
                    [[fallthrough]];
                case FieldDescriptor::TYPE_UINT64: {
                    Check<ui64>(*fieldDescriptor, lhs, rhs, differReport, newPath, importantFieldCheck);
                    break;
                }
                case FieldDescriptor::TYPE_SFIXED32:
                    [[fallthrough]];
                case FieldDescriptor::TYPE_SINT32:
                    [[fallthrough]];
                case FieldDescriptor::TYPE_INT32: {
                    Check<i32>(*fieldDescriptor, lhs, rhs, differReport, newPath, importantFieldCheck);
                    break;
                }
                case FieldDescriptor::TYPE_BOOL: {
                    Check<bool>(*fieldDescriptor, lhs, rhs, differReport, newPath, importantFieldCheck);
                    break;
                }
                case FieldDescriptor::TYPE_GROUP:
                    break;
                case FieldDescriptor::TYPE_MESSAGE: {
                    Check<const Message&>(*fieldDescriptor, lhs, rhs, differReport, newPath, importantFieldCheck);
                    continue;
                }
                case FieldDescriptor::TYPE_BYTES:
                    [[fallthrough]];
                case FieldDescriptor::TYPE_STRING: {
                    Check<TString>(*fieldDescriptor, lhs, rhs, differReport, newPath, importantFieldCheck);
                    break;
                }
                case FieldDescriptor::TYPE_FIXED32:
                    [[fallthrough]];
                case FieldDescriptor::TYPE_UINT32: {
                    Check<ui32>(*fieldDescriptor, lhs, rhs, differReport, newPath, importantFieldCheck);
                    break;
                }
                case FieldDescriptor::TYPE_ENUM: {
                    Check<const EnumValueDescriptor&>(*fieldDescriptor, lhs, rhs, differReport, newPath,
                                                      importantFieldCheck);
                    break;
                }
            }
        }
        if (mayContainImportantFields) {
            if (fieldDescriptor->is_repeated()) {
                const auto lenLhs = GetSize(lhs, *fieldDescriptor);
                const auto lenRhs = GetSize(rhs, *fieldDescriptor);
                if (lenLhs != lenRhs) {
                    Check<const Message&>(*fieldDescriptor, lhs, rhs, differReport, newPath,
                                          EImportantFieldCheck::IFC_DIFF);
                } else {
                    for (int i = 0; i < lenLhs; ++i) {
                        FindDiffsImpl(GetRepeatedValue<const Message&>(lhs, *fieldDescriptor, i),
                                      GetRepeatedValue<const Message&>(rhs, *fieldDescriptor, i), differReport,
                                      newPath);
                    }
                }
            } else {
                if (HasValue(lhs, *fieldDescriptor) != HasValue(rhs, *fieldDescriptor)) {
                    Check<const Message&>(*fieldDescriptor, lhs, rhs, differReport, newPath,
                                          EImportantFieldCheck::IFC_PRESENCE);
                } else if (reflection.HasField(lhs, fieldDescriptor) && reflection.HasField(rhs, fieldDescriptor)) {
                    FindDiffsImpl(GetValue<const Message&>(lhs, *fieldDescriptor),
                                  GetValue<const Message&>(rhs, *fieldDescriptor), differReport, newPath);
                }
            }
        }
    }
}

} // namespace NAlice
