#include "render_nlg.h"
#include "render_bass_block_context.h"
#include "render_suggest.h"

#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/vins/api/vins_api/speechkit/connectors/protocol/protos/state.pb.h>

#include <alice/protos/data/contextual_data.pb.h>
#include <alice/protos/data/language/language.pb.h>

#include <alice/library/json/json.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/stream/zlib.h>


namespace NAlice::NHollywoodFw {

namespace {

TVector<NJson::TJsonValue> MapBassBlocksToJson(const ::google::protobuf::RepeatedPtrField<::google::protobuf::Struct>& bassBlocks) {
    auto result = TVector<NJson::TJsonValue>(Reserve(bassBlocks.size()));
    for (const auto& bassBlock : bassBlocks) {
        result.push_back(::NAlice::JsonFromProto(bassBlock));
    }
    return result;
}

TStringBuf GetBassBlockType(const NJson::TJsonValue& bassBlock) {
    if (bassBlock.Has("error")) {
        return "error";
    }
    return bassBlock["type"].GetStringSafe();
}

NJson::TJsonValue BuildNlgContext(const NJson::TJsonValue& vinsForm, const TVector<NJson::TJsonValue>& bassBlocks) {
    auto context = NJson::TJsonValue(NJson::EJsonValueType::JSON_MAP);

    context["form"] = NJson::TJsonValue(NJson::EJsonValueType::JSON_MAP);
    for (const auto& slot : vinsForm["slots"].GetArray()) {
        context["form"].InsertValue(slot["slot"].GetStringSafe(), slot["value"]);
    }

    for (const auto& bassBlock : bassBlocks) {
        if (GetBassBlockType(bassBlock) == "attention") {
            const auto& attentionType = bassBlock["attention_type"].GetStringSafe();
            context["attentions"][attentionType] = bassBlock;
        }
    }

    return context;
}

TGenericErrorVoidOr<NJson::TJsonValue> TryParseVinsForm(const NScenarios::TScenarioResponseBody& responseBody) {
    if (!responseBody.HasState()) {
        return TGenericErrorVoid() << "Vins state does not exist";
    }

    NAlice::NProtoVins::TState vinsState;
    if (!responseBody.GetState().UnpackTo(&vinsState)) {
        return TGenericErrorVoid() << "Failed to unpack vins state";
    }

    if (vinsState.GetSession().empty()) {
        return TGenericErrorVoid() << "Vins session is empty";
    }

    const auto& sessionCompressedEncoded = vinsState.GetSession();
    const auto sessionCompressed = Base64Decode(sessionCompressedEncoded);
    auto sessionCompressedStream = TStringInput(sessionCompressed);
    const auto sessionJsonStr = TZLibDecompress(&sessionCompressedStream).ReadAll();
    auto sessionJson = NJson::ReadJsonFastTree(sessionJsonStr, true);

    auto& form = sessionJson["objects"]["form"]["value"];
    if (!form.IsMap()) {
        return TGenericErrorVoid() << "Vins form expected to be a map, but actually it is " << form.GetType();
    }

    return std::move(form);
}

NNlg::TRenderPhraseResult RenderPhrase(const TStringBuf phraseId, TRenderBassBlockContext ctx) {
    if (ctx.Request.Nlg().HasPhrase(ctx.NlgTemplate, phraseId)) {
        return ctx.Request.Nlg().RenderPhrase(ctx.NlgTemplate, phraseId, ctx.NlgContext);
    }
    static constexpr TStringBuf CommonNlgTemplateName = "common";
    if (ctx.Request.Nlg().HasPhrase(CommonNlgTemplateName, phraseId)) {
        return ctx.Request.Nlg().RenderPhrase(CommonNlgTemplateName, phraseId, ctx.NlgContext);
    }
    ythrow yexception() << "Cannot find nlg phrase to render: template_id = '" << ctx.NlgTemplate << "', phrase_id = '" << phraseId << "'";
}

void RenderAndSayPhrase(const TStringBuf phraseId, TRenderBassBlockContext ctx) {
    const auto phrase = RenderPhrase(phraseId, ctx);
    if (phrase.Text) {
        ctx.ResponseBody.MutableLayout()->AddCards()->SetText(phrase.Text);
    }
    if (phrase.Voice) {
        auto& outputSpeech = *ctx.ResponseBody.MutableLayout()->MutableOutputSpeech();
        if (outputSpeech) {
            outputSpeech.append('\n');
        }
        outputSpeech.append(phrase.Voice);
    }
}

void RenderErrorBlock(const NJson::TJsonValue& bassBlock, TRenderBassBlockContext ctx) {
    const TString phraseId = TStringBuilder() << "render_error__" << bassBlock["error"]["type"].GetStringSafe();
    LOG_DEBUG(ctx.Request.Debug().Logger()) << "Rendering error block with phrase " << phraseId;
    RenderAndSayPhrase(phraseId, ctx);
}

void RenderTextCard(const NJson::TJsonValue& bassBlock, TRenderBassBlockContext ctx) {
    const auto phraseId = bassBlock["phrase_id"].GetStringSafe();
    LOG_DEBUG(ctx.Request.Debug().Logger()) << "Rendering text_card block with phrase " << phraseId;
    auto nlgContext = ctx.NlgContext;
    nlgContext["data"] = bassBlock["data"];
    RenderAndSayPhrase(phraseId, TRenderBassBlockContext{ctx.Request, ctx.NlgTemplate, nlgContext, ctx.ResponseBody});
}

void RenderIgnoredBlock(const NJson::TJsonValue& bassBlock, TRenderBassBlockContext ctx) {
    LOG_DEBUG(ctx.Request.Debug().Logger()) << "Skipped rendering bass block of type '" << bassBlock["type"].GetStringSafe() << "'";
}

void RenderUnknownBlock(const NJson::TJsonValue& bassBlock, TRenderBassBlockContext ctx) {
    LOG_WARN(ctx.Request.Debug().Logger()) << "Skipped rendering bass block of unknown type: " << bassBlock;
}

void RenderBassBlocks(const TVector<NJson::TJsonValue>& bassBlocks, TRenderBassBlockContext ctx) {
    using TRenderBassBlock = void(*)(const NJson::TJsonValue&, TRenderBassBlockContext ctx);
    static const THashMap<TStringBuf, TRenderBassBlock> blockRenderers = {
        {"error", RenderErrorBlock},
        {"text_card", RenderTextCard},
        {"suggest", RenderSuggest},
        {"attention", RenderIgnoredBlock},
    };

    for (const auto& bassBlock : bassBlocks) {
        auto renderer = blockRenderers.Value(GetBassBlockType(bassBlock), RenderUnknownBlock);
        renderer(bassBlock, ctx);
    }
}

} // namespace

void RenderNlg(
    const NHollywoodFw::TRequest& request,
    const TStringBuf nlgTemplateName,
    const NProtoVins::TNlgRenderData& vinsNlgRenderData,
    NScenarios::TScenarioResponseBody& responseBody)
{
    auto& logger = request.Debug().Logger();

    NJson::TJsonValue vinsForm;
    if (const auto error = TryParseVinsForm(responseBody).MoveTo(vinsForm)) {
        LOG_WARN(logger) << "Vins form not found: " << error->Message();
        return;
    }

    LOG_DEBUG(logger) << "Vins form: " << vinsForm;

    const auto bassBlocks = MapBassBlocksToJson(vinsNlgRenderData.GetBassBlocks());
    const auto nlgContext = BuildNlgContext(vinsForm, bassBlocks);

    RenderBassBlocks(bassBlocks, TRenderBassBlockContext{request, nlgTemplateName, nlgContext, responseBody});

    // TODO(d-dima): add nlg render history

    responseBody.MutableContextualData()->SetResponseLanguage(static_cast<::NAlice::ELang>(request.Input().GetUserLanguage()));

    LOG_INFO(logger) << "Successfully rendered " << bassBlocks.size() << " bass blocks";
}

} // namespace NAlice::NHollywood
