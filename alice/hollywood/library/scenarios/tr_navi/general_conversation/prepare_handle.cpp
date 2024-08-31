#include "prepare_handle.h"

#include "common.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/scenarios/begemot.pb.h>

namespace NAlice::NHollywood::NTrNavi {

namespace {

using TDialogTurn = NScenarios::TDialogHistoryDataSource::TDialogTurn;

constexpr size_t CONTEXT_LENGTH = 3;
constexpr TStringBuf EXP_GC_NOT_BANNED = "gc_not_banned";
constexpr TStringBuf GC_REQUEST_BANLIST = "gc_request_banlist";

TVector<TDialogTurn> GetDialogTurns(const TScenarioRunRequestWrapper& requestWrapper) {
    const auto* dialogHistoryDataSource = requestWrapper.GetDataSource(EDataSourceType::DIALOG_HISTORY);
    Y_ENSURE(dialogHistoryDataSource, "DialogHistoryDataSource not found in the run request");

    return {dialogHistoryDataSource->GetDialogHistory().GetDialogTurns().begin(), dialogHistoryDataSource->GetDialogHistory().GetDialogTurns().end()};
}

TString ConstructContext(const TVector<TDialogTurn>& dialogTurns, TStringBuf utterance) {
    TStringBuilder context;
    const auto contextStartIndex = dialogTurns.size() >= CONTEXT_LENGTH ? dialogTurns.size() - CONTEXT_LENGTH + 1 : 0;
    for (auto dialogTurnsIter = dialogTurns.begin() + contextStartIndex; dialogTurnsIter != dialogTurns.end(); ++dialogTurnsIter) {
        context << dialogTurnsIter->GetRequest() << '\n';
    }
    context << utterance;

    return context;
}

bool IsRequestBannedForGC(const TScenarioRunRequestWrapper& requestWrapper, TStringBuf& banAnswer) {
    const auto* begemotFixlistResultDataSource = requestWrapper.GetDataSource(EDataSourceType::BEGEMOT_FIXLIST_RESULT);
    Y_ENSURE(begemotFixlistResultDataSource, "BegemotFixlistResultDataSource not found in the run request");

    const auto matches = begemotFixlistResultDataSource->GetBegemotFixlistResult().GetMatches();
    for (const auto& match : matches) {
        if (match.GetKey() == GC_REQUEST_BANLIST) {
            const auto intents = match.GetValue().GetIntents();
            if (intents.empty()) {
                return false;
            }
            banAnswer = intents[0];
        }
    }

    return true;
}

TFrame PrepareGeneralConversationFrame(const TString& context) {
    TFrame frame{TString{GENERAL_CONVERSATION_FRAME}};
    frame.AddSlot(TSlot{"context", "string", TSlot::TValue{context}});

    return frame;
}

} // namespace

void TGeneralConversationTrPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder responseBuilder(&nlgWrapper);
    auto& logger = ctx.Ctx.Logger();

    const auto& utterance = request.Input().Utterance();
    if (!utterance) {
        LOG_WARNING(logger) << "Empty utterance";

        responseBuilder.SetIrrelevant();
        responseBuilder.CreateResponseBodyBuilder();

        const auto response = *std::move(responseBuilder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
        return;
    }

    if (!request.HasExpFlag(EXP_GC_NOT_BANNED)) {
        // Reply is ready (beg_your_pardon)
        PrepareResponseWithPhrase(responseBuilder.CreateResponseBodyBuilder(), logger, request, BEG_YOUR_PARDON);

        const auto response = *std::move(responseBuilder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
        return;
    }

    TStringBuf banReply = BEG_YOUR_PARDON;
    if (IsRequestBannedForGC(request, banReply)) {
        // Reply is ready (banReply)
        PrepareResponseWithPhrase(responseBuilder.CreateResponseBodyBuilder(), logger, request, banReply);

        const auto response = *std::move(responseBuilder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
        return;
    }

    // Otherwise, go for reply
    const auto dialogTurns = GetDialogTurns(request);
    const auto context = ConstructContext(dialogTurns, utterance);
    const auto frame = PrepareGeneralConversationFrame(context);

    const auto bassRequest = PrepareBassVinsRequest(
        ctx.Ctx.Logger(),
        request,
        frame,
        /* textFrame= */ nullptr,
        ctx.RequestMeta,
        /* imageSearch= */ false,
        ctx.AppHostParams
    );
    AddBassRequestItems(ctx, bassRequest);
}

}  // namespace NAlice::NHollywood::NTrNavi
