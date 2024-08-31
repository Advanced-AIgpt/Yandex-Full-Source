#include "draw_picture_impl.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/megamind/protos/common/events.pb.h>
#include <alice/protos/data/language/language.pb.h>

#include <alice/library/proto/proto.h>

#include <util/generic/variant.h>
#include <util/string/cast.h>
#include <util/random/shuffle.h>

#include <algorithm>


using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

    namespace {
        constexpr TStringBuf FRAME = "alice.draw_picture";

        constexpr TStringBuf NLG_TEMPLATE_NAME = "draw_picture";
        constexpr TStringBuf DIVCARD_NAME = "render_result";
        constexpr TStringBuf PHRASE_NAME = "standard_response";

        constexpr TStringBuf ACTION_PREFIX = "action_";
    }

    TDrawPictureImpl::TDrawPictureImpl(TScenarioHandleContext &ctx)
        : Ctx(ctx)
        , RequestProto(GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM))
        , Request(RequestProto, ctx.ServiceCtx)
        , Frame(Request.Input().CreateRequestFrame(FRAME))
        , Data(ctx.Ctx.ScenarioResources<TDrawPictureResources>())
    {}

    void TDrawPictureImpl::RejectDrawPicture(const TStringBuf& text, bool isIrrelevant) {
        TNlgWrapper nlgWrapper = TNlgWrapper::Create(Ctx.Ctx.Nlg(), Request, Ctx.Rng, Ctx.UserLang);
        TRunResponseBuilder builder(&nlgWrapper);
        auto& bodyBuilder = builder.CreateResponseBodyBuilder(&Frame);

        TNlgData nlgData{Ctx.Ctx.Logger(), Request};
        nlgData.Context["tts"] = text;
        nlgData.Context["text"] = text;
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_TEMPLATE_NAME, PHRASE_NAME, /* buttons= */ {}, nlgData);

        if (isIrrelevant) {
            builder.SetIrrelevant();
        }

        Ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
    }

    void TDrawPictureImpl::AddSuggest(
            TResponseBodyBuilder& bodyBuilder,
            const TString& actionId,
            const TString& phrase,
            const TString& title,
            bool addHints)
    {
        TFrameAction action;

        TDirective directive;
        TTypeTextDirective* typeTextDirective = directive.MutableTypeTextDirective();
        typeTextDirective->SetText(phrase);
        *action.MutableDirectives()->AddList() = directive;

        TFrameNluHint nluHint;
        nluHint.SetFrameName(actionId);
        if (addHints) {
            for (const auto& hint : Data.GetNluHints()) {
                NAlice::TNluPhrase nluPhrase;
                nluPhrase.SetLanguage(NAlice::ELang::L_RUS);
                nluPhrase.SetPhrase(hint);
                (*nluHint.AddNegatives()) = nluPhrase;
            }
        }
        *action.MutableNluHint() = std::move(nluHint);

        bodyBuilder.AddAction(actionId, std::move(action));
        bodyBuilder.AddActionSuggest(actionId).Title(title);
    }

    void TDrawPictureImpl::RenderDrawPicture(const TResponse& response) {
        TNlgWrapper nlgWrapper = TNlgWrapper::Create(Ctx.Ctx.Nlg(), Request, Ctx.Rng, Ctx.UserLang);
        TRunResponseBuilder builder(&nlgWrapper);
        auto& bodyBuilder = builder.CreateResponseBodyBuilder(&Frame);

        TNlgData nlgData{Ctx.Ctx.Logger(), Request};
        nlgData.Context["original"] = response.GetUrl() + "/orig";
        nlgData.Context["preview"] = response.GetUrl() + "/w512p";
        nlgData.Context["tts"] = response.GetComment().GetTts();
        nlgData.Context["text"] = response.GetComment().Text;
        nlgData.Context["render_share"] = !Request.ClientInfo().IsYaBrowserDesktop();
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_TEMPLATE_NAME, PHRASE_NAME, /* buttons= */ {}, nlgData);
        bodyBuilder.AddRenderedDivCard(NLG_TEMPLATE_NAME, DIVCARD_NAME, nlgData);

        TString userInput;
        if (Request.Input().IsTextInput()) {
            userInput = Request.Input().Proto().GetText().GetRawUtterance();
        } else {
            userInput = Request.Input().Proto().GetVoice().GetAsrData(0).GetUtterance();
        }

        TStringStream actionName;
        actionName << ACTION_PREFIX << "more";
        AddSuggest(bodyBuilder, actionName.Str(), userInput, "покажи ещё", true);

        const auto& suggests = Data.GetSuggests();
        TVector<int> idx(suggests.size());
        std::iota(idx.begin(), idx.end(), 0);
        Shuffle(idx.begin(), idx.end(), Ctx.Rng);
        idx.resize(SuggestsNum);
        for (const auto i : idx) {
            actionName.Clear();
            actionName << ACTION_PREFIX << i;
            AddSuggest(bodyBuilder, actionName.Str(), suggests[i], suggests[i], false);
        }

        Ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
    }

    TDrawPictureImpl::TResponse TDrawPictureImpl::GetRankedRandomImage(const NMilab::NI2t::TI2tVector &features) {
        const auto ranking = NMilab::NI2t::Rank(Data.GetFeatures(), features, RankingSize);
        return TResponse{
            Data.GetUrls()[RandomChoice(ranking.Container()).Index],
            RandomChoice(Data.GetCommentBuckets()[Data.GetGenericCommentBucketId()]),
        };
    }

    TMaybe<TDrawPictureImpl::TResponse> TDrawPictureImpl::LookupSubstitutes(const TString& request) {
        const auto& reqSub = Data.GetRequestToSubstituteId();
        if (auto iter = reqSub.find(request); iter != reqSub.end()) {
            const auto& subst = Data.GetSubstitutes()[iter->second];
            return TResponse{
                Data.GetUrls()[RandomChoice(subst.ImageIds)],
                RandomChoice(Data.GetCommentBuckets()[subst.CommentBucketId]),
            };
        }
        return Nothing();
    }

    TDrawPictureImpl::TResponse TDrawPictureImpl::GetRandomImage() {
        return TResponse{
            RandomChoice(Data.GetUrls()),
            RandomChoice(Data.GetCommentBuckets()[Data.GetGenericCommentBucketId()]),
        };
    }

} // namespace NAlice::NHollywood
