#include "utils.h"

#include "names.h"

#include <google/protobuf/unknown_field_set.h>

#include <library/cpp/openssl/crypto/sha.h>
#include <library/cpp/regex/pire/pire.h>

#include <util/datetime/parser.h>
#include <util/generic/maybe.h>
#include <util/string/util.h>

namespace NAlice::NWonderlogs {

namespace {

using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Message;

const TString IP_PATTERN =
    R"(((^\s*((([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]))\s*$)|(^\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:)))(%.+)?\s*$)))";
Pire::Scanner CompileRegexp(const TString& pattern) {
    TVector<char> ucs4;
    Pire::Encodings::Latin1().FromLocal(pattern.begin(), pattern.end(), std::back_inserter(ucs4));
    return Pire::Lexer(ucs4.begin(), ucs4.end()).Parse().Compile<Pire::Scanner>();
}

const auto IP_SCANNER = CompileRegexp(IP_PATTERN);

const THashSet<TStringBuf> NOT_ALICE_TOPICS = {"chats", "chats-gpu", "messenger"};

} // namespace

bool TEnvironment::SuitableEnvironment(const TMaybe<TStringBuf>& uniproxyQloudProject,
                                       const TMaybe<TStringBuf>& uniproxyQloudApplication,
                                       const TMaybe<TStringBuf>& megamindEnvironment) const {
    if (uniproxyQloudProject && !UniproxyQloudProjects.contains(*uniproxyQloudProject)) {
        return false;
    }
    if (uniproxyQloudApplication && !UniproxyQloudApplications.contains(*uniproxyQloudApplication)) {
        return false;
    }
    if (megamindEnvironment && !MegamindEnvironments.contains(*megamindEnvironment)) {
        return false;
    }
    return true;
}

TMaybe<TInstant> ParseDatetime(const TString& datetime) {
    TIso8601DateTimeParser parser;
    if (parser.ParsePart(datetime.data(), datetime.size())) {
        return parser.GetDateTimeFields().ToInstant({});
    }
    return {};
}

TInstant ParseDatetimeOrFail(const TString& datetime) {
    const auto timestamp = ParseDatetime(datetime);
    if (timestamp) {
        return *timestamp;
    }
    Y_FAIL("Could not convert datetime %s to timestamp", datetime.c_str());
}

bool SkipRequest(TInstant timestamp, TInstant timestampFrom, TInstant timestampTo, TDuration requestsShift) {
    return !(timestamp >= timestampFrom - requestsShift && timestamp < timestampTo);
}

bool SkipWithoutMegamindData(TInstant timestamp, TInstant timestampFrom, TInstant timestampTo,
                             TDuration requestsShift) {
    return !(timestamp >= timestampFrom - requestsShift && timestamp < timestampTo - requestsShift);
}

bool SkipNotRequest(TInstant timestamp, TInstant timestampFrom, TInstant timestampTo) {
    return !(timestamp >= timestampFrom && timestamp < timestampTo);
}

TString NormalizeUuid(TString uuid) {
    RemoveAll(uuid, '-');
    uuid.to_lower();
    return uuid;
}

void FixInvalidEnums(Message& message, bool& containsUnknownFields, const bool deleteUnknownFields) {
    const auto* reflection = message.GetReflection();
    if (!reflection->GetUnknownFields(message).empty()) {
        containsUnknownFields = true;
        if (deleteUnknownFields) {
            reflection->MutableUnknownFields(&message)->Clear();
        }
    }

    {
        std::vector<const FieldDescriptor*> fieldDescriptors;
        reflection->ListFields(message, &fieldDescriptors);
        for (const auto* fieldDescriptor : fieldDescriptors) {
            if (fieldDescriptor->type() == FieldDescriptor::TYPE_ENUM) {
                const auto correctEnumValue = [](Message& message, const FieldDescriptor& fieldDescriptor) -> bool {
                    return fieldDescriptor.enum_type()->FindValueByNumber(
                        message.GetReflection()->GetEnumValue(message, &fieldDescriptor));
                };
                if (fieldDescriptor->is_required()) {
                    if (!correctEnumValue(message, *fieldDescriptor)) {
                        reflection->SetEnum(&message, fieldDescriptor, fieldDescriptor->default_value_enum());
                    }
                } else if (fieldDescriptor->is_optional()) {
                    if (!correctEnumValue(message, *fieldDescriptor)) {
                        reflection->ClearField(&message, fieldDescriptor);
                    }
                } else if (fieldDescriptor->is_repeated()) {
                    for (int i = 0; i < reflection->FieldSize(message, fieldDescriptor); ++i) {
                        const auto value = reflection->GetRepeatedEnumValue(message, fieldDescriptor, i);
                        if (!fieldDescriptor->enum_type()->FindValueByNumber(value)) {
                            reflection->SetRepeatedEnum(&message, fieldDescriptor, i,
                                                        fieldDescriptor->default_value_enum());
                        }
                    }
                }
            } else if (fieldDescriptor->type() == FieldDescriptor::TYPE_MESSAGE) {
                if (TStringBuf{fieldDescriptor->full_name()}.StartsWith("google.protobuf")) {
                    continue;
                }
                if (fieldDescriptor->is_repeated()) {
                    for (int i = 0; i < reflection->FieldSize(message, fieldDescriptor); ++i) {
                        FixInvalidEnums(*reflection->MutableRepeatedMessage(&message, fieldDescriptor, i),
                                        containsUnknownFields, deleteUnknownFields);
                    }
                } else {
                    FixInvalidEnums(*reflection->MutableMessage(&message, fieldDescriptor), containsUnknownFields,
                                    deleteUnknownFields);
                }
            }
        }
    }
}

TMaybe<TString> MaybeStringFromJson(const NJson::TJsonValue& json) {
    if (json.GetType() == NJson::JSON_STRING) {
        return json.GetString();
    }
    return {};
}

TMaybe<bool> MaybeBoolFromJson(const NJson::TJsonValue& json) {
    if (json.GetType() == NJson::JSON_BOOLEAN) {
        return json.GetBoolean();
    }
    return {};
}

TMaybe<TString> MaybeStringFromYson(const NYT::TNode& yson) {
    if (yson.IsString()) {
        return yson.AsString();
    }
    return {};
}

TMaybe<TString> TryGenerateSetraceUrl(const TVector<TMaybe<TString>>& ids) {
    for (const auto& id : ids) {
        if (id && !id->empty()) {
            return SETRACE_URL_PREFIX + *id;
        }
    }
    return {};
}

bool IsIpValid(TStringBuf ip) {
    return NPire::Runner(IP_SCANNER).Begin().Run(ip).End();
}

ui64 HashStringToUi64(TStringBuf string) {
    const auto digest = NOpenSsl::NSha256::Calc(string);
    ui64 step = sizeof(NOpenSsl::NSha256::TDigest::value_type) * 8;
    ui64 res = 0;
    for (size_t i = 0; i < NOpenSsl::NSha256::DIGEST_LENGTH; ++i) {
        res ^= static_cast<ui64>(digest[i]) << (step * (i % sizeof(ui64)));
    }
    return res;
}

bool AliceTopic(const TStringBuf topic) {
    return !NOT_ALICE_TOPICS.contains(topic);
}

} // namespace NAlice::NWonderlogs
