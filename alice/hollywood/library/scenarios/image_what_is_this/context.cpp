#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/resources/geobase.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer_library.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/context.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/image_what_is_this_resources.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/proto/image_what_is_this.pb.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/proto/image_what_is_this.pb.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/proto/render_request.pb.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/utils.h>
#include <alice/library/url_builder/url_builder.h>
#include <alice/megamind/protos/common/location.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <library/cpp/uri/uri.h>

using namespace NAlice::NHollywood::NImage;

namespace NAlice::NHollywood::NImage::NFlags {

const TString USE_TAG_NO_TEMPLATE = "image_recognizer_use_tag_no_";

}

namespace {

constexpr TStringBuf ALICE_REQUEST_ITEM = "image_what_is_this_request";
constexpr TStringBuf ALICE_RESPONSE_ITEM = "image_what_is_this_response";
const TString DETECTED_OBJECTS_ITEM = "detected_objects";
constexpr TStringBuf CBIR_FEATURES_REQUEST_ITEM = "cbir_features_request";
constexpr TStringBuf CBIR_FEATURES_RESPONSE_ITEM = "cbir_features_response";
constexpr TStringBuf DISK_CREATE_DIR_REQUEST_ITEM = "disk_create_dir_request";
constexpr TStringBuf DISK_CREATE_DIR_RESPONSE_ITEM = "disk_create_dir_response";
constexpr TStringBuf DISK_SAVEFILE_REQUEST_ITEM = "disk_save_file_request";
constexpr TStringBuf RENDER_REQUEST_ITEM = "render_request";
const TString FLAG_DISABLE_IMAGE_RECOGNIZER = "disable_image_recognizer";
constexpr int MAX_DETECTED_OBJECTS_REQUESTS = 5;

TVector<std::pair<float, float>> LoadCropCoordinates(const TString& coordinatesStr) {
    TVector<std::pair<float, float>> coordinates;
    TVector<TString> items;
    Split(coordinatesStr, ";", items);
    if (items.size() % 2  != 0) {
        return coordinates;
    }
    for (size_t i = 0; i < items.size(); i += 2) {
        coordinates.push_back({FromString<float>(items[i]), FromString<float>(items[i + 1])});
    }
    return coordinates;
}

template<typename TImageWhatIsThisContext>
void AttachUnsupportedOperationError(TImageWhatIsThisContext& context, bool /*isOnboarding*/) {
    if (context.GetRequest().Input().Utterance()) {
        // TODO:
        //context.AddSearchSuggest();
    }
    if (!context.GetRequest().ClientInfo().IsYaAuto()) {
        context.AddOnboardingSuggest();
    }

    context.TryAddShowPromoDirective(context.GetRequest().Interfaces());

    context.AddTextCard("render_inability", {});

    // TODO: Add Error block
    /*
    NSc::TValue unsupportedOperationError;
    unsupportedOperationError[TStringBuf("code")].SetString("unsupported_operation");
    if (isOnboarding) {
        context.AddErrorBlock(TError(TError::EType::ONBOARDINGERROR,
                                     TStringBuf("image_onboarding_error")),
                              std::move(unsupportedOperationError));
    } else {
        context.AddErrorBlock(TError(TError::EType::IMAGEERROR,
                                     TStringBuf("image_recognizer_error")),
                              std::move(unsupportedOperationError));
    }
    */
}

NAlice::NHollywood::NImage::TImageWhatIsThisState LoadState(const NAlice::NHollywood::TScenarioApplyRequestWrapper& request) {
    return request.UnpackArguments<NAlice::NHollywood::NImage::TImageWhatIsThisState>();
}

NAlice::NHollywood::NImage::TImageWhatIsThisState LoadState(const NAlice::NHollywood::TScenarioRunRequestWrapper& request) {
    return request.LoadState<NAlice::NHollywood::NImage::TImageWhatIsThisState>();
}

NJson::TJsonValue MakeAliceMode(const TStringBuf mode, bool isFrontal = false) {
    NJson::TJsonValue directive;
    if (isFrontal) {
        directive["camera_type"] = "front";
    } else {
        directive["capture_mode"] = mode;
    }
    return directive;
}

NJson::TJsonValue MakeSmartCamera(const TStringBuf mode) {
    NJson::TJsonValue directive;
    TCgiParameters wireframeCgi;
    wireframeCgi.InsertUnescaped("intent", "openImageSearchScreen");
    wireframeCgi.InsertUnescaped("modeId", mode);
    wireframeCgi.InsertUnescaped("entryPoint", "alice");

    const TString wireframeUrl = "wireframe://?" + wireframeCgi.Print();

    TCgiParameters openCgi;
    openCgi.InsertUnescaped("uri", wireframeUrl);

    const TString openUrl = "ya-search-app-open://?" + openCgi.Print();
    directive["uri"] = openUrl;
    return directive;
}

NJson::TJsonValue MakeSmartCameraBro(const TStringBuf mode) {
    NJson::TJsonValue directive;
    const TString openUrl = TString("smartcamera://?mode=") + mode;
    directive["uri"] = openUrl;
    return directive;
}

}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::TImageWhatIsThisContext(TScenarioHandleContext& ctx, EHandlerStage handlerStage)
    : Ctx(ctx)
    , RequestProto(GetOnlyProtoOrThrow<TRequest>(ctx.ServiceCtx, REQUEST_ITEM))
    , Request(RequestProto, ctx.ServiceCtx)
    , NlgWrapper(TNlgWrapper::Create(ctx.Ctx.Nlg(), Request, ctx.Rng, ctx.UserLang))
    , ResponseBuilder(&NlgWrapper)
    , NlgData{ctx.Ctx.Logger(), Request}
    , BodyBuilder(ResponseBuilder.CreateResponseBodyBuilder())
    , HandlerStage(handlerStage)
    , UserRegion(0)
{
    for (size_t i = 0; i != Request.Input().Proto().SemanticFramesSize(); ++i) {
        SemanticFrames.push_back(Request.Input().Proto().GetSemanticFrames(i).GetName());
    }

    const NAlice::NScenarios::TCallbackDirective* callback = GetCallback();
    if (callback) {
        TString semanticFrameCallback = callback->GetName();
        if (!semanticFrameCallback.empty()) {
            SemanticFrames.push_back(semanticFrameCallback);
        }
    }

    if (SemanticFrames.size() > 1) {
        LOG_INFO(Logger()) << "There are intersection of semantic frames";
        for(const TStringBuf semanticFrame : SemanticFrames) {
            LOG_INFO(Logger()) << semanticFrame;
        }
    }

    if (!Request.IsNewSession()) {
        State = LoadState(Request);
    }

    NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder = BodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetProductScenarioName("images_what_is_this");

    FillImageAliceHttpResponse();

    FillClothesHttpResponses();

    FillCbirFeaturesHttpResponse();

    FillDiskCreateDirHttpResponse();

    FillUsedTag();
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
bool TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::ShouldStopHandling() {
    if (!GetRequest().Interfaces().GetCanRecognizeImage()) {
        AttachUnsupportedOperationError(*this, /*isOnboarding*/ true);
        return true;
    }
    return false;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
THttpHeaders TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::MakeHeaders(bool addCookie) const {
    const NAlice::NHollywood::TConfig& config = Ctx.Ctx.GlobalContext().Config();
    THttpHeaders headers;
    if (config.HasCurrentDc()) {
        TMaybe<THttpInputHeader> balancingHint = GetBalancingHintHeader(config.GetCurrentDc());
        if (balancingHint.Defined()) {
            headers.AddHeader(*balancingHint);
        }
    }
    headers.AddHeader("User-Agent", Request.BaseRequestProto().GetOptions().GetUserAgent());
    if (addCookie) {
        TString buffer;
        if (google::protobuf::util::MessageToJsonString(Request.BaseRequestProto().GetOptions().GetMegamindCookies(), &buffer).ok()) {
            headers.AddHeader("Cookie", buffer);
        }
    }
    return headers;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddImageAliceRequest(const TVector<std::pair<TStringBuf, TStringBuf>>& params) {
    TCgiParameters cgi;
    cgi.InsertUnescaped("url", ImageUrl);
    for (std::pair<TStringBuf, TStringBuf> param : params) {
        cgi.InsertUnescaped(param.first, param.second);
    }

    FillCommonRequestParams(cgi);

    const TString url = "?" + cgi.Print();

    Ctx.ServiceCtx.AddProtobufItem(PrepareHttpRequest(url, Ctx.RequestMeta, Ctx.Ctx.Logger(), Default<TString>(), Nothing(), Nothing(), MakeHeaders(true)).Request, ALICE_REQUEST_ITEM);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddDetectedObjectRequest(const TStringBuf crop) {
    if (DetectedObjectRequestsCount >= MAX_DETECTED_OBJECTS_REQUESTS) {
        // TODO: Log this
        return;
    }

    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("avatarurl"), ImageUrl);
    cgi.InsertUnescaped(TStringBuf("crop"), crop);
    cgi.InsertUnescaped(TStringBuf("flag"), TStringBuf("images_alice_response=da"));
    FillCommonRequestParams(cgi);

    const TString url = "?" + cgi.Print();
    const TString detectedObjectsRequestItem = DETECTED_OBJECTS_ITEM + "_" + ToString(DetectedObjectRequestsCount) + "_" + "request";

    Ctx.ServiceCtx.AddProtobufItem(PrepareHttpRequest(url, Ctx.RequestMeta, Ctx.Ctx.Logger(), Default<TString>(), Nothing(), Nothing(), MakeHeaders(true)).Request, detectedObjectsRequestItem);

    ++DetectedObjectRequestsCount;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddCbirFeaturesRequest(const TStringBuf cbird,
                                                                                                  const TString& additionalParams) {
    TCgiParameters cgi;
    cgi.InsertUnescaped("url", ImageUrl);
    cgi.InsertUnescaped("cbird", cbird);
    cgi.InsertUnescaped("type", "json");

    TString url = "?" + cgi.Print();
    if (!additionalParams.Empty()) {
        url += "&" + additionalParams;
    }

    Ctx.ServiceCtx.AddProtobufItem(PrepareHttpRequest(url, Ctx.RequestMeta, Ctx.Ctx.Logger(), Default<TString>(), Nothing(), Nothing(), MakeHeaders(true)).Request, CBIR_FEATURES_REQUEST_ITEM);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddDiskCreateDirRequest(const TStringBuf path) {
    TCgiParameters cgi;
    cgi.InsertUnescaped("path", path);

    const TString url = "?" + cgi.Print();

    Ctx.ServiceCtx.AddProtobufItem(PrepareHttpRequest(url, Ctx.RequestMeta, Ctx.Ctx.Logger(), "disk_create_dir",
                                                      Nothing(), NAppHostHttp::THttpRequest_EMethod::THttpRequest_EMethod_Put, MakeHeaders(false)).Request,
                                   DISK_CREATE_DIR_REQUEST_ITEM);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddDiskSaveFileRequest(const TStringBuf path, const TStringBuf fileUrl) {
    TCgiParameters cgi;
    cgi.InsertUnescaped("path", path);
    cgi.InsertUnescaped("url", fileUrl);

    const TString url = "?" + cgi.Print();

    Ctx.ServiceCtx.AddProtobufItem(PrepareHttpRequest(url, Ctx.RequestMeta, Ctx.Ctx.Logger(), "disk_save_file",
                                                      Nothing(), NAppHostHttp::THttpRequest_EMethod::THttpRequest_EMethod_Post, MakeHeaders(false)).Request,
                                   DISK_SAVEFILE_REQUEST_ITEM);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddRenderRequest(const TRenderRequestProto& request) {
    Ctx.ServiceCtx.AddProtobufItem(request, RENDER_REQUEST_ITEM);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
bool TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::ExtractImageUrl() {
    if (Request.Input().Proto().HasImage()) {
        const auto& inputImagePB = Request.Input().Proto().GetImage();

        ImageUrl = inputImagePB.GetUrl();
        CaptureMode = inputImagePB.GetCaptureMode();//FromString<ECaptureMode>(inputImagePB.GetCaptureMode());

        CropCoordinates = LoadCropCoordinates(inputImagePB.GetCropCoordinates());
    } else if (!State.GetImageUrl().empty()) {
        ImageUrl = State.GetImageUrl();
        if (!CbirId.Defined()) {
            CbirId = State.GetCbirId();
        }
    } else {
        const NAlice::NScenarios::TCallbackDirective* callback = GetCallback();
        if (!callback) {
            return false;
        }

        const TMaybe<TString> imageUrlMaybe = ExtractPayloadField(callback, "image_url");
        if (!imageUrlMaybe.Defined()) {
            return false;
        }
        ImageUrl = imageUrlMaybe.GetRef();
    }


    NUri::TUri uri;
    NUri::TState::EParsed uriResult =
        uri.Parse(ImageUrl, NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeFlexible);

    if (NUri::TState::EParsed::ParsedOK != uriResult) {
        AddError("bad_img_url");
        StatIncCounter("hollywood_computer_vision_result_error_bad_img_url");
        return false;
    }
    if (TStringBuf("avatars.mds.yandex.net") != uri.GetHost()) {
        AddError("img_url_from_forbidden_host");
        StatIncCounter("hollywood_computer_vision_result_error_img_url_from_forbidden_host");
        return false;
    }

    return true;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::FillCommonRequestParams(TCgiParameters& cgi) {
    cgi.InsertUnescaped(TStringBuf("lr"), ToString(GetUserRegion()));
    const NAlice::TLocation& location = RequestProto.GetBaseRequest().GetLocation();
    cgi.InsertUnescaped(TStringBuf("ll"), TStringBuilder() << location.GetLon() << ','
                                                               << location.GetLat());
    cgi.InsertUnescaped(TStringBuf("service"), TStringBuf("assistant.yandex"));

    const NAlice::TClientInfo& clientInfo = GetClientInfo();
    cgi.InsertUnescaped(TStringBuf("ui"), clientInfo.GetSearchRequestUI());

    const NAlice::TClientInfoProto& clientInfoProto = RequestProto.GetBaseRequest().GetClientInfo();
    if (clientInfoProto.HasUuid()) {
        cgi.InsertUnescaped(TStringBuf("uuid"), clientInfoProto.GetUuid());
    }

    // TODO: YandexUID?
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::RenderPhotoRequest(const NAnswers::IAnswer* answer) {
    BodyBuilder.AddRenderedTextWithButtonsAndVoice("image_what_is_this", "render_photo_request", /* buttons */ {}, NlgData);
    const bool clientSupportSmartMode = SupportSmartMode();
    const bool clientIsAndroid = GetClientInfo().IsAndroid();
    const bool clientIsBro = GetClientInfo().IsYaBrowserMobile();

    const bool clientIosSupportSmartCamera = IosSupportSmartCamera();
    const TStringBuf answerAliceMode = answer->GetAliceMode();
    const TMaybe<TStringBuf> answerAliceSmartMode = answer->GetAliceSmartMode(*this);

    NJson::TJsonValue directiveValue;
    TString directiveName;
    if (!clientSupportSmartMode || !answerAliceSmartMode.Defined()) {
        const bool isFrontal = answer->GetIsFrontalCaptureMode();
        directiveValue = MakeAliceMode(answerAliceMode, isFrontal);
        directiveName = "start_image_recognizer";
        GetAnalyticsInfoBuilder().AddAction(CaptureModeToAnalyticsAction(answerAliceMode,
                                                                         /* isStartImageRecognizer */ true));

        const TStringBuf forceAnswer = answer->GetAnswerName();
        const bool isForceable = answer->GetIsForceable();
        if (forceAnswer && isForceable) {
            State.SetForceAnswer(ToString(forceAnswer));
        }
    } else {
        bool isStartImageRecognizer = true;
        if (clientIsAndroid) {
            directiveValue = MakeAliceMode(answerAliceSmartMode.GetRef());
            directiveName = "start_image_recognizer";
        } else {
            if (clientIsBro) {
                directiveValue = MakeSmartCameraBro(answerAliceSmartMode.GetRef());
                directiveName = "open_uri";
                isStartImageRecognizer = false;
            } else if (!clientIosSupportSmartCamera) {
                directiveValue = MakeAliceMode(answerAliceSmartMode.GetRef());
                directiveName = "start_image_recognizer";
            } else {
                directiveValue = MakeSmartCamera(answerAliceSmartMode.GetRef());
                directiveName = "open_uri";
                isStartImageRecognizer = false;
            }
        }
        GetAnalyticsInfoBuilder().SetIntentName(TString(answer->GetAnswerName()));
        GetAnalyticsInfoBuilder().AddAction(CaptureModeToAnalyticsAction(answerAliceSmartMode.GetRef(),
                                                                         isStartImageRecognizer));
    }

    LOG_INFO(Logger()) << directiveName << " " << directiveValue << Endl;

    BodyBuilder.AddClientActionDirective(directiveName, directiveValue);

    State.ClearCbirId();
    State.ClearImageUrl();
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::MakeResponse() {
    BodyBuilder.SetState(State);
    auto response = std::move(ResponseBuilder).BuildResponse();
    Ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddContinueRequest() {
    TRunResponseBuilder response(&NlgWrapper);
    response.SetContinueArguments(GetState());
    Ctx.ServiceCtx.AddProtobufItem(*std::move(response).BuildResponse(), RESPONSE_ITEM);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddError(TStringBuf errorCode) {
    NlgData.Context["error_code"] = errorCode;
    BodyBuilder.AddRenderedTextWithButtonsAndVoice("image_what_is_this_activate", "render_error", /* buttons = */ {}, NlgData);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AppendFeedbackOption(const TStringBuf feedbackType) {
    NegativeFeedbackOptions.push_back(feedbackType);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddSpecialSuggestAction(const TString& name, const TString& type,
                                                                                                  const TString& data) {
    NScenarios::TFrameAction action;
    NScenarios::TCallbackDirective* callback = action.MutableCallback();
    callback->SetName(name);
    auto& payloadFields = *callback->MutablePayload()->mutable_fields();

    ::google::protobuf::Value suggestTypePB;
    suggestTypePB.set_string_value(TString(type));
    payloadFields["type"] = suggestTypePB;

    if (!data.empty()) {
        ::google::protobuf::Value dataPB;
        dataPB.set_string_value(data);
        payloadFields["additional_data"] = dataPB;
    }

    const TString actionId = "suggest_" + type;
    BodyBuilder.AddAction(actionId, std::move(action));
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddSpecialSuggest(const TString& type) {
    const auto& suggestPhrase = NlgWrapper.RenderPhrase("image_what_is_this", "render_suggest_" + type,  TNlgData{Ctx.Ctx.Logger(), Request}).Text;

    const TString actionId = "suggest_" + type;
    BodyBuilder.AddActionSuggest(actionId).Title(suggestPhrase);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddRepeatButton(const TString& name, const TString& type) {
    TMap<TString, TString> payloadWithUrl;
    payloadWithUrl["image_url"] = ImageUrl;
    payloadWithUrl["repeat"] = "1";
    AddButton(name, type, payloadWithUrl);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddOpenUriButton(const TString& type, const TString& url) {
    NScenarios::TFrameAction action;
    NScenarios::TDirectiveList* directives = action.MutableDirectives();
    NScenarios::TOpenUriDirective* openUriDirective = directives->AddList()->MutableOpenUriDirective();
    openUriDirective->SetName(type);
    openUriDirective->SetUri(url);

    const auto& suggestPhrase = NlgWrapper.RenderPhrase("image_what_is_this", "render_button_" + type,  TNlgData{Ctx.Ctx.Logger(), Request}).Text;

    const TString actionId = "button_" + type;

    NScenarios::TLayout::TButton button;
    button.SetActionId(actionId);
    button.SetTitle(suggestPhrase);
    BodyBuilder.AddAction(actionId, std::move(action));
    Buttons.push_back(button);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddSearchSuggest(const TStringBuf title) {
    TSearchButtonBuilder buttonBuilder = BodyBuilder.AddSearchSuggest();
    buttonBuilder.Title(TString(title));
    buttonBuilder.Query(TString(title));
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddActionSuggest(const TString& title, const TString& actionId,
                                                                                            const NJson::TJsonValue& data) {
    TActionButtonBuilder buttonBuilder = BodyBuilder.AddActionSuggest(actionId);
    if (!data.IsNull()) {
        NlgData.Context["data"] = data;
    }
    NlgData.Context["data"]["switch_name"] = title;
    const auto& suggestPhrase = NlgWrapper.RenderPhrase("image_what_is_this", "render_switch_suggest", NlgData).Text;
    buttonBuilder.Title(suggestPhrase);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddOnboardingSuggest() {
    TString actionId = "onboarding_suggest_action";

    TString suggestUtterance = NlgWrapper.RenderPhrase("image_what_is_this", "render_suggest_utterance__onboarding__what_can_you_do", NlgData).Text;
    TString suggestCaption = NlgWrapper.RenderPhrase("image_what_is_this", "render_suggest_utterance__onboarding__what_can_you_do", NlgData).Text;

    NScenarios::TFrameAction action;

    NScenarios::TDirective directive;
    NScenarios::TTypeTextDirective* typeTextDirective = directive.MutableTypeTextDirective();
    typeTextDirective->SetText(suggestUtterance);
    typeTextDirective->SetName("type");
    *action.MutableDirectives()->AddList() = directive;

    BodyBuilder.AddAction(actionId, std::move(action));
    BodyBuilder.AddActionSuggest(actionId).Title(suggestCaption);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddButton(const TString& name, const TString& type,
                                                                                     const TMap<TString, TString>& payload) {
    NScenarios::TFrameAction action;
    NScenarios::TCallbackDirective* callback = action.MutableCallback();
    callback->SetName(name);
    auto& payloadFields = *callback->MutablePayload()->mutable_fields();

    ::google::protobuf::Value suggestTypePB;
    suggestTypePB.set_string_value(TString(type));
    payloadFields["type"] = suggestTypePB;

    for (const auto& payloadElem : payload) {
        ::google::protobuf::Value dataPB;
        dataPB.set_string_value(payloadElem.second);
        payloadFields[payloadElem.first] = dataPB;
    }

    const auto& suggestPhrase = NlgWrapper.RenderPhrase("image_what_is_this", "render_button_" + type,  TNlgData{Ctx.Ctx.Logger(), Request}).Text;

    const TString actionId = "button_" + type;

    NScenarios::TLayout::TButton button;
    button.SetActionId(actionId);
    button.SetTitle(suggestPhrase);
    BodyBuilder.AddAction(actionId, std::move(action));
    Buttons.push_back(button);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
TString TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddSwitchIntentAction(const TString& name) {
    TMap<TString, TString> payload;
    payload["image_url"] = ImageUrl;

    return AddAction(name, payload);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
TString TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddAction(const TString& name, const TMap<TString, TString>& payload) {
    NScenarios::TFrameAction action;
    NScenarios::TCallbackDirective* callback = action.MutableCallback();
    callback->SetName(name);
    auto& payloadFields = *callback->MutablePayload()->mutable_fields();

    for (const auto& payloadElem : payload) {
        ::google::protobuf::Value dataPB;
        dataPB.set_string_value(payloadElem.second);
        payloadFields[payloadElem.first] = dataPB;
    }

    const TString actionId = name + "_button_click";
    BodyBuilder.AddAction(actionId, std::move(action));

    return actionId;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::RenderFeedbackAnswer() {
    const NAlice::NScenarios::TCallbackDirective* callback = GetCallback();
    const auto& payloadFields = callback->GetPayload().fields();
    if (payloadFields.count("type")) {
        const TString type = payloadFields.at("type").string_value();
        NSc::TValue data;
        data["feedback"] = type;
        AddTextCard("render_feedback_answer", data);
        GetAnalyticsInfoBuilder().SetProductScenarioName("feedback");
        if (type == "feedback_positive_images") {
            GetAnalyticsInfoBuilder().SetIntentName("personal_assistant.feedback.feedback_positive");
        } else if (type == "feedback_negative_images") {
            GetAnalyticsInfoBuilder().SetIntentName("personal_assistant.feedback.feedback_negative");
        } else {
            GetAnalyticsInfoBuilder().SetIntentName("personal_assistant.feedback.feedback_negative_images");
        }
        if (type == "feedback_negative_images" && payloadFields.count("additional_data")) {
            const TString feedbacks = payloadFields.at("additional_data").string_value();
            TVector<TString> buttons;
            Split(feedbacks, ";", buttons);
            for (const TString& button : buttons) {
                AddSpecialSuggest(button);
                AddSpecialSuggestAction("alice.image_what_is_this_feedback", button);
            }
        }
    }
    AddOnboardingSuggest();
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const NAlice::NScenarios::TCallbackDirective* TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetCallback() const {
    return Request.Input().GetCallback();
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
bool TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::HasFlag(const TString& flag) const {
    return Request.Proto().GetBaseRequest().GetExperiments().fields().count(flag);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::SetLastAnswer(const TString& lastAnswer) {
    State.SetLastAnswer(lastAnswer);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::SetBestIntent(NAnswers::IAnswer* answer) {
    BestIntent = answer;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddDivCardBlock(TStringBuf cardName, const NSc::TValue& cardData) {
    NlgData.Context["data"] = cardData.ToJsonValue();
    BodyBuilder.AddRenderedDivCard(TStringBuf("image_what_is_this_cards"), cardName, NlgData);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddTextCard(TStringBuf cardName, const NSc::TValue& cardData) {
    NlgData.Context["data"] = cardData.ToJsonValue();
    BodyBuilder.AddRenderedTextWithButtonsAndVoice("image_what_is_this", cardName, /* buttons */ Buttons, NlgData);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::AddSuggest(TStringBuf name, bool autoAction, const NJson::TJsonValue& data) {
    auto suggest = TBassResponseRenderer::CreateSuggest(NlgWrapper, TStringBuf("search"), name,
                                                        /* analyticsTypeAction = */ "", autoAction,
                                                        data, NlgData);
    if (suggest.Defined()) {
        BodyBuilder.AddRenderedSuggest(std::move(suggest.GetRef()));
    }
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
TString TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GenerateSearchUri(TStringBuf uri, const TCgiParameters& cgiParams) {
    return NAlice::GenerateSearchUri(GetClientInfo(), CreateUserLocation(), GetRequest().ContentRestrictionLevel(),
                                     uri, GetRequest().Interfaces().GetCanOpenLinkSearchViewport(), cgiParams);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
TString TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GenerateImagesSearchUrl(const TStringBuf aliceSource, const TStringBuf report,
                                                         bool disablePtr, const TStringBuf cbirPage) const {
    return NAlice::GenerateSimilarImagesSearchUrl(GetClientInfo(), CbirId.GetRef(), aliceSource, cbirPage, report, /*needNextPage*/ false, disablePtr);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
TString TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GenerateMarketDealsLink(int cropId) const {
    return NAlice::GenerateSimilarImagesSearchUrl(GetClientInfo(), CbirId.GetRef(), /* aliceSource */ "market",
                                                  /* cbirPage */ "products", /* report */ "imageview",
                                                  /* needNextPage */ false, /* disablePtr */ false, cropId);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
NSc::TValue TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GenerateContact(const NSc::TValue& value, const EContactType type) {
    NSc::TValue cardContact;
    cardContact["value"] = value;
    cardContact["type"] = ToString(type);
    if (type == EContactType::CT_MAIL) {
        cardContact["uri"] = TString("mailto:") + value.GetString();
    } else if (type == EContactType::CT_PHONE) {
        cardContact["uri"] = GeneratePhoneUri(GetRequest().ClientInfo(), value);
    } else if (type == EContactType::CT_URL) {
        const TStringBuf url = value.GetString();
        cardContact["uri"] = (url.StartsWith("http://") || url.StartsWith("https://"))
                             ? url
                             : TString("http://") + url;
    } else {
        Y_FAIL("Unsupported contact type %d!", static_cast<int>(type));
    }

    NAlice::NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder = GetAnalyticsInfoBuilder();
    const TString analyticsContactType = ToString(type) + "_contact";
    analyticsInfoBuilder.AddObject(analyticsContactType, analyticsContactType, TString(value.GetString()));

    return cardContact;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::RedirectTo(const TStringBuf aliceSource, const TStringBuf report, bool disablePtr) {
    RedirectToCustomUri(GenerateImagesSearchUrl(aliceSource, report, disablePtr));
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::RedirectToCustomUri(const TStringBuf uri) {
    NJson::TJsonValue link;
    link["uri"] = uri;
    BodyBuilder.AddClientActionDirective("open_uri", std::move(link));
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::StatIncCounter(const TString& statValue) {
    const TString statName = "sensor";
    Ctx.Ctx.GlobalContext().Sensors().IncRate({{statName, statValue}});
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
bool TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::TryAddShowPromoDirective(NAlice::NScenarios::TInterfaces Interfaces) {
    if (Interfaces.GetSupportsShowPromo()) {
        BodyBuilder.AddShowPromoDirective();
        return true;
    }
    return false;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
TMaybe<NAnswers::IAnswer*>& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetBestIntent() {
    return BestIntent;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const TMaybe<TString>& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetCbirId() const {
    return CbirId;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const TMaybe<TString>& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetImageAliceReqid() const {
    return ImageAliceReqid;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const TString& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetImageUrl() const {
    return ImageUrl;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const NAlice::TClientInfo& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetClientInfo() const {
    return Request.ClientInfo();
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
NAlice::TUserLocation TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::CreateUserLocation() const {
    return GetUserLocation(Request);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
NGeobase::TId TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetUserRegion() const {
    if (UserRegion == 0) {
        UserRegion = ConstructUserRegion();
    }
    return UserRegion;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
NGeobase::TId TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::ConstructUserRegion() const {
    const auto& baseRequest = RequestProto.GetBaseRequest();
    const auto userRegion = baseRequest.GetOptions().GetUserDefinedRegionId();
    if (userRegion != 0) {
        LOG_INFO(Logger()) << "User-defined region " << userRegion << Endl;
        return userRegion;
    }

    NGeobase::TId regionId = CreateUserLocation().UserRegion();
    auto& geoLookup = Ctx.Ctx.GlobalContext().CommonResources().Resource<TGeobaseResource>().GeobaseLookup();
    if (regionId == NGeobase::UNKNOWN_REGION && baseRequest.HasLocation()) {
        const auto& location = baseRequest.GetLocation();
        regionId = geoLookup.GetRegionIdByLocation(location.GetLat(), location.GetLon());
    }
    if (regionId == NGeobase::UNKNOWN_REGION || regionId == -1) {
        LOG_WARNING(Logger()) << "Unknown user region";
        return regionId;
    }
    const int MaxIterCount = 10;
    for (size_t iter = 0; iter <= MaxIterCount; ++iter) {
        const NGeobase::TRegion region = geoLookup.GetRegionById(regionId);
        const NGeobase::ERegionType type = static_cast<NGeobase::ERegionType>(region.GetType());
        if (type == NGeobase::ERegionType::CITY || type == NGeobase::ERegionType::REGION || type == NGeobase::ERegionType::COUNTRY) {
            LOG_INFO(Logger()) << "User region " << regionId << ": " << region.GetName() << " (" << static_cast<int>(type) << ")";
            break;
        }
        Y_ENSURE(iter != MaxIterCount);
        regionId = region.GetParentId();
    }
    return regionId;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
NAlice::NScenarios::TUserPreferences::EFiltrationMode TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetFiltrationMode() const {
    return Request.Proto().GetBaseRequest().GetUserPreferences().GetFiltrationMode();
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const NAnswers::IAnswer* TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetForceAnswer() const {
    const NAnswers::IAnswer *answer = nullptr;
    const TStringBuf forceAnswerName = State.GetForceAnswer();
    if (forceAnswerName) {
        answer = NAnswers::TAnswerLibrary::GetAnswerByName(forceAnswerName);
    }
    if (!answer) {
        ECaptureMode captureMode = GetCaptureMode();
        answer = NAnswers::TAnswerLibrary::GetAnswerByCaptureMode(captureMode);
    }

    return answer;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
TMaybe<TString> TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::ExtractPayloadField(const NAlice::NScenarios::TCallbackDirective* callback,
                                                                                                          const TString& field) {
    if (!callback) {
        return Nothing();
    }
    const auto& payloadFields = callback->GetPayload().fields();
    if (!payloadFields.count(field)) {
        return Nothing();
    }
    return payloadFields.at(field).string_value();
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
ECaptureMode TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetCaptureMode() const {
    return CaptureMode;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const TImageWhatIsThisState& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetState() const {
    return State;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
TImageWhatIsThisState& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetState() {
    return State;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::SetForceAnswerState(const TString& forceAnswer) {
    State.SetForceAnswer(forceAnswer);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::SetImageUrlState(const TString& imageUrl) {
    State.SetImageUrl(imageUrl);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const TMaybe<TStringBuf> TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetSemanticFrame() const {
    if (SemanticFrames.empty()) {
        return Nothing();
    }

    if (SemanticFrames.at(0) == NAnswers::TAnswerLibrary::GetDefaultAnswer()->GetAnswerName()
            && SemanticFrames.size() > 1) {
        return SemanticFrames.at(1);
    }

    return SemanticFrames.at(0);
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const TMaybe<NSc::TValue>& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetImageAliceResponse() const {
    return ImageAliceResponse;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const TVector<NSc::TValue>& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetDetectedObjectsResponse() const {
    return DetectedObjectsResponses;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const TMaybe<NSc::TValue>& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetCbirFeaturesResponse() const {
    return CbirFeaturesResponse;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const TMaybe<int> TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetDiskCreateDirResponseStatusCode() const {
    return DiskCreateDirResponseStatusCode;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const NAlice::NHollywood::TScenarioHandleContext& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetScenarioHandleContext() const {
    return Ctx;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const TRequestWrapper& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetRequest() const {
    return Request;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const TVector<NImages::NCbir::ECbirIntents> TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetCbirIntents() const {
    return CbirIntents;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
size_t TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetUsedTagNo() const {
    return UsedTagNo;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const TString& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetUsedTagPath() const {
    return UsedTagPath;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
NAlice::NScenarios::IAnalyticsInfoBuilder& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::CreateAnalyticsInfoBuilder() {
    return BodyBuilder.CreateAnalyticsInfoBuilder();
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
NAlice::NScenarios::IAnalyticsInfoBuilder& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetAnalyticsInfoBuilder() {
    return BodyBuilder.GetAnalyticsInfoBuilder();
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const TVector<TStringBuf>& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetNegativeFeedbackOptions() const {
    return NegativeFeedbackOptions;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const TImageWhatIsThisResources& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetResources() const {
    return Ctx.Ctx.ScenarioResources<TImageWhatIsThisResources>();
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
const TCropCoordinates& TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetCropCoordinates() const {
    return CropCoordinates;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
EHandlerStage TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::GetHandlerStage() const {
    return HandlerStage;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
bool TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::SupportSmartMode() const {
    const TClientInfo& clientInfo = GetClientInfo();
    return (clientInfo.IsSearchApp() && (clientInfo.IsAndroidAppOfVersionOrNewer(21, 2, 2)
                                        || clientInfo.IsIOSAppOfVersionOrNewer(5600)))
           || (clientInfo.IsYaBrowser() && (clientInfo.IsAndroidAppOfVersionOrNewer(21, 6, 6)
                                        || clientInfo.IsIOSAppOfVersionOrNewer(2201, 6)));
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
bool TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::IosSupportSmartCamera() const {
    const TClientInfo& clientInfo = GetClientInfo();
    return (clientInfo.IsSearchApp() && clientInfo.IsIOSAppOfVersionOrNewer(8100))
               || (clientInfo.IsYaBrowser() && clientInfo.IsIOSAppOfVersionOrNewer(2201, 6));
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
bool TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::FillImageAliceHttpResponse() {
    TMaybe<NAppHostHttp::THttpResponse> detectedObjectsHttpResponseRaw = GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(Ctx.ServiceCtx, ALICE_RESPONSE_ITEM);
    if (detectedObjectsHttpResponseRaw.Defined()) {
        NAppHostHttp::THttpResponse detectedObjectsHttpResponse = detectedObjectsHttpResponseRaw.GetRef();

        if (detectedObjectsHttpResponse.GetStatusCode() != 200) {
            TStringBuilder errText;
            errText << TStringBuf("ComputerVision fetching error: ") << detectedObjectsHttpResponse.GetStatusCode();
            LOG_ERROR(Ctx.Ctx.Logger()) << errText << Endl;
            StatIncCounter("hollywood_computer_vision_result_error_computer_vision_fetching_error");
            ythrow yexception() << errText;
        }

        NSc::TValue detectedObjectsResponse;
        if (!NSc::TValue::FromJson(detectedObjectsResponse, detectedObjectsHttpResponse.GetContent())) {
            LOG_ERROR(Ctx.Ctx.Logger()) << TStringBuf("ComputerVision answer error: cannot parse JSON") << Endl;
            AddError("computer_vision_json_parse_error");
            StatIncCounter("hollywood_computer_vision_result_error_computer_vision_json_parse_error");
            return false;
        }

        ImageAliceResponse = std::move(detectedObjectsResponse);
        LogImageAliceHttpResponse();

        CbirId = ImageAliceResponse.GetRef()["CbirId"].GetString();
        ImageAliceReqid = ImageAliceResponse.GetRef()["ReqId"].GetString();

        for (const NSc::TValue& intent : ImageAliceResponse.GetRef()["Intents"].GetArray()) {
            const TStringBuf intentName = intent["name"].GetString();
            CbirIntents.emplace_back(FromString<NImages::NCbir::ECbirIntents>(intentName));
            if (CbirIntents.back() == NImages::NCbir::ECbirIntents::CI_OCR) {
                CbirIntents.emplace_back(NImages::NCbir::ECbirIntents::CI_OCR_VOICE);
            }
        }
    }
    return true;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::LogImageAliceHttpResponse() {
    if (!ImageAliceResponse.Defined()) {
        return;
    }
    const NSc::TValue& imageAliceResponse = ImageAliceResponse.GetRef();
    StatIncCounter("hollywood_computer_vision_alice_received");
    const TVector<TString> toChecks = {"Tags", "Market", "Similars", "ObjectResonses",
                                      "SimilarPeople", "SimilarArtwork"};
    for (const TString& toCheck : toChecks) {
        if (imageAliceResponse[toCheck].ArraySize()) {
            StatIncCounter("hollywood_computer_vision_alice_" + toCheck + "_received");
        }
    }

    if (imageAliceResponse["FastOcr"].Has("fulltext")) {
        StatIncCounter("hollywood_computer_vision_alice_fastocr_has_fulltext");
        if (imageAliceResponse["FastOcr"]["fulltext"].ArraySize()) {
            StatIncCounter("hollywood_computer_vision_alice_fastocr_fulltext_received");
        }
    }

    const NSc::TValue& factors = imageAliceResponse["IntentFactors"];
    if (factors.ArraySize() == 0) {
        StatIncCounter("hollywood_computer_vision_result_error_factors_empty");
        return;
    }
    TStringBuilder serializedFactors;

    for (const auto& factor : factors.GetArray()) {
        const TStringBuf name = factor["name"].GetString();
        const double value = factor["value"].GetNumber(-1.);
        if (name.empty() || value < 0.) {
            StatIncCounter("hollywood_computer_vision_result_error_factors_broken");
            LOG_DEBUG(Logger()) << TStringBuf("[CV FACTORS ERROR]") << factors << Endl;
            return;
        }
        serializedFactors << name << ':' << value << ';';
    }
    LOG_INFO(Logger()) << TStringBuf("[CV FACTORS]") << serializedFactors << Endl;
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::FillClothesHttpResponses() {
    for (int i = 0; i != MAX_DETECTED_OBJECTS_REQUESTS; ++i) {
        const TString detectedObjectsResponseItem = DETECTED_OBJECTS_ITEM + "_" + ToString(i) + "_" + "response";
        TMaybe<NAppHostHttp::THttpResponse> detectedObjectsHttpResponseRaw = GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(Ctx.ServiceCtx, detectedObjectsResponseItem);
        if (detectedObjectsHttpResponseRaw.Defined()) {
            NAppHostHttp::THttpResponse detectedObjectsHttpResponse = detectedObjectsHttpResponseRaw.GetRef();
            if (detectedObjectsHttpResponse.GetStatusCode() != 200) {
                StatIncCounter("hollywood_computer_vision_result_error_clothes_market_fetching_error");
                continue;
            }

            NSc::TValue detectedObjectsResponse;
            if (!NSc::TValue::FromJson(detectedObjectsResponse, detectedObjectsHttpResponse.GetContent())) {
                StatIncCounter("hollywood_computer_vision_result_error_clothes_market_json_parse_error");
                continue;
            }
            DetectedObjectsResponses.push_back(std::move(detectedObjectsResponse));
        } else {
            break;
        }
    }
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::FillCbirFeaturesHttpResponse() {
    TMaybe<NAppHostHttp::THttpResponse> cbirFeaturesHttpResponseRaw
                            = GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(Ctx.ServiceCtx, CBIR_FEATURES_RESPONSE_ITEM);
    if (cbirFeaturesHttpResponseRaw.Defined()) {
        NAppHostHttp::THttpResponse cbirFeaturesHttpResponse = cbirFeaturesHttpResponseRaw.GetRef();

        if (cbirFeaturesHttpResponse.GetStatusCode() != 200) {
            TStringBuilder errText;
            errText << TStringBuf("ComputerVision fetching error: ") << cbirFeaturesHttpResponse.GetStatusCode();
            LOG_ERROR(Ctx.Ctx.Logger()) << errText << Endl;
            StatIncCounter("hollywood_computer_vision_result_error_cbir_features_fetching_error");
            ythrow yexception() << errText;
        }

        NSc::TValue cbirFeaturesResponse;
        if (!NSc::TValue::FromJson(cbirFeaturesResponse, cbirFeaturesHttpResponse.GetContent())) {
            LOG_ERROR(Ctx.Ctx.Logger()) << TStringBuf("ComputerVision answer error: cannot parse JSON") << Endl;
            AddError("computer_vision_json_parse_error");
            StatIncCounter("hollywood_computer_vision_result_error_cbir_features_json_parse_error");
            return;
        }

        CbirFeaturesResponse = std::move(cbirFeaturesResponse);
    }
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::FillDiskCreateDirHttpResponse() {
    TMaybe<NAppHostHttp::THttpResponse> diskCreateDirHttpResponseRaw
                            = GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(Ctx.ServiceCtx, DISK_CREATE_DIR_RESPONSE_ITEM);
    if (diskCreateDirHttpResponseRaw.Defined()) {
        NAppHostHttp::THttpResponse diskCreateDirHttpResponse = diskCreateDirHttpResponseRaw.GetRef();

        DiskCreateDirResponseStatusCode = diskCreateDirHttpResponse.GetStatusCode();
    }
}

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
void TImageWhatIsThisContext<TRequest, TRequestWrapper, TResponseBuilder>::FillUsedTag() {
    for (int i = 1; i < 11; ++i) {
        const TString tagFlagWithNo = NFlags::USE_TAG_NO_TEMPLATE + ToString(i);
        const bool hasFlag = Request.Proto().GetBaseRequest().GetExperiments().fields().count(tagFlagWithNo);
        if (hasFlag) {
            UsedTagNo = i - 1;
            break;
        }
    }
    UsedTagPath = "/Tags[" + ToString(UsedTagNo) + "]";
}

namespace NAlice::NHollywood::NImage {
template class TImageWhatIsThisContext<NAlice::NScenarios::TScenarioRunRequest, TScenarioRunRequestWrapper, TRunResponseBuilder>;
template class TImageWhatIsThisContext<NAlice::NScenarios::TScenarioApplyRequest, TScenarioApplyRequestWrapper, TApplyResponseBuilder>;
}
