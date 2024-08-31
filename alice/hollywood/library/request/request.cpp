#include "request.h"

#include <alice/hollywood/library/util/service_context.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/location.pb.h>

#include <alice/library/scenarios/data_sources/data_sources.h>

namespace NAlice::NHollywood {

namespace {

const TString STRING_TYPE = "string";

// Like find_if, but with pointers.
//
// NOTE(a-square): TContainer contains the const qualifier by design
// so that the return type has the appropriate constness.
//
// TODO(a-square): consider moving this into util, as a companion to FindPtr
template <typename TContainer, typename TPredicate>
decltype(&*std::declval<TContainer>().begin()) FindPtrIf(TContainer& container, TPredicate&& predicate) {
    const auto end = container.end();
    const auto it = std::find_if(container.begin(), end, std::forward<TPredicate>(predicate));
    if (it == end) {
        return nullptr;
    }

    return &*it;
}

const NScenarios::TDataSource* GetDataSourceImpl(const google::protobuf::Map<int, NScenarios::TDataSource>& dataSources,
                                                 NAlice::EDataSourceType sourceType,
                                                 THashMap<NAlice::EDataSourceType, NScenarios::TDataSource>& contextDataSourcesCache,
                                                 const NAppHost::IServiceContext& serviceCtx)
{
    if (const auto it = dataSources.find(sourceType); it != dataSources.end()) {
        return &it->second;
    }

    const auto& items = serviceCtx.GetProtobufItemRefs(NScenarios::GetDataSourceContextName(sourceType),
                                                       NAppHost::EContextItemSelection::Input);
    if (items.empty()) {
        return nullptr;
    }
    NScenarios::TDataSource dataSource;
    items.front().Fill(&dataSource);

    const auto [ref, _] = contextDataSourcesCache.emplace(sourceType, dataSource);
    return &ref->second;
}

} // namespace


// TScenarioBaseRequestWrapper -------------------------------------------------

TScenarioBaseRequestWrapper::TScenarioBaseRequestWrapper(const NScenarios::TScenarioBaseRequest& request, const NAppHost::IServiceContext& serviceCtx)
    : BaseRequestProto_(request)
    , ExpFlags_{ExpFlagsFromProto(BaseRequestProto_.GetExperiments())}
    , ClientInfo_{BaseRequestProto_.GetClientInfo()}
    , ContentRestrictionLevel_{CalculateContentRestrictionLevel(BaseRequestProto_.GetDeviceState().GetDeviceConfig().GetContentSettings(),
                                                                BaseRequestProto_.GetOptions().GetFiltrationLevel())}
    , RequestId_(BaseRequestProto_.GetRequestId())
    , Location_(BaseRequestProto_.GetLocation())
    , ServiceCtx_(serviceCtx)
{
    RawPersonalData_.SetType(NJson::JSON_MAP);
    if (BaseRequestProto_.HasOptions()) {
        const auto rawPersonalData = BaseRequestProto_.GetOptions().GetRawPersonalData();
        if (!rawPersonalData.empty()) {
            Y_ENSURE(NJson::ReadJsonFastTree(rawPersonalData, &RawPersonalData_));
        }
    }
}

// TScenarioInputWrapper -------------------------------------------------------

TString TScenarioInputWrapper::Utterance() const {
    switch (Proto_.GetEventCase()) {
        case NScenarios::TInput::kText:
            return Proto_.GetText().GetUtterance();
        case NScenarios::TInput::kVoice:
            return Proto_.GetVoice().GetUtterance();
        case NScenarios::TInput::kImage:
        case NScenarios::TInput::kMusic:
        case NScenarios::TInput::kCallback:
        default:
            return {};
    }
}

bool TScenarioInputWrapper::IsTextInput() const {
    return Proto_.GetEventCase() == NScenarios::TInput::kText;
}

bool TScenarioInputWrapper::IsVoiceInput() const {
    return Proto_.GetEventCase() == NScenarios::TInput::kVoice;
}

const TPtrWrapper<TSemanticFrame> TScenarioInputWrapper::FindSemanticFrame(const TStringBuf frameName) const {
    const TSemanticFrame* frame = FindPtrIf(Proto_.GetSemanticFrames(), [frameName](const TSemanticFrame& frame) {
        return frame.GetName() == frameName;
    });
    return TPtrWrapper<TSemanticFrame>(frame, frameName);
}

TFrame TScenarioInputWrapper::CreateRequestFrame(const TStringBuf frameName) const {
    const auto frame = FindSemanticFrame(frameName);
    Y_ENSURE(frame, "Semantic frame not found: " << frameName);
    return TFrame::FromProto(*frame);
}

TMaybe<TFrame> TScenarioInputWrapper::TryCreateRequestFrame(const TStringBuf frameName) const {
    if (const auto frame = FindSemanticFrame(frameName)) {
        return TFrame::FromProto(*frame);
    }
    return Nothing();
}


const NScenarios::TCallbackDirective* TScenarioInputWrapper::GetCallback() const {
    return Proto().HasCallback() ? &Proto().GetCallback() : nullptr;
}


// TScenarioRunRequestWrapper --------------------------------------------------

const NScenarios::TDataSource* TScenarioRunRequestWrapper::GetDataSource(NAlice::EDataSourceType sourceType) const {
    return GetDataSourceImpl(Proto_.GetDataSources(), sourceType, ContextDataSourcesCache_, ServiceCtx());
}

// --------------------------------------------------

const NAlice::TBlackBoxUserInfo* GetUserInfoProto(const TScenarioRunRequestWrapper& request) {
    const auto* userInfoPtr = request.GetDataSource(NAlice::EDataSourceType::BLACK_BOX);
    if (!userInfoPtr) {
        return nullptr;
    }
    const auto& userInfoProto = userInfoPtr->GetUserInfo();
    return &userInfoProto;
}

// --------------------------------------------------

const NAlice::TIoTUserInfo* GetIoTUserInfoProto(const TScenarioRunRequestWrapper& request) {
    const auto* userInfoPtr = request.GetDataSource(NAlice::EDataSourceType::IOT_USER_INFO);
    if (!userInfoPtr) {
        return nullptr;
    }
    const auto& iotUserInfoProto = userInfoPtr->GetIoTUserInfo();
    return &iotUserInfoProto;
}

// --------------------------------------------------

const NAlice::TEnvironmentState* GetEnvironmentStateProto(const TScenarioRunRequestWrapper& request) {
    const auto* environmentStatePtr = request.GetDataSource(NAlice::EDataSourceType::ENVIRONMENT_STATE);
    if (!environmentStatePtr) {
        return nullptr;
    }
    return &environmentStatePtr->GetEnvironmentState();
}

// --------------------------------------------------

TStringBuf GetUid(const TScenarioRunRequestWrapper& request) {
    const auto* userInfoProtoPtr = GetUserInfoProto(request);
    if (!userInfoProtoPtr) {
        return TStringBuf("");
    }
    return userInfoProtoPtr->GetUid();
}

// --------------------------------------------------

const NAlice::TGuestData* GetGuestDataProto(const TScenarioRunRequestWrapper& request) {
    const auto* guestDataPtr = request.GetDataSource(NAlice::EDataSourceType::GUEST_DATA);
    if (!guestDataPtr) {
        return nullptr;
    }
    const auto& guestDataProto = guestDataPtr->GetGuestData();
    return &guestDataProto;
}

// --------------------------------------------------

const NAlice::TGuestOptions* GetGuestOptionsProto(const TScenarioRunRequestWrapper& request) {
    const auto* guestOptionsPtr = request.GetDataSource(NAlice::EDataSourceType::GUEST_OPTIONS);
    if (!guestOptionsPtr) {
        return nullptr;
    }
    const auto& guestOptionsProto = guestOptionsPtr->GetGuestOptions();
    return &guestOptionsProto;
}

// --------------------------------------------------

TMaybe<TGeoPosition> InitGeoPositionFromRequest(const NScenarios::TScenarioBaseRequest& request) {
    TMaybe<TGeoPosition> pos;
    if (request.HasLocation()) {
        const auto& location = request.GetLocation();
        if (location.HasLat() && location.HasLon()) {
            pos = TGeoPosition();
            pos->Lon = location.GetLon();
            pos->Lat = location.GetLat();
        }
    }
    return pos;
}

// --------------------------------------------------

TMaybe<TFrame> TryGetFrame(const TStringBuf frameName, const TMaybe<TFrame>& callbackFrame, const TScenarioInputWrapper& input) {
    if (callbackFrame.Defined() && callbackFrame->Name() == frameName) {
        return callbackFrame;
    }
    if (const auto frame = input.FindSemanticFrame(frameName)) {
        return TFrame::FromProto(*frame);
    }
    return Nothing();
}

} // namespace NAlice::NHollywood
