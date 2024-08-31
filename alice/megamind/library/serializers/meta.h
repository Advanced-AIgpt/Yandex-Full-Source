#pragma once

#include <alice/megamind/library/models/directives/callback_directive_model.h>
#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/util/guid.h>

#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/megamind/protos/common/smart_home.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>

#include <alice/library/client/client_info.h>
#include <alice/library/proto/protobuf.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>

namespace NAlice::NMegamind {

inline constexpr TStringBuf ON_SUGGEST_DIRECTIVE_NAME = "on_suggest";

inline constexpr TStringBuf MM_DEEPLINK_PLACEHOLDER = "@@mm_deeplink#";

// Indicates if start_multiroom directive is used for iot groups.
inline constexpr TStringBuf IOT_GROUP_ROOM_ID = "all";

inline const TString ROOM_DEVICE_IDS = "room_device_ids";
inline const TString ROOM_ID = "room_id";
inline const TString MULTIROOM_TOKEN = "multiroom_token";

class TSerializerMeta {
public:
    explicit TSerializerMeta(const TString& scenarioName = "", const TString& requestId = "",
                             const TClientInfo& clientInfo = TClientInfo(TClientInfoProto()),
                             const TMaybe<TIoTUserInfo>& iotUserInfo = Nothing(),
                             const TSmartHomeInfo& smartHomeInfo = {},
                             const TSpeechKitRequestProto::TRequest::TAdditionalOptions& additionalOptions = {},
                             const IGuidGenerator& guidGenerator = TGuidGenerator(),
                             const TString& onSuggestScenarioName = TString{MM_PROTO_VINS_SCENARIO},
                             const TBuilderOptions& options = TBuilderOptions{.ProcessDefaultFields = true});

    [[nodiscard]] const TString& GetCallbackDirectiveScenarioName(const TString& directiveName) const;
    [[nodiscard]] const TString& GetScenarioName() const;
    [[nodiscard]] const TString& GetRequestId() const;
    [[nodiscard]] const TSmartHomeInfo& GetSmartHomeInfo() const;
    [[nodiscard]] const TClientInfo& GetClientInfo() const;
    [[nodiscard]] const TMaybe<TIoTUserInfo>& GetIoTUserInfo() const;
    [[nodiscard]] const TSpeechKitRequestProto::TRequest::TAdditionalOptions& GetAdditionalOptions() const;

    [[nodiscard]] const TIntrusivePtr<IGuidGenerator>& GetGuidGenerator() const;

    [[nodiscard]] TString WrapDialogId(const TString& dialogId) const;

    [[nodiscard]] const TBuilderOptions& GetBuilderOptions() const;

private:
    TString OnSuggestScenarioName;
    TString ScenarioName;
    TString RequestId;
    TClientInfo ClientInfo;
    TMaybe<TIoTUserInfo> IoTUserInfo;
    TSmartHomeInfo SmartHomeInfo;
    TSpeechKitRequestProto::TRequest::TAdditionalOptions AdditionalOptions;

    TIntrusivePtr<IGuidGenerator> GuidGenerator;

    TBuilderOptions BuilderOptions;
};

TProtoStructBuilder GetCallbackPayload(const google::protobuf::Struct& rawPayload,
                                       const TSerializerMeta& serializerMeta, const TString& directiveName);

google::protobuf::ListValue ToProtoList(const TVector<TString>& items);

using TIoTUserInfoDevices = ::google::protobuf::RepeatedPtrField<::NAlice::TIoTUserInfo_TDevice>;

void ForEachQuasarDeviceIdInLocation(const TIoTUserInfoDevices& ioTUserInfoDevices,
                                     const TString& locationId,
                                     std::function<void(const TString&)> onDeviceId);

void ForEachQuasarDeviceIdInLocation(const TIoTUserInfoDevices& ioTUserInfoDevices,
                                     const NScenarios::TLocationInfo& locationInfo,
                                     std::function<void(const TString&)> onDeviceId,
                                     const TString& currentDeviceId);

void ForEachQuasarDeviceIdThatSharesGroupWith(const TIoTUserInfoDevices& ioTUserInfoDevices, const TString& deviceId,
                                              std::function<void(const TString&)> onDeviceId);

} // namespace NAlice::NMegamind
