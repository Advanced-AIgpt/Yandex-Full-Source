#include "greetings_scene.h"

#include "greetings_consts.h"
#include "memento.h"
#include "skillrec_request_helper.h"
#include "onboarding.h"

#include <alice/hollywood/library/scenarios/onboarding/proto/onboarding.pb.h>

#include <alice/hollywood/library/framework/core/codegen/gen_directives.pb.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/library/network/headers.h>
#include <alice/library/onboarding/enums.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>
#include <alice/megamind/protos/scenarios/layout.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/api/renderer/api.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/data/scenario/onboarding/greetings.pb.h>
#include <alice/protos/div/div2card.pb.h>

#include <dj/services/alisa_skills/server/proto/client/onboarding_response.pb.h>
#include <dj/services/alisa_skills/server/proto/client/proactivity_request.pb.h>
#include <dj/services/alisa_skills/server/proto/client/request.pb.h>
#include <dj/services/alisa_skills/server/proto/client/response.pb.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/quote/quote.h>

#include <util/generic/guid.h>

using TServiceOrOnboardingResponse = std::variant<NDJ::NAS::TServiceResponse, NDJ::NAS::TOnboardingResponse>;

namespace NAlice::NHollywoodFw::NOnboarding {

    namespace {

        constexpr float DEFAULT_SCREEN_SCALE_FACTOR = 2;
        constexpr std::array<float, 6> ALLOWED_SCREEN_SCALE_FACTORS = { 1, 1.5, 2, 3, 3.5, 4 };

        class TActionsBuilder {
        public:
            explicit TActionsBuilder(NScenarios::TScenarioResponseBody& data)
                : Actions_(*data.MutableFrameActions())
            {
            }

            void AddActions(TString actionId, TString activation, TString itemNumber, TString actionName, TString requestId) {
                // client_actions
                NScenarios::TFrameAction action;
                if (activation == "SmartCam") {
                    action.MutableDirectives()->AddList()->MutableStartImageRecognizerDirective();
                } else if (activation == "AliceChat") {
                    auto* directive = action.MutableDirectives()->AddList()->MutableOpenUriDirective();
                    directive->SetUri("dialog://?full_alice=true");
                } else if (activation == "SmartHome") {
                    auto* directive = action.MutableDirectives()->AddList()->MutableOpenUriDirective();
                    directive->SetUri("opensettings://?screen=quasar");
                } else {
                    auto* directive = action.MutableDirectives()->AddList()->MutableTypeTextDirective();
                    directive->SetText(activation);
                }
                // server_actions
                if (auto* callbackDirective = action.MutableDirectives()->AddList()->MutableCallbackDirective()) {
                    callbackDirective->SetIgnoreAnswer(true);
                    callbackDirective->SetName("on_card_action");
                    const auto payload = NJson::TJsonMap({
                        {"request_id", requestId},
                        {"card_id", "skill_recommendation"},
                        {"case_name", "skill_recommendation__get_greetings__editorial#__" + actionName},
                        {"intent_name", "personal_assistant.scenarios.skill_recommendation"},
                        {"item_number", itemNumber},
                    });
                    const auto status = NAlice::JsonToProto(payload, *callbackDirective->MutablePayload());
                    Y_ENSURE(status.ok(), "Failed to convert provided Json to Proto: " << status.ToString());
                }

                Actions_.insert({actionId, action});
            }

        private:
            ::google::protobuf::Map<TProtoStringType, NScenarios::TFrameAction>& Actions_;
        };

        class TButtonsBuilder {
        public:
            explicit TButtonsBuilder(NScenarios::TScenarioResponseBody& data)
                : Directive_(*data.MutableLayout()->AddDirectives()->MutableShowButtonsDirective())
            {
                Directive_.SetScreenId(SCREEN_ID);
            }

            void AddSkill(const TSkill& skill) {
                auto& button = *Directive_.AddButtons();
                button.SetTitle(skill.GetTitle());
                button.SetActionId(skill.GetActionId());
                if (const auto& imageUrl = skill.GetImageUrl(); !imageUrl.empty()) {
                    button.MutableTheme()->SetImageUrl(imageUrl);
                }
            }

        private:
            NScenarios::TShowButtonsDirective& Directive_;
        };

        bool isDivLayerAvailable(const TVector<std::shared_ptr<NRenderer::TRenderResponse>>& divRenderResponse, const TString layerName) {
            for (auto resp : divRenderResponse) {
                if (resp->GetCardId() == layerName) {
                    return true;
                }
            }
            return false;
        }

        bool GetSource(const TSource& src, TStringBuf key, TServiceOrOnboardingResponse& out) {
            const auto rawResponse = src.GetRawHttpContent(key, /* throwOnFailure */ false);
            if (!rawResponse) {
                return false;
            }
            if (auto* oldResponse = std::get_if<NDJ::NAS::TServiceResponse>(&out)) {
                NProtobufJson::TJson2ProtoConfig cfg;
                cfg.FieldNameMode = NProtobufJson::TJson2ProtoConfig::FieldNameSnakeCase;
                try {
                    NProtobufJson::Json2Proto(*rawResponse, *oldResponse, cfg);
                    return true;
                } catch (...) {}
                return false;
            }
            if (auto* updResponse = std::get_if<NDJ::NAS::TOnboardingResponse>(&out)) {
                if (updResponse->ParseFromString(*rawResponse)) {
                    return true;
                }
            }
            return false;
        }

        bool IsEmpty(const TServiceOrOnboardingResponse& in) {
            return std::visit([](const auto& resp) { return resp.ItemsSize() == 0; }, in);
        }

        TString ToTitleUTF8(const TString str) {
            return ToUpperUTF8(SubstrUTF8(str, 0, 1)) + SubstrUTF8(str, 1, str.length() - 1);
        }

        float NormalizeScreenScaleFactor(float ssfValue) {
            /*Taken from recommender service*/
            const float desiredScale = ssfValue - std::numeric_limits<float>::epsilon();
            if (ssfValue < 0) {
                return DEFAULT_SCREEN_SCALE_FACTOR;
            }
            for (float s : ALLOWED_SCREEN_SCALE_FACTORS) {
                if (s >= desiredScale) {
                    return s;
                }
            }
            return ALLOWED_SCREEN_SCALE_FACTORS.back();
        }

        void FillSkills(const NDJ::NAS::TServiceResponse& in, TGreetingsRenderProto& out, const bool imageRequired, const float scaleFactor) {
            std::size_t skillId = 0;
            for (const auto& item : in.GetItems()) {
                if (auto* skill = out.AddSkills()) {
                    skill->SetTitle(ToTitleUTF8(item.GetActivation())); // can start with lowercase
                    skill->SetActivation(item.GetActivation());
                    skill->SetDivActionId("div_action_" + ToString(skillId));
                    skill->SetActionId("action_" + ToString(skillId));
                    skill->SetActionName(item.GetId());
                    if (imageRequired) {
                        skill->SetImageUrl(TStringBuilder() << "https://avatars.mds.yandex.net/get-dialogs/" << item.GetLogoAvatarId() << "/mobile-logo-x" << scaleFactor);
                    }
                    ++skillId;
                }
            }
        }

        void FillSkills(const NDJ::NAS::TOnboardingResponse& in, TGreetingsRenderProto& out, const bool imageRequired, const float /* scaleFactor */) {
            std::size_t skillId = 0;
            for (const auto& item : in.GetItems()) {
                if (auto* skill = out.AddSkills()) {
                    skill->SetTitle(item.GetResult().GetTitle());
                    skill->SetActivation(item.GetResult().GetTitle());
                    skill->SetDivActionId("div_action_" + ToString(skillId));
                    skill->SetActionId("action_" + ToString(skillId));
                    skill->SetActionName(item.GetId());
                    if (imageRequired) {
                        skill->SetImageUrl(item.GetResult().GetLogo().GetImageUrl());
                    }
                    ++skillId;
                }
            }
        }

        void FillSkills(const TServiceOrOnboardingResponse& in, TGreetingsRenderProto& out, const bool imageRequired, const bool isSearchApp, const float scaleFactor) {
            std::visit([&out, imageRequired, scaleFactor](const auto& resp) { FillSkills(resp, out, imageRequired, scaleFactor); }, in);
            // add skills stub
            const std::size_t startId = out.SkillsSize();
            out.MergeFrom(GetPromoSkillsStub(startId, isSearchApp));
        }

    } // namespace

    TGreetingsScene::TGreetingsScene(const TScenario* owner)
        : TScene(owner, GREETINGS_SCENE_NAME)
    {
        RegisterRenderer(&TGreetingsScene::Render);
    }

    TRetSetup TGreetingsScene::MainSetup(const TGreetingsSceneArgs&, const TRunRequest& request, const TStorage& storage) const {
        const bool useUpdatedBackend = request.Flags().IsExperimentEnabled(NAlice::NHollywood::EXP_HW_ONBOARDING_GREETINGS_USE_UPDATED_BACKEND);
        const auto& meta = request.GetRequestMeta();
        std::unique_ptr<ISkillRecRequest> greetingsRequest;
        if (useUpdatedBackend) {
            greetingsRequest = std::make_unique<TGreetingsRequestNew>(request, storage);
        } else {
            greetingsRequest = std::make_unique<TGreetingsRequestOld>(request);
        }
        NHollywood::THttpProxyRequestBuilder proxyRequest(greetingsRequest->GetPath(), meta, request.Debug().Logger(), TString{GREETINGS_SCENE_NAME});
        proxyRequest.SetMethod(NAppHostHttp::THttpRequest::Post).SetBody(greetingsRequest->GetBody(), greetingsRequest->GetContentType());
        TSetup setup(request);
        setup.Attach(proxyRequest.Build(), SKILLREC_REQUEST_KEY);
        return setup;
    }

    TRetMain TGreetingsScene::Main(const TGreetingsSceneArgs&, const TRunRequest& request, TStorage& storage, const TSource& src) const {
        // Create greetings response classes (may be TServiceResponse or TOnboardingResponse)
        const bool useUpdatedBackend = request.Flags().IsExperimentEnabled(NAlice::NHollywood::EXP_HW_ONBOARDING_GREETINGS_USE_UPDATED_BACKEND);
        TServiceOrOnboardingResponse response = NDJ::NAS::TServiceResponse();
        if (useUpdatedBackend) {
            response = NDJ::NAS::TOnboardingResponse();
        }
        // Get greetings response
        if (!GetSource(src, SKILLREC_RESPONSE_KEY, response)) {
            LOG_INFO(request.Debug().Logger()) << "Using fallback response";
            if (auto* oldResponse = std::get_if<NDJ::NAS::TServiceResponse>(&response)) {
                oldResponse->MergeFrom(GetOldResponseFallbackValues());
            } else if (auto* newResponse = std::get_if<NDJ::NAS::TOnboardingResponse>(&response)) {
                newResponse->MergeFrom(GetNewResponseFallbackValues());
            }
        }
        if (IsEmpty(response)) {
            TError err(TError::EErrorDefinition::SubsystemError);
            err.Details() << "No skills found";
            return err;
        }
        // Fill skills
        TGreetingsRenderProto renderProto;
        const bool imageRequired = !request.Flags().IsExperimentEnabled(NAlice::NHollywood::EXP_HW_ONBOARDING_DISABLE_GREETINGS_IMAGES); // image_url are omitted for cloud_ui_2
        const bool isSearchApp = request.Client().GetClientInfo().IsSearchApp();
        const float scaleFactor = NormalizeScreenScaleFactor(request.GetRunRequest().GetBaseRequest().GetOptions().GetScreenScaleFactor());
        FillSkills(response, renderProto, imageRequired, isSearchApp, scaleFactor);
        // Create render
        auto ret = TReturnValueRender(&TGreetingsScene::Render, renderProto);

        // Prepare data for div render if appropriate feature is supported
        if (request.Client().GetInterfaces().GetSupportsShowViewLayerContent()) {
            NRenderer::TDivRenderData contentRenderData;
            contentRenderData.SetCardId(GREETINGS_DIV_LAYER_CONTENT);
            NData::TGreetingsCardData contentCardData;

            for (const auto& skill : renderProto.GetSkills()) {
                if (auto* button = contentCardData.AddButtons()) {
                    button->SetActionId(skill.GetDivActionId());
                    button->SetTitle(skill.GetTitle());
                }
            }

            NData::TScenarioData scenarioContentData;
            *scenarioContentData.MutableGreetingsCardData() = contentCardData;
            *contentRenderData.MutableScenarioData() = scenarioContentData;
            ret.AddDivRender(std::move(contentRenderData));
        }

        if (request.Client().GetInterfaces().GetSupportsShowViewLayerFooter()) {
            NRenderer::TDivRenderData footerRenderData;
            footerRenderData.SetCardId(GREETINGS_DIV_LAYER_FOOTER);
            NData::TGreetingsFooterCardData footerCardData;

            for (const auto& promoSkill : renderProto.GetPromoSkills()) {
                if (auto* tile = footerCardData.AddTiles()) {
                    tile->SetTitle(promoSkill.GetTitle());
                    tile->SetActionId(promoSkill.GetDivActionId());
                    tile->SetImageUrl(promoSkill.GetImageUrl());
                }
            }

            NData::TScenarioData scenarioFooterData;
            *scenarioFooterData.MutableGreetingsFooterCardData() = footerCardData;
            *footerRenderData.MutableScenarioData() = scenarioFooterData;
            ret.AddDivRender(std::move(footerRenderData));
        }

        if (const auto onboardingResponsePtr = std::get_if<NDJ::NAS::TOnboardingResponse>(&response)) {
            UpdateTagStats(request, storage, onboardingResponsePtr->GetItems());
            AddLastViews(request, storage, onboardingResponsePtr->GetItems(), onboardingResponsePtr->GetItemType());
        }

        return ret;
    }

    TRetResponse TGreetingsScene::Render(const TGreetingsRenderProto& args, TRender& render) {
        auto& responseBody = render.GetResponseBody();
        auto& divRenderResponse = render.GetDivRenderResponse();

        TActionsBuilder actionsBuilder(responseBody);
        const TString requestId = render.GetRequest().GetRequestMeta().GetRequestId();
        std::size_t item = 1;
        // Actions for content layer
        if (isDivLayerAvailable(divRenderResponse, GREETINGS_DIV_LAYER_CONTENT)) {
            for (const auto& skill : args.GetSkills()) {
                actionsBuilder.AddActions(skill.GetDivActionId(), skill.GetActivation(), ToString(item), skill.GetActionName(), requestId);
                item++;
            }
        } else {
            LOG_INFO(render.GetRequest().Debug().Logger()) << "Content layer is not available, fallback to ShowButtonsDirective";
            TButtonsBuilder buttonsBuilder(responseBody);
            for (const auto& skill : args.GetSkills()) {
                buttonsBuilder.AddSkill(skill);
                actionsBuilder.AddActions(skill.GetActionId(), skill.GetActivation(), ToString(item), skill.GetActionName(), requestId);
                item++;
            }
        }
        // Actions for footer layer
        if (isDivLayerAvailable(divRenderResponse, GREETINGS_DIV_LAYER_FOOTER)) {
            for (const auto& promoSkill : args.GetPromoSkills()) {
                actionsBuilder.AddActions(promoSkill.GetDivActionId(), promoSkill.GetActivation(), ToString(item), "hardcoded_skill", requestId);
                item++;
            }
        }

        // Add ShowView directives with divcard body
        for (const auto& response : divRenderResponse) {
            ::NAlice::TDiv2Card div2Card;
            *div2Card.MutableBody() = *response->MutableDiv2Body();
            ::NAlice::NScenarios::TShowViewDirective showView;
            *showView.MutableDiv2Card() = div2Card;
            showView.SetLayerName(response->GetCardId());
            showView.SetScreenId(SCREEN_ID);
            render.Directives().AddShowViewDirective(std::move(showView));
        }

        responseBody.MutableLayout()->SetShouldListen(true);

        return NHollywoodFw::TReturnValueSuccess();
    }

}
