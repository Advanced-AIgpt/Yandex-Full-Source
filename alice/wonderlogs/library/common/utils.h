#pragma once

#include <alice/library/json/json.h>

#include <google/protobuf/message.h>

#include <library/cpp/binsaver/bin_saver.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/yson/node/node.h>
#include <library/cpp/yson/node/node_io.h>

#include <util/datetime/base.h>
#include <util/generic/hash_set.h>
#include <util/ysaveload.h>

namespace NAlice::NWonderlogs {

struct TEnvironment {
    THashSet<TString> UniproxyQloudProjects;
    THashSet<TString> UniproxyQloudApplications;
    THashSet<TString> MegamindEnvironments;

    [[nodiscard]] bool SuitableEnvironment(const TMaybe<TStringBuf>& uniproxyQloudProject,
                                           const TMaybe<TStringBuf>& uniproxyQloudApplication,
                                           const TMaybe<TStringBuf>& megamindEnvironment) const;

    Y_SAVELOAD_DEFINE(UniproxyQloudProjects, UniproxyQloudApplications, MegamindEnvironments);
    SAVELOAD(UniproxyQloudProjects, UniproxyQloudApplications, MegamindEnvironments);
};

TMaybe<TInstant> ParseDatetime(const TString& datetime);
TInstant ParseDatetimeOrFail(const TString& datetime);

[[nodiscard]] bool SkipRequest(TInstant timestamp, TInstant timestampFrom, TInstant timestampTo,
                               TDuration requestsShift);

[[nodiscard]] bool SkipWithoutMegamindData(TInstant timestamp, TInstant timestampFrom, TInstant timestampTo,
                                           TDuration requestsShift);

[[nodiscard]] bool SkipNotRequest(TInstant timestamp, TInstant timestampFrom, TInstant timestampTo);

TString NormalizeUuid(TString uuid);

void FixInvalidEnums(google::protobuf::Message& message, bool& containsUnknownFields, bool deleteUnknownFields = true);

TMaybe<TString> MaybeStringFromJson(const NJson::TJsonValue& json);
TMaybe<bool> MaybeBoolFromJson(const NJson::TJsonValue& json);

TMaybe<TString> MaybeStringFromYson(const NYT::TNode& yson);

TMaybe<TString> TryGenerateSetraceUrl(const TVector<TMaybe<TString>>& ids);

bool IsIpValid(TStringBuf ip);

ui64 HashStringToUi64(TStringBuf string);

template <typename T>
NYT::TNode NodeFromProto(const T& proto) {
    return NYT::NodeFromJsonString(NAlice::JsonStringFromProto(proto));
}

bool AliceTopic(TStringBuf topic);

} // namespace NAlice::NWonderlogs
