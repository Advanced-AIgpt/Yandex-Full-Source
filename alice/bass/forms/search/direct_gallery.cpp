#include "direct_gallery.h"
#include "direct_continuation.h"
#include "serp.h"

#include <alice/bass/forms/urls_builder.h>

#include <alice/library/experiments/flags.h>
#include <alice/library/websearch/direct_gallery.h>

#include <library/cpp/html/dehtml/dehtml.h>
#include <library/cpp/iterator/concatenate.h>

#include <util/generic/hash_set.h>
#include <util/string/cast.h>
#include <util/string/split.h>

using namespace NAlice::NDirectGallery;
using namespace NHttpFetcher;

namespace NBASS::NDirectGallery {

namespace {

constexpr TStringBuf SLOT_NAME_ITEMS = "direct_items";
constexpr TStringBuf SLOT_TYPE_ITEMS = "direct_items";

constexpr TStringBuf FORM_NAME_DIRECT_GALLERY = "personal_assistant.scenarios.direct_gallery";

constexpr TStringBuf FLAG_DO_NOT_ADD_BUTTON_LINK = "direct_do_not_add_button_link";
constexpr size_t NOT_A_MEDICINE_DISCLAIMER_LEN = 23;
constexpr size_t HAS_CONTRADICTIONS_DISCLAIMER_LEN = 46;
constexpr TStringBuf NOT_A_MEDICINE_DISCLAIMER_SIZE = "10";
constexpr TStringBuf HAS_CONTRADICTIONS_DISCLAIMER_SIZE = "5";

} // namespace

TString ReplaceHtmlEntitiesInText(const TString& htmlText) {
    THtmlStripper stripper(HtmlStripMode::HSM_ENTITY | HtmlStripMode::HSM_SPACE, ECharset::CODES_UTF8);
    return stripper(htmlText);
}

bool DisclaimerHasCorrectSize(const TStringBuf disclaimer, const TStringBuf requiredSize) {
    const TUtf32String disclaimerUtf32 = TUtf32String::FromUtf8(disclaimer);
    return !(
        (requiredSize == NOT_A_MEDICINE_DISCLAIMER_SIZE && disclaimerUtf32.size() > NOT_A_MEDICINE_DISCLAIMER_LEN) ||
        (requiredSize == HAS_CONTRADICTIONS_DISCLAIMER_SIZE &&
         disclaimerUtf32.size() > HAS_CONTRADICTIONS_DISCLAIMER_LEN));
}

size_t DirectItemsCount(const NSc::TValue& searchResult) {
    const auto& docsHalfpremium = searchResult.TrySelect("banner/data/direct_halfpremium").GetArray();
    const auto& docsPremium = searchResult.TrySelect("banner/data/direct_premium").GetArray();

    return docsPremium.size() + docsHalfpremium.size();
}

TDirectGalleryBuilder::TDirectGalleryBuilder(TContext& ctx)
    : Ctx(&ctx) {
}

TMaybe<TDirectGalleryBuilder> TDirectGalleryBuilder::Create(TContext& ctx) {
    if (!CanShowDirectGallery(ctx.ClientFeatures())) {
        return Nothing();
    }

    return TDirectGalleryBuilder(ctx);
}

TMaybe<NSc::TValue> TDirectGalleryBuilder::BuildDirectGalleryItem(const NSc::TValue& document) {
    if (Ctx->HasExpFlag(NAlice::NExperiments::WEBSEARCH_DISABLE_DIRECT_GALLERY)) {
        return Nothing();
    }

    NSc::TValue result;
    result["title"] = ReplaceHtmlEntitiesInText(NSerpSnippets::RemoveHiLight(document.TrySelect("title")));
    result["text"] = ReplaceHtmlEntitiesInText(NSerpSnippets::RemoveHiLight(document.TrySelect("body")));
    result["green_url"] = NSerpSnippets::RemoveHiLight(document.TrySelect("domain"));
    result["domain_name"] = document.TrySelect("fav_domain");
    result["url"] = document.TrySelect("url");
    result["disclaimer"] = ReplaceHtmlEntitiesInText(ToString(document.TrySelect("warning").GetString()));

    if (!result["disclaimer"].GetString().empty() &&
        DisclaimerHasCorrectSize(result["disclaimer"].GetString(), document.TrySelect("warning_size").GetString())) {
        result["required_size"] = document.TrySelect("warning_size").GetString();
    }

    return result;
}

TContext::TPtr TDirectGalleryBuilder::Build(const TStringBuf& query, const NSc::TValue& searchResult) {
    const auto& bannerData = searchResult["banner"]["data"];
    const auto& docsHalfpremium = bannerData["direct_halfpremium"].GetArray();
    const auto& docsPremium = bannerData["direct_premium"].GetArray();

    TString hitCounter;
    TString linkHead;
    TVector<TString> linkTails;
    const bool shouldConfirmHit = !Ctx->HasExpFlag(NAlice::NExperiments::WEBSEARCH_DISABLE_CONFIRM_DIRECT_HIT);
    if (shouldConfirmHit) {
        const auto& stat = bannerData["stat"][0];
        hitCounter = stat["hit_counter"].GetString();
        linkHead = stat["link_head"].GetString();
    }

    THashSet<TStringBuf> bidSet;
    NSc::TValue items;
    items.SetArray();
    bool hasDisclaimer = false;
    bool hasLargeDisclaimer = false;

    for (const auto& doc : Concatenate(docsHalfpremium, docsPremium)) {
        TStringBuf docBid = doc["bid"].GetString();
        if (bidSet.contains(docBid)) {
            continue;
        }
        TMaybe<NSc::TValue> item = BuildDirectGalleryItem(doc);
        if (item) {
            items.Push(*item);
            if (!item->TrySelect("disclaimer").GetString().empty()) {
                hasDisclaimer = true;
                const TStringBuf size = (*item)["required_size"].GetString();
                if (size.empty() || size == HAS_CONTRADICTIONS_DISCLAIMER_SIZE) {
                    hasLargeDisclaimer = true;
                }
            }
            if (shouldConfirmHit) {
                TStringBuf linkTail = doc["link_tail"].GetString();
                if (!linkTail.empty()) {
                    linkTails.push_back(TString{linkTail});
                }
            }
            bidSet.insert(docBid);
        }
    }

    if (items.GetArray().size() != 2) {
        return TContext::TPtr{};
    }

    TContext::TPtr newContext = Ctx->SetResponseForm(FORM_NAME_DIRECT_GALLERY, false /* setCurrentFormAsCallback */);
    Y_ENSURE(newContext);

    const auto& cards = searchResult.TrySelect("renderrer/docs[0]/scenario_response/layout/cards");
    if (cards.IsNull()) {
        NSc::TValue data;
        data["direct_items"] = items;
        data["has_disclaimer"] = hasDisclaimer;
        data["has_large_disclaimer"] = hasLargeDisclaimer;
        data["cards_with_button"] = !Ctx->HasExpFlag(FLAG_DO_NOT_ADD_BUTTON_LINK);

        data["card_height"] = 210;
        if (data["has_large_disclaimer"].GetBool()) {
            data["card_height"] = 250;
        } else if (data["has_disclaimer"].GetBool()) {
            data["card_height"] = 220;
        }

        if (Ctx->HasExpFlag(FLAG_DO_NOT_ADD_BUTTON_LINK)) {
            data["card_height"] = data["card_height"].GetIntNumber() - 40;
        }

        auto cgi = NAlice::NDirectGallery::MakeDirectExperimentCgi(Ctx->ClientFeatures().Experiments());
        data["serp_url"] = GenerateSearchUri(Ctx, query, cgi);

        newContext->AddDivCardBlock("direct_gallery", std::move(data));

        if (NSerp::IsPornoSerp(searchResult)) {
            newContext->AddTextCardBlock("direct_gallery_porno_warning");
        } else {
            newContext->AddTextCardBlock("direct_gallery_invitation_message");
        }
    } else {
        for (const auto& card : cards.GetArray()) {
            if (card.Has("text")) {
                newContext->AddTextCardBlock("direct_gallery_identity", card);
                continue;
            }
            const auto& divCard = card.TrySelect("div_card");
            if (!divCard.IsNull()) {
                newContext->AddPreRenderedDivCardBlock(divCard);
                continue;
            }
            const auto& div2Card = card.TrySelect("div2_card_extended");
            if (!div2Card.IsNull()) {
                newContext->AddDiv2CardBlock(div2Card["body"], div2Card["hide_borders"].GetBool());
                continue;
            }
        }
    }

    newContext->CreateSlot(SLOT_NAME_ITEMS, SLOT_TYPE_ITEMS, true /* optional */, items);
    newContext->AddStopListeningBlock();

    if (!linkTails.empty()) {
        newContext->AddCommitCandidateBlock(
            TDirectGalleryHitConfirmContinuation{Ctx, std::move(hitCounter), std::move(linkHead), std::move(linkTails)}
                .ToJson());
    }

    return newContext;
}

} // namespace NBASS::NDirectGallery
