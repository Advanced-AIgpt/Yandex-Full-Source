#pragma once

#include <alice/wonderlogs/library/common/utils.h>

#include <alice/wonderlogs/protos/wonderlogs.pb.h>
#include <alice/wonderlogs/sdk/utils/speechkit_utils.h>

#include <alice/megamind/protos/analytics/megamind_analytics_info.pb.h>
#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/common/experiments.pb.h>
#include <alice/megamind/protos/guest/enrollment_headers.pb.h>
#include <alice/megamind/protos/guest/guest_data.pb.h>

#include <library/cpp/yson/node/node.h>

#include <util/generic/maybe.h>

namespace NAlice::NWonderlogs {

struct TDialog {
    explicit TDialog(const TWonderlog& wonderlog);

    NYT::TNode DumpNode() const;

    ui64 ServerTimeMs = 0;
    TString Uuid;
    ui64 SequenceNumber;
    NMegamind::TMegamindAnalyticsInfo MegamindAnalyticsInfo;
    TMaybe<TBiometryScoring> BiometryScoring;
    TMaybe<TBiometryClassification> BiometryClassification;
    TMaybe<TString> CallbackName;
    TMaybe<google::protobuf::Struct> CallbackArgs;
    ui64 ClientTime = 0;
    TString ClientTz;
    bool ContainsSensitiveData = false;
    TMaybe<TString> DeviceId;
    TMaybe<TString> DeviceRevision;
    TMaybe<TString> DialogId;
    bool DoNotUseUserLogs = false;
    NAlice::TExperimentsProto Experiments;
    NJson::TJsonValue Form = NJson::EJsonValueType::JSON_NULL;
    TMaybe<TString> FormName;
    TString Lang;
    TMaybe<double> LocationLat;
    TMaybe<double> LocationLon;
    TString MessageId;
    TMaybe<TString> Puid;
    TVinsLikeRequest Request;
    TString RequestId;
    NJson::TJsonValue RequestStat;
    TVinsLikeResponse Response;
    TString ResponseId;
    ui64 ServerTime = 0;
    TMaybe<TString> SessionId;
    TString Type;
    TMaybe<TString> UtteranceSource;
    TMaybe<TString> UtteranceText;
    TMaybe<bool> TrashOrEmptyRequest;
    TMaybe<TEnrollmentHeaders> EnrollmentHeaders;
    TMaybe<TGuestData> GuestData;
};

class TDialogsBuilder : public NNonCopyable::TMoveOnly {
public:
    using TError = NYT::TNode;
    using TErrors = TVector<TError>;

    explicit TDialogsBuilder(const TMaybe<TEnvironment>& productionEnvironment);

    TErrors AddWonderlog(TWonderlog wonderlog);
    bool Valid() const;
    NYT::TNode Build() &&;

private:
    TError GenerateError(const TString& reason, const TString& message);
    TMaybe<TString> Uuid_;
    TMaybe<TString> MessageId_;
    TMaybe<TString> MegamindRequestId_;
    TMaybe<TDialog> Dialog_;
    TMaybe<TEnvironment> ProductionEnvironment_;
};

} // namespace NAlice::NWonderlogs
