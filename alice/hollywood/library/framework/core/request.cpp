//
// HOLLYWOOD FRAMEWORK
// Base request information
//

#include "request.h"
#include "render_impl.h"
#include "nlg_helper.h"
#include "scenario_baseinit.h"
#include "scenario.h"

#include "alice/hollywood/library/framework/proto/framework_state.pb.h"
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/request/experiments.h>

#include <alice/library/geo/geodb.h>

#include <alice/library/restriction_level/restriction_level.h>
#include <alice/library/scenarios/data_sources/data_sources.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/protos/extensions/extensions.pb.h>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.pb.h>

#include <library/cpp/timezone_conversion/civil.h>


namespace NAlice::NHollywoodFw {

namespace {

static NScenarios::TInput EmptyInput;

} // anoniumous namespace

class TRunRequestImpl {
public:
    const NScenarios::TDataSource* GetDataSourceDirect(const NScenarios::TScenarioRunRequest& runRequest,
                                                       NAlice::EDataSourceType sourceType) {
        const auto& dataSources = runRequest.GetDataSources();
        const auto& it = dataSources.find(sourceType);
        if (it != dataSources.end()) {
            return &it->second;
        }
        return nullptr;
    }
    const NScenarios::TDataSource* GetDataSourceCache(const NAppHost::IServiceContext& serviceCtx,
                                                      NAlice::EDataSourceType sourceType) const {

        const auto& it = ContextDataSourcesCache_.find(sourceType);
        if (it != ContextDataSourcesCache_.end()) {
            return &it->second;
        }
        const auto& items = serviceCtx.GetProtobufItemRefs(NScenarios::GetDataSourceContextName(sourceType),
                                                        NAppHost::EContextItemSelection::Input);
        if (items.empty()) {
            return nullptr;
        }
        NScenarios::TDataSource dataSource;
        items.front().Fill(&dataSource);

        const auto [ref, _] = ContextDataSourcesCache_.emplace(sourceType, dataSource);
        return &ref->second;
    }
private:
    mutable THashMap<NAlice::EDataSourceType, NScenarios::TDataSource> ContextDataSourcesCache_;
};


/*
    TRequest ctor
*/
TRequest::TRequest(const NScenarios::TRequestMeta& requestMeta,
                   const NScenarios::TScenarioBaseRequest& baseRequest,
                   const NScenarios::TInput& inputProto,
                   const NAppHost::IServiceContext& serviceCtx,
                   const TProtoHwScene* protoHwScene,
                   NAlice::NHollywood::IGlobalContext& globalCtx,
                   TRTLogger& logger,
                   const TApphostNodeInfo& apphostNodeInfo,
                   TMaybe<NHollywood::TCompiledNlgComponent> nlg)
    : RequestMeta_(requestMeta)
    , ServiceCtx_(serviceCtx)
    , System_(requestMeta, globalCtx, baseRequest)
    , Debug_(logger)
    , Client_(baseRequest, requestMeta)
    , Flags_(baseRequest, logger)
    , Input_(baseRequest, inputProto, requestMeta)
    , User_(baseRequest, logger)
    , Nlg_(*this, nlg)
    , AI_(protoHwScene)
    , ApphostInfo_(apphostNodeInfo)
{
}

void TRequest::ExportToProto(TProtoHwScene& hwSceneProto) const {
    AI_.ExportToProto(hwSceneProto);
}

/*
    TRequest::TSystem class functions
*/
TRequest::TSystem::TSystem(const NScenarios::TRequestMeta& requestMeta,
                           NAlice::NHollywood::IGlobalContext& globalCtx,
                           const NScenarios::TScenarioBaseRequest& baseRequest)
    : RequestId_(requestMeta.GetRequestId())
    , Random_(TRng{requestMeta.GetRandomSeed()}) // Seed will be re-initialized later
    , ServerTime_(std::chrono::milliseconds(baseRequest.GetServerTimeMs()))
    , CommonResources_(globalCtx.CommonResources())
    , FastData_(globalCtx.FastData())
    , Sensors_(globalCtx.Sensors())
{
}

/*
    TRequest::TClient class functions
*/
TRequest::TClient::TClient(const NScenarios::TScenarioBaseRequest& baseRequestProto, const NScenarios::TRequestMeta& requestMeta)
    : BaseRequestProto_(baseRequestProto)
    , DeviceInfo_(requestMeta)
    , ClientInfo_(baseRequestProto.GetClientInfo())
{
}

bool TRequest::TClient::TryGetMessageImpl(google::protobuf::Message& object) const {
    const TString& prototypeName = object.GetTypeName();
    const google::protobuf::Reflection* ref = BaseRequestProto_.GetReflection();
    std::vector<const google::protobuf::FieldDescriptor*> allFields;
    ref->ListFields(BaseRequestProto_, &allFields);

    for (const auto& it : allFields) {
        if (it->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
            const google::protobuf::Message& childMsg = ref->GetMessage(BaseRequestProto_, it);
            if (prototypeName == childMsg.GetTypeName()) {
                object.CopyFrom(childMsg);
                return true;
            }
        }
    }
    return false;
}

/*
    Get client info in GMT
    Note client info resolution is 1 second!
*/
std::chrono::milliseconds TRequest::TClient::GetClientTimeMsGMT() const {
    return std::chrono::seconds(ClientInfo_.Epoch);
}

/*
    Get client info in local timezone + return timezone information
*/
std::chrono::milliseconds TRequest::TClient::GetClientTimeMs(TString& tz) const {
    tz = ClientInfo_.Timezone;
    const NDatetime::TCivilSecond timeSec = NDatetime::TCivilSecond(ClientInfo_.Epoch);
    return std::chrono::seconds(NDatetime::Convert(timeSec, NDatetime::GetTimeZone(tz)).Seconds());
}

/*
    Get client time zone only
*/
const TString& TRequest::TClient::GetTimezone() const {
    return ClientInfo_.Timezone;
}

/*
TInstant TRequest::TClient::GetClientTime() const {
    return TInstant::Seconds(ClientInfo_.Epoch);
}
*/

/*
    Return reference to TInterface object
    (supported client features)
    You have to include <alice/megaming/protos/scenarios/response.pb.h> to access to these methods
    TODO [DD]: this intefrace will be replaced with jinja2 codogenerated interface soon
*/
const NScenarios::TInterfaces& TRequest::TClient::GetInterfaces() const {
    return BaseRequestProto_.GetInterfaces();
}

/*
    TRequest::TClient::TDeviceInfo class functions
*/
TRequest::TClient::TDeviceInfo::TDeviceInfo(const NScenarios::TRequestMeta& requestMeta)
    : DeviceLang_(LanguageByName(requestMeta.GetLang()))
{
    Y_ENSURE(DeviceLang_ != ELanguage::LANG_UNK, "Device language is: " << requestMeta.GetLang());
}

/*
    TRequest::TInput::TDeviceInfo class functions
*/
TRequest::TInput::TInput(const NScenarios::TScenarioBaseRequest& baseRequest, const NScenarios::TInput& input, const NScenarios::TRequestMeta& requestMeta)
    : UserLang_(LanguageByName(requestMeta.GetUserLang() ? requestMeta.GetUserLang() : requestMeta.GetLang()))
    , BaseRequest_(baseRequest)
    , InputProto_(input)
    , InputType_(EInputType::Unknown)
{
    Y_ENSURE(UserLang_ != ELanguage::LANG_UNK, "User language is: " << requestMeta.GetUserLang());

    switch (InputProto_.GetEventCase()) {
        case NScenarios::TInput::kText:
            Utterance_ = InputProto_.GetText().GetUtterance();
            InputType_ = EInputType::Text;
            break;
        case NScenarios::TInput::kVoice:
            Utterance_ = InputProto_.GetVoice().GetUtterance();
            InputType_ = EInputType::Voice;
            break;
        case NScenarios::TInput::kImage:
            InputType_ = EInputType::Image;
            break;
        case NScenarios::TInput::kMusic:
            InputType_ = EInputType::Music;
            break;
        case NScenarios::TInput::kCallback:
            InputType_ = EInputType::Callback;
            break;
        case NScenarios::TInput::kTypedCallback:
            InputType_ = EInputType::TypedCallback;
            break;
        case NScenarios::TInput::EVENT_NOT_SET:
            // Remains unchanged
            break;
    }
}

/*
    TInput dtor
    Removes all previously allocated semantic frames and free memory
*/
TRequest::TInput::~TInput() {
    for (auto it : Frames_) {
        delete it;
    }
}

bool TRequest::TInput::HasSemanticFrame(TStringBuf frameName) const {
    return AnyOf(InputProto_.GetSemanticFrames(), [frameName](const auto& sf) {
        return sf.GetName() == frameName;
    });
}
/*
    Find and create new semantic frame
*/
const NHollywood::TPtrWrapper<NHollywood::TFrame> TRequest::TInput::FindSemanticFrame(TStringBuf frameName) const {
    for (const auto& it : InputProto_.GetSemanticFrames()) {
        if (it.GetName() == frameName) {
            NHollywood::TFrame* frame = new NHollywood::TFrame(NHollywood::TFrame::FromProto(it));
            Frames_.push_back(frame);
            return NHollywood::TPtrWrapper<NHollywood::TFrame>(frame, frameName);
        }
    }
    return NHollywood::TPtrWrapper<NHollywood::TFrame>(nullptr, frameName);
}

/*
    Get a callback directive (is exist) from input request
*/
const NHollywood::TPtrWrapper<NScenarios::TCallbackDirective> TRequest::TInput::FindCallback() const {
    if (!InputProto_.HasCallback()) {
        return NHollywood::TPtrWrapper<NScenarios::TCallbackDirective>(nullptr, "Callback");
    }
    return NHollywood::TPtrWrapper<NScenarios::TCallbackDirective>(&InputProto_.GetCallback(), "Callback");
}

/*
    Find typed semantic frame
    Internal implementation
*/
bool TRequest::TInput::FindTSFImpl(google::protobuf::Message& proto) const {
    const auto& frameName = proto.GetDescriptor()->options().GetExtension(SemanticFrameName);
    return FindTSFImpl(frameName, proto);
}

bool TRequest::TInput::FindTSFImpl(TStringBuf frameName, google::protobuf::Message& proto) const {
    const TString& prototypeName = proto.GetTypeName();
    for (const auto& frame : InputProto_.GetSemanticFrames()) {
        if (frame.GetName() == frameName && frame.HasTypedSemanticFrame()) {
            // Check TSF data
            const ::NAlice::TTypedSemanticFrame& tsf = frame.GetTypedSemanticFrame();
            const google::protobuf::Reflection* ref = tsf.GetReflection();
            std::vector<const google::protobuf::FieldDescriptor*> allFields;
            ref->ListFields(tsf, &allFields);

            // Note allFields should have 1 member, because this structure contais 1 Oneof only!
            for (const auto& it : allFields) {
                if (it->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
                    const google::protobuf::Message& childMsg = ref->GetMessage(tsf, it);
                    if (prototypeName == childMsg.GetTypeName()) {
                        proto.CopyFrom(childMsg);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/*
    TUser class ctor
*/
TRequest::TUser::TUser(const NScenarios::TScenarioBaseRequest& baseRequest, TRTLogger& logger)
    : ContentSettings_(CalculateContentRestrictionLevel(baseRequest.GetDeviceState().GetDeviceConfig().GetContentSettings(),
                                                        baseRequest.GetOptions().GetFiltrationLevel()))
{
    const auto mode = baseRequest.GetUserPreferences().GetFiltrationMode();
    switch (mode) {
        case NScenarios::TUserPreferences::NoFilter:
            FiltrationMode_ = EFiltrationMode::NoFilter;
            break;
        case NScenarios::TUserPreferences::Moderate:
            FiltrationMode_ = EFiltrationMode::Moderate;
            break;
        case NScenarios::TUserPreferences::FamilySearch:
            FiltrationMode_ = EFiltrationMode::FamilySearch;
            break;
        case NScenarios::TUserPreferences::Safe:
            FiltrationMode_ = EFiltrationMode::Safe;
            break;
        default:
            LOG_ERROR(logger) << "Undefined FiltrationMode level: " << static_cast<int>(mode);
            FiltrationMode_ = EFiltrationMode::Safe;
    }
}

/*
    NlgHelper()
*/
TRequest::TNlgHelper::TNlgHelper(const TRequest& request, TMaybe<NHollywood::TCompiledNlgComponent> nlg)
    : Request_(request)
    , Nlg_(nlg)
{
}

NNlg::TRenderPhraseResult TRequest::TNlgHelper::RenderPhrase(TStringBuf nlgName, TStringBuf phraseName, const google::protobuf::Message& msgProto) const {
    return RenderPhrase(nlgName, phraseName, NPrivate::Proto2Json(msgProto));
}

/*
    TODO: This function must be refactored and merged with the same code from TRender
*/
NNlg::TRenderPhraseResult TRequest::TNlgHelper::RenderPhrase(TStringBuf nlgName, TStringBuf phraseName, const NJson::TJsonValue& jsonContext) const {
    if (!Nlg_.Defined()) {
        LOG_ERROR(Request_.Debug().Logger()) << "Can not render result: NLG is not defined";
        return NNlg::TRenderPhraseResult{};
    }
    NHollywood::TNlgData nlgData = NPrivate::ConstructNlgData(Request_, jsonContext, NJson::TJsonValue{});

    const ELanguage language = Request_.Input().GetUserLanguage();
    IRng& rng = Request_.System().Random();
    // NlgRenderHistoryRecordStorage_.TrackRenderPhrase(nlgName, phraseName, nlgData, language);
    const auto ret = Nlg_->RenderPhrase(nlgName, phraseName, language, rng, nlgData);

    if (ret.Text.Empty() && ret.Voice.Empty()) {
        LOG_WARNING(Request_.Debug().Logger()) << "You called NLG renderer but both text and voice are empty. " <<
            "NlgName: " << nlgName << "; Phrase: " << phraseName << "; Language: " << NameByLanguage(language);
    }
    return ret;
}

bool TRequest::TNlgHelper::HasPhrase(TStringBuf nlgName, TStringBuf phraseName) const {
    return Nlg_ && Nlg_->HasPhrase(nlgName, phraseName, Request_.Input().GetUserLanguage());
}


/*
    TAnalyticsInfo class members
*/
TRequest::TAnalyticsInfo::TAnalyticsInfo(const TProtoHwScene* protoHwScene) {
    if (protoHwScene != nullptr) {
        const auto& ai = protoHwScene->GetCustomAnalyticsInfo();
        if (ai.HasSemanticFrameName()) {
            Intent_ = ai.GetSemanticFrameName();
        }
        if (ai.HasAiPsn()) {
            Psn_ = ai.GetAiPsn();
        }
        if (ai.HasAiFrameName()) {
            OutputFrameName_ = ai.GetAiFrameName();
        }
        for (const auto& it : ai.GetActions()) {
            NScenarios::TAnalyticsInfo_TAction a;
            a.CopyFrom(it);
            Actions_.emplace_back(std::move(a));
        }
        for (const auto& it : ai.GetObjects()) {
            NScenarios::TAnalyticsInfo_TObject a;
            a.CopyFrom(it);
            Objects_.emplace_back(std::move(a));
        }
        for (const auto& it : ai.GetEvents()) {
            NScenarios::TAnalyticsInfo_TEvent a;
            a.CopyFrom(it);
            Events_.emplace_back(std::move(a));
        }
    }
}

TRequest::TAnalyticsInfo::~TAnalyticsInfo() {
}

void TRequest::TAnalyticsInfo::AddAction(NScenarios::TAnalyticsInfo_TAction&& action) {
    Actions_.emplace_back(std::move(action));
}
void TRequest::TAnalyticsInfo::AddObject(NScenarios::TAnalyticsInfo_TObject&& object) {
    Objects_.emplace_back(std::move(object));
}
void TRequest::TAnalyticsInfo::AddEvent(NScenarios::TAnalyticsInfo_TEvent&& event) {
    Events_.emplace_back(std::move(event));
}

void TRequest::TAnalyticsInfo::ExportToProto(TProtoHwScene& hwSceneProto) const {
    if (Intent_) {
        hwSceneProto.MutableCustomAnalyticsInfo()->SetSemanticFrameName(Intent_);
    }
    if (Psn_) {
        hwSceneProto.MutableCustomAnalyticsInfo()->SetAiPsn(Psn_);
    }
    if (OutputFrameName_) {
        hwSceneProto.MutableCustomAnalyticsInfo()->SetAiFrameName(OutputFrameName_);
    }
    for (const auto& it : Actions_) {
        hwSceneProto.MutableCustomAnalyticsInfo()->AddActions()->CopyFrom(it);
    }
    for (const auto& it : Objects_) {
        hwSceneProto.MutableCustomAnalyticsInfo()->AddObjects()->CopyFrom(it);
    }
    for (const auto& it : Events_) {
        hwSceneProto.MutableCustomAnalyticsInfo()->AddEvents()->CopyFrom(it);
    }
}

void TRequest::TAnalyticsInfo::BuildAnswer(NScenarios::TScenarioResponseBody* response, NPrivate::TAICustomData& data) const {
    data.Psn = Psn_;
    data.Intent = Intent_;
    data.OutputFrameName = OutputFrameName_;

    for (const auto& it : Actions_) {
        response->MutableAnalyticsInfo()->AddActions()->CopyFrom(it);
    }
    for (const auto& it : Objects_) {
        response->MutableAnalyticsInfo()->AddObjects()->CopyFrom(it);
    }
    for (const auto& it : Events_) {
        response->MutableAnalyticsInfo()->AddEvents()->CopyFrom(it);
    }
}

//
// Run request
//
TRunRequest::TRunRequest(const NScenarios::TRequestMeta& requestMeta,
                         const NScenarios::TScenarioRunRequest& runRequest,
                         const NAppHost::IServiceContext& serviceCtx,
                         const TProtoHwScene* protoHwScene,
                         NAlice::NHollywood::IGlobalContext& globalCtx,
                         TRTLogger& logger,
                         const TApphostNodeInfo& apphostNodeInfo,
                         TMaybe<NHollywood::TCompiledNlgComponent> nlg)
    : TRequest(requestMeta,
               runRequest.GetBaseRequest(),
               runRequest.HasInput() ? runRequest.GetInput() : EmptyInput,
               serviceCtx,
               protoHwScene,
               globalCtx,
               logger,
               apphostNodeInfo,
               nlg)
    , RunRequest_(runRequest)
    , ServiceCtx_(serviceCtx)
    , Impl_(new TRunRequestImpl)
{
}

TRunRequest::~TRunRequest() {
    delete Impl_;
}

/*
*/
NAlice::TUserLocation TRunRequest::GetUserLocation() const {
    const auto& tz = RunRequest_.GetBaseRequest().GetClientInfo().GetTimezone();
    const NScenarios::TDataSource* userLocationPtr = nullptr;
    userLocationPtr = Impl_->GetDataSourceDirect(RunRequest_, NAlice::EDataSourceType::USER_LOCATION);
    if (userLocationPtr == nullptr) {
        userLocationPtr = Impl_->GetDataSourceCache(ServiceCtx_, NAlice::EDataSourceType::USER_LOCATION);
    }
    if (userLocationPtr == nullptr) {
        LOG_ERROR(Debug().Logger()) << "Unable to retrive user location info. Please check DataSources: [USER_LOCATION] in MM config.";
        return TUserLocation{tz, /* tld */ "ru"};
    }
    return TUserLocation{userLocationPtr->GetUserLocation(), tz};
}

const NScenarios::TDataSource* TRunRequest::GetDataSource(NAlice::EDataSourceType sourceType, bool logError) const {
    const auto* ds = Impl_->GetDataSourceDirect(RunRequest_, sourceType);
    if (ds == nullptr) {
        ds = Impl_->GetDataSourceCache(ServiceCtx_, sourceType);
    }
    if (ds == nullptr && logError) {
        LOG_ERROR(Debug().Logger()) << "Unable to get data source " << NScenarios::GetDataSourceContextName(sourceType);
    }
    return ds;
}

/*
    Get UserID (UID) from source request
    Note:
        1. This function availabe in Run stage only. You have to store UID in the scene arguments to continue use it
        2. You must have an access to blackbox info
*/
const TString TRunRequest::GetPUID() const {
    const auto* userInfoPtr = GetDataSource(NAlice::EDataSourceType::BLACK_BOX);
    if (userInfoPtr == nullptr || !userInfoPtr->HasUserInfo()) {
        LOG_ERROR(Debug().Logger()) << "Unable to retrive User ID via BlackBox data source. Check scenario datasources in megamind config";
        return "";
    }
    const NAlice::TBlackBoxUserInfo& userInfo = userInfoPtr->GetUserInfo();
    return userInfo.GetUid();
}


//
// Apply request
//
TApplyRequest::TApplyRequest(const NScenarios::TRequestMeta& requestMeta,
                             const NScenarios::TScenarioApplyRequest& applyRequest,
                             const NAppHost::IServiceContext& serviceCtx,
                             const TProtoHwScene* protoHwScene,
                             NAlice::NHollywood::IGlobalContext& globalCtx,
                             TRTLogger& logger,
                             const TApphostNodeInfo& apphostNodeInfo,
                             TMaybe<NHollywood::TCompiledNlgComponent> nlg)
    : TRequest(requestMeta,
               applyRequest.GetBaseRequest(),
               applyRequest.HasInput() ? applyRequest.GetInput() : EmptyInput,
               serviceCtx,
               protoHwScene,
               globalCtx,
               logger,
               apphostNodeInfo,
               nlg)
    , ApplyRequest_(applyRequest)
    , Arguments_(applyRequest.GetArguments())
{
    TProtoHwSceneCCAArguments args;
    if (Arguments_.Is<NHollywoodFw::TProtoHwSceneCCAArguments>() && Arguments_.UnpackTo(&args)) {
        Arguments_ = args.GetScenarioArgs();
    }
}

//
// Continue request
//
TContinueRequest::TContinueRequest(const NScenarios::TRequestMeta& requestMeta,
                                   const NScenarios::TScenarioApplyRequest& continueRequest,
                                   const NAppHost::IServiceContext& serviceCtx,
                                   const TProtoHwScene* protoHwScene,
                                   NAlice::NHollywood::IGlobalContext& globalCtx,
                                   TRTLogger& logger,
                                   const TApphostNodeInfo& apphostNodeInfo,
                                   TMaybe<NHollywood::TCompiledNlgComponent> nlg)
    : TRequest(requestMeta,
               continueRequest.GetBaseRequest(),
               continueRequest.HasInput() ? continueRequest.GetInput() : EmptyInput,
               serviceCtx,
               protoHwScene,
               globalCtx,
               logger,
               apphostNodeInfo,
               nlg)
    , ContinueRequest_(continueRequest)
    , Arguments_(continueRequest.GetArguments())
{
    TProtoHwSceneCCAArguments args;
    if (Arguments_.Is<NHollywoodFw::TProtoHwSceneCCAArguments>() && Arguments_.UnpackTo(&args)) {
        Arguments_ = args.GetScenarioArgs();
    }
}

//
// Commit request
//
TCommitRequest::TCommitRequest(const NScenarios::TRequestMeta& requestMeta,
                               const NScenarios::TScenarioApplyRequest& commitRequest,
                               const NAppHost::IServiceContext& serviceCtx,
                               const TProtoHwScene* protoHwScene,
                               NAlice::NHollywood::IGlobalContext& globalCtx,
                               TRTLogger& logger,
                               const TApphostNodeInfo& apphostNodeInfo,
                               TMaybe<NHollywood::TCompiledNlgComponent> nlg)
    : TRequest(requestMeta,
               commitRequest.GetBaseRequest(),
               commitRequest.HasInput() ? commitRequest.GetInput() : EmptyInput,
               serviceCtx,
               protoHwScene,
               globalCtx,
               logger,
               apphostNodeInfo,
               nlg)
    , CommitRequest_(commitRequest)
    , Arguments_(commitRequest.GetArguments())
{
    TProtoHwSceneCCAArguments args;
    if (Arguments_.Is<NHollywoodFw::TProtoHwSceneCCAArguments>() && Arguments_.UnpackTo(&args)) {
        Arguments_ = args.GetScenarioArgs();
    }
}

} // namespace NAlice::NHollywoodFw
