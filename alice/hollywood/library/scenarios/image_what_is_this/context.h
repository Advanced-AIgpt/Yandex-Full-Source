#pragma once

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/hollywood/library/scenarios/image_what_is_this/proto/image_what_is_this.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <alice/library/geo/user_location.h>

#include <web/src_setup/lib/setup/images_cbir_postprocess/intents_classifier/intents/intents.h>

#include <library/cpp/json/json_reader.h>

TMaybe<TString> GetSlotValueString(const NAlice::TSemanticFrame& frame, const TStringBuf slotName);

namespace NAlice::NHollywood::NImage {

typedef TVector<std::pair<float, float>> TCropCoordinates;

class TRenderRequestProto;

namespace NAnswers {
    class IAnswer;
}

using ECaptureMode = NScenarios::TInput::TImage::ECaptureMode;

enum class EContactType {
    CT_MAIL     = 0 /* "email" */,
    CT_PHONE    = 1 /* "phone" */,
    CT_URL      = 2 /* "url" */,
};

enum class EHandlerStage {
    RUN,
    CONTINUE,
    INT,
    RENDER,
};

namespace NComputerVisionFeedbackOptions {

constexpr TStringBuf TAG_WRONG = "feedback_negative_images__tag_wrong";
constexpr TStringBuf USELESS = "feedback_negative_images__useless";
constexpr TStringBuf OFFENSIVE_ANSWER = "feedback_negative__offensive_answer";
constexpr TStringBuf OTHER = "feedback_negative__other";

} // NComputerVisionFeedbackOptions

class TImageWhatIsThisState;
class TImageWhatIsThisResources;

template <typename TRequest, typename TRequestWrapper, typename TResponseBuilder>
class TImageWhatIsThisContext {
public:
    TImageWhatIsThisContext(TScenarioHandleContext& ctx, EHandlerStage handlerStage);

    bool ShouldStopHandling();

    void AddImageAliceRequest(const TVector<std::pair<TStringBuf, TStringBuf>>& params = {});
    bool ExtractImageUrl();
    void AddDetectedObjectRequest(const TStringBuf crop);
    void AddCbirFeaturesRequest(const TStringBuf cbird, const TString& additionalParams = TString());
    void AddDiskCreateDirRequest(const TStringBuf path);
    void AddDiskSaveFileRequest(const TStringBuf path, const TStringBuf fileUrl);
    void AddRenderRequest(const TRenderRequestProto& request);
    void FillCommonRequestParams(TCgiParameters& cgi);

    void RenderPhotoRequest(const NAnswers::IAnswer* answer);//, const TScenarioRunRequestWrapper& request);
    void AddError(const TStringBuf errorCode);
    void MakeResponse();
    void AddContinueRequest();
    void AppendFeedbackOption(const TStringBuf feedbackType);
    void AddSpecialSuggest(const TString& type);
    void AddSpecialSuggestAction(const TString& name, const TString& type, const TString& data = TString());
    void AddRepeatButton(const TString& name, const TString& type);
    void AddButton(const TString& name, const TString& type, const TMap<TString, TString>& payload);
    TString AddSwitchIntentAction(const TString& name);
    TString AddAction(const TString& name, const TMap<TString, TString>& payload = TMap<TString, TString>());
    void AddOpenUriButton(const TString& type, const TString& url);
    void AddSearchSuggest(const TStringBuf title);
    void AddActionSuggest(const TString& title, const TString& actionId, const NJson::TJsonValue& data = NJson::TJsonValue(NJson::JSON_NULL));
    void AddOnboardingSuggest();
    void RenderFeedbackAnswer();
    const NAlice::NScenarios::TCallbackDirective* GetCallback() const;

    bool HasFlag(const TString& flag) const;

    void SetLastAnswer(const TString& lastAnswer);
    void SetBestIntent(NAnswers::IAnswer* bestIntent);

    void AddDivCardBlock(TStringBuf cardName, const NSc::TValue& cardData);
    void AddTextCard(TStringBuf cardName, const NSc::TValue& cardData);
    void AddSuggest(TStringBuf name, bool autoAction = false, const NJson::TJsonValue& data = NJson::TJsonValue(NJson::JSON_NULL));

    TString GenerateSearchUri(TStringBuf uri, const TCgiParameters& cgiParams = TCgiParameters());
    TString GenerateImagesSearchUrl(const TStringBuf aliceSource, const TStringBuf report = "imageview",
                                    bool disablePtr = false, const TStringBuf cbirPage = "") const;
    TString GenerateMarketDealsLink(int cropId) const;
    NSc::TValue GenerateContact(const NSc::TValue& value, const EContactType type);
    void RedirectTo(const TStringBuf aliceSource, const TStringBuf report, bool disablePtr = false);
    void RedirectToCustomUri(const TStringBuf uri);

    void StatIncCounter(const TString& statValue);
    bool TryAddShowPromoDirective(NAlice::NScenarios::TInterfaces Interfaces);
    TMaybe<NAnswers::IAnswer*>& GetBestIntent();
    const TMaybe<TString>& GetCbirId() const;
    const TString& GetImageUrl() const;
    const NAlice::TClientInfo& GetClientInfo() const;
    NAlice::TUserLocation CreateUserLocation() const;
    NGeobase::TId GetUserRegion() const;
    NAlice::NScenarios::TUserPreferences::EFiltrationMode GetFiltrationMode() const;
    const NAnswers::IAnswer* GetForceAnswer() const;
    TMaybe<TString> ExtractPayloadField(const NAlice::NScenarios::TCallbackDirective* callback, const TString& field);
    ECaptureMode GetCaptureMode() const;
    const TImageWhatIsThisState& GetState() const;
    TImageWhatIsThisState& GetState();
    void SetForceAnswerState(const TString& forceAnswer);
    void SetImageUrlState(const TString& imageUrl);
    const TMaybe<TStringBuf> GetSemanticFrame() const;
    const TMaybe<NSc::TValue>& GetImageAliceResponse() const;
    const TVector<NSc::TValue>& GetDetectedObjectsResponse() const;
    const TMaybe<NSc::TValue>& GetCbirFeaturesResponse() const;
    const TMaybe<int> GetDiskCreateDirResponseStatusCode() const;
    const TScenarioHandleContext& GetScenarioHandleContext() const;
    const TRequestWrapper& GetRequest() const;
    const TVector<NImages::NCbir::ECbirIntents> GetCbirIntents() const;
    const TMaybe<TString>& GetImageAliceReqid() const;
    size_t GetUsedTagNo() const;
    const TString& GetUsedTagPath() const;
    TScenarioHandleContext& GetContext() {
        return Ctx;
    }
    TRTLogger& Logger() const {
        return Ctx.Ctx.Logger();
    }
    NAlice::NScenarios::IAnalyticsInfoBuilder& CreateAnalyticsInfoBuilder();
    NAlice::NScenarios::IAnalyticsInfoBuilder& GetAnalyticsInfoBuilder();
    const TVector<TStringBuf>& GetNegativeFeedbackOptions() const;
    const TImageWhatIsThisResources& GetResources() const;
    const TCropCoordinates& GetCropCoordinates() const;
    EHandlerStage GetHandlerStage() const;
    bool SupportSmartMode() const;
    bool IosSupportSmartCamera() const;

private:
    bool FillImageAliceHttpResponse();
    void LogImageAliceHttpResponse();
    void FillClothesHttpResponses();
    void FillCbirFeaturesHttpResponse();
    void FillUsedTag();
    void FillDiskCreateDirHttpResponse();
    THttpHeaders MakeHeaders(bool addCookie) const;

private:
    TScenarioHandleContext& Ctx;
    TVector<TStringBuf> SemanticFrames;
    TImageWhatIsThisState State;

    TRequest RequestProto;
    TRequestWrapper Request;
    TNlgWrapper NlgWrapper;
    TResponseBuilder ResponseBuilder;
    TNlgData NlgData;
    //TFrame Frame;
    TResponseBodyBuilder& BodyBuilder;

    TString ImageUrl;
    TCropCoordinates CropCoordinates;
    ECaptureMode CaptureMode = ECaptureMode::TInput_TImage_ECaptureMode_Photo;

    TMaybe<NSc::TValue> ImageAliceResponse;
    TMaybe<TString> CbirId;
    TMaybe<NAnswers::IAnswer*> BestIntent;
    TVector<NImages::NCbir::ECbirIntents> CbirIntents;
    TMaybe<TString> ImageAliceReqid;

    TVector<NSc::TValue> DetectedObjectsResponses;
    TMaybe<NSc::TValue> CbirFeaturesResponse;
    TMaybe<int> DiskCreateDirResponseStatusCode;
    TMaybe<NSc::TValue> DiskSaveFileResponse;

    TVector<TStringBuf> NegativeFeedbackOptions;
    TVector<NAlice::NScenarios::TLayout::TButton> Buttons;

    size_t UsedTagNo = 0;
    TString UsedTagPath;

    int DetectedObjectRequestsCount = 0;

    EHandlerStage HandlerStage = EHandlerStage::RUN;

    mutable NGeobase::TId UserRegion;

    NGeobase::TId ConstructUserRegion() const;
};

using TImageWhatIsThisRunContext = TImageWhatIsThisContext<NAlice::NScenarios::TScenarioRunRequest, TScenarioRunRequestWrapper, TRunResponseBuilder>;
using TImageWhatIsThisApplyContext = TImageWhatIsThisContext<NAlice::NScenarios::TScenarioApplyRequest, TScenarioApplyRequestWrapper, TApplyResponseBuilder>;

}
