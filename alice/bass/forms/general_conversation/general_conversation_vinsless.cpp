#include "general_conversation_vinsless.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/str_stl.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/join.h>

namespace NBASS {

namespace {

constexpr TStringBuf GENERAL_CONVERSATION_VINSLESS_FORM_NAME = "alice.vinsless.general_conversation";

struct TReplyCandidate {
    TString Reply = "";
    double Relevance = 0;
    TString Source = "";
};

TVector<TReplyCandidate> ExtractReplies(const NSc::TValue& docs) {
    TVector<TReplyCandidate> replies;
    for (const NSc::TValue& grouping : docs["Grouping"].GetArray()) {
        for (const NSc::TValue& group : grouping["Group"].GetArray()) {
            for (const NSc::TValue& doc : group["Document"].GetArray()) {
                const auto relevance = doc["Relevance"].ForceNumber();
                if (doc["ArchiveInfo"]["GtaRelatedAttribute"].IsNull()) {
                    replies.push_back({"", relevance, ""});
                    continue;
                }
                TString reply;
                TString source;
                for (const NSc::TValue& attr : doc["ArchiveInfo"]["GtaRelatedAttribute"].GetArray()) {
                    if (attr["Key"] == "reply")
                        reply = attr["Value"];
                    if (attr["Key"] == "source")
                        source = attr["Value"];
                }
                if (reply) {
                    replies.push_back({std::move(reply), relevance, std::move(source)});
                }
            }
        }
    }

    return replies;
}


TString ConstructRelevParams(const TContext& ctx) {
    const auto& config = ctx.GetConfig().Vins().GeneralConversationTurkish();
    TStringBuilder relevParams;
    relevParams << "MaxResults=" << config.MaxResults() << ';';
    relevParams << "MinRatioWithBestResponse=" << config.MinRatioWithBestResponse() << ';';
    relevParams << "SearchFor=" << config.SearchFor() << ';';
    relevParams << "SearchBy=" << config.SearchBy() << ';';
    relevParams << "ContextWeight=" << config.ContextWeight() << ';';
    relevParams << "DssmModelName=" << config.DssmModelName() << ';';
    relevParams << "RankerModelName=" << config.RankerModelName() << ';';

    return relevParams;
}

NHttpFetcher::TRequestPtr PrepareGCRequest(TContext& ctx, const TStringBuf context) {
    auto request = ctx.GetSources().GeneralConversationTurkish().Request();
    request->AddCgiParam("relev", ConstructRelevParams(ctx));
    request->AddCgiParam("text", context);

    return request;
}

void FilterViaWizard(TContext& ctx, TVector<TReplyCandidate>& candidates) {
    TCgiParameters params;
    TStringBuilder texts;
    texts << "fixlist_type=gc_response_banlist";
    for (const auto& candidate : candidates) {
        texts << ";text=" << candidate.Reply;
    }
    params.InsertEscaped(TStringBuf("wizextra"), texts);
    params.InsertEscaped(TStringBuf("tld"), ctx.MetaLocale().Lang);
    params.InsertEscaped(TStringBuf("rwr"), TStringBuf("AliceFixlistFilter"));

    const auto& wizardResponse = ctx.ReqWizard(TStringBuf(""), ctx.UserRegion(), params, TStringBuf("megamind"));
    if (wizardResponse.IsNull()) {
        Y_STATS_INC_COUNTER_IF(!ctx.IsTestUser(), "general_conversation_wizard_error");
        candidates.clear();
        return;
    }

    const auto& passedNode = wizardResponse["rules"]["AliceFixlistFilter"]["Passed"];
    THashSet<TStringBuf> passedSet;
    if (passedNode.IsArray()) {
        const auto& passedArray = passedNode.GetArray();
        passedSet.reserve(passedArray.size());
        for (const auto& passed : passedArray) {
            passedSet.insert(passed.GetString());
        }
    } else if (passedNode.IsString()) {
        passedSet.insert(passedNode.GetString());
    }

    EraseIf(candidates, [&passedSet] (const auto& candidate) { return !passedSet.contains(candidate.Reply); });
}

} // namespace

TResultValue TGeneralConversationVinslessFormHandler::Do(TRequestHandler& r) {
    auto& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::GENERAL_CONVERSATION);
    if (ctx.MetaLocale().Lang != "tr") {
        LOG(ERR) << "Not supported language for GC: " << ctx.MetaLocale().Lang << Endl;
        return TError(TError::EType::GENERAL_CONVERSATION_ERROR, TStringBuf("language"));

    }
    auto* contextSlot = ctx.GetSlot("context", "string");
    if (IsSlotEmpty(contextSlot) || contextSlot->Value.GetString().empty()) {
        LOG(ERR) << "Empty context for GC" << Endl;

        r.Ctx().AddErrorBlock(TError::EType::GENERAL_CONVERSATION_ERROR, TStringBuf("empty_context"));
        return TResultValue();
    }

    const auto& context = contextSlot->Value.GetString();
    const auto& request = PrepareGCRequest(ctx, context);
    auto resp = request->Fetch()->Wait();
    if (resp->IsError()) {
        LOG(ERR) << "Fetching from GC service error: " << resp->GetErrorText() << Endl;
        Y_STATS_INC_COUNTER_IF(!ctx.IsTestUser(), "general_conversation_service_error");

        r.Ctx().AddErrorBlock(TError::EType::GENERAL_CONVERSATION_ERROR, TStringBuf("service_error"));
        return TResultValue();
    }

    auto replies = ExtractReplies(NSc::TValue::FromJson(resp->Data));
    FilterViaWizard(ctx, replies);

    if (replies.empty()) {
        LOG(ERR) << "No replies fonund for context: " << contextSlot->Value.GetString() << Endl;
        Y_STATS_INC_COUNTER_IF(!ctx.IsTestUser(), "general_conversation_no_replies");

        r.Ctx().AddErrorBlock(TError::EType::GENERAL_CONVERSATION_ERROR, TStringBuf("no_replies"));
        return TResultValue();
    }

    const auto idx = ctx.GetRng().RandomInteger(replies.size());
    const auto reply = replies.at(idx);

    r.Ctx().CreateSlot("reply", "string", true, TStringBuf(reply.Reply), TStringBuf(reply.Reply));
    return TResultValue();
}

void TGeneralConversationVinslessFormHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TGeneralConversationVinslessFormHandler>();
    };

    handlers->emplace(GENERAL_CONVERSATION_VINSLESS_FORM_NAME, handler);
}

}
