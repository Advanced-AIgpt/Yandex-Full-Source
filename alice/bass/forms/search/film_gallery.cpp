#include "film_gallery.h"
#include "serp.h"

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {
namespace NFilmGallery {

namespace {

constexpr TStringBuf FORM_NAME_FILM_GALLERY = "personal_assistant.scenarios.film_gallery";
constexpr TStringBuf FORM_NAME_FILM_GALLERY_SCROLL_NEXT = "personal_assistant.scenarios.film_gallery__scroll_next";

constexpr TStringBuf SLOT_NAME_ANSWER = "answer";
constexpr TStringBuf SLOT_TYPE_ANSWER = "answer";

struct TFilmGalleryAnswer {
    NSc::TValue Items;
    i64 Idx = 0;
    i64 PageSize = 3;

    NSc::TValue ToJson() const {
        NSc::TValue result;
        result["gallery_items"] = Items;
        result["idx"].SetIntNumber(Idx);
        result["page_size"].SetIntNumber(PageSize);
        if (static_cast<size_t>(Idx + PageSize) >= Items.ArraySize()) {
            result["has_no_more"].SetBool(true);
        }
        return result;
    }

    static TMaybe<TFilmGalleryAnswer> FromJson(const NSc::TValue& json) {
        const auto& items = json.TrySelect("gallery_items");
        const auto& idx = json.TrySelect("idx");
        const auto& pageSize = json.TrySelect("page_size");

        if (items.IsArray() && idx.IsIntNumber() && pageSize.IsIntNumber()) {
            return TFilmGalleryAnswer({items, idx.GetIntNumber(), pageSize.GetIntNumber()});
        }

        return Nothing();
    }
};

NSc::TValue TryConstructFilmGalleryItems(const NSc::TValue& parentCollection) {
    NSc::TValue items, nullTypeItems;
    items.SetArray();
    nullTypeItems.SetArray();

    for (const auto& object : parentCollection.TrySelect("object").GetArray()) {
        NSc::TValue item;
        item["title"] = object.TrySelect("title");
        item["ugc_type"] = object.TrySelect("ugc_type");
        if (item["ugc_type"].IsNull()) {
            nullTypeItems.Push(item);
        } else {
            items.Push(item);
        }
    }
    items.AppendAll(nullTypeItems.GetArray().cbegin(), nullTypeItems.GetArray().cend());

    return items;
}

void SetupFilmGalleryResponse(const TFilmGalleryAnswer& answer, TContext& ctx) {
    auto newCtx = ctx.SetResponseForm(FORM_NAME_FILM_GALLERY, false /* setCurrentFormAsCallback */);
    Y_ENSURE(newCtx);
    newCtx->CreateSlot(SLOT_NAME_ANSWER, SLOT_TYPE_ANSWER, true /* optional */, answer.ToJson());
}

void HandleScrollNext(TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::ONTOFACTS);
    TSlot* answerSlot = ctx.GetSlot(SLOT_NAME_ANSWER, SLOT_TYPE_ANSWER);
    if (IsSlotEmpty(answerSlot)) {
        LOG(ERR) << "Expected the answer slot to be filled, but found it empty" << Endl;
        return;
    }

    auto filmGalleryAnswer = TFilmGalleryAnswer::FromJson(answerSlot->Value);
    if (!filmGalleryAnswer) {
        LOG(ERR) << "Failed to parse film gallery answer from the answer slot" << Endl;
        return;
    }

    filmGalleryAnswer->Idx += filmGalleryAnswer->PageSize;
    answerSlot->Value = filmGalleryAnswer->ToJson();
}

} // namespace

TMaybe<TFilmGalleryBuilder> TFilmGalleryBuilder::Create(TContext& ctx) {
    if (!ctx.HasExpFlag(EXPERIMENTAL_FLAG_FILM_GALLERY) ||
        ctx.IsIntentForbiddenByExperiments(FORM_NAME_FILM_GALLERY) ||
        ctx.IsIntentForbiddenByExperiments(FORM_NAME_FILM_GALLERY_SCROLL_NEXT))
    {
        return Nothing();
    }

    if (!ctx.MetaClientInfo().IsSmartSpeaker() || ctx.Meta().DeviceState().IsTvPluggedIn()) {
        return Nothing();
    }

    return TFilmGalleryBuilder(ctx);
}

TFilmGalleryBuilder::TFilmGalleryBuilder(TContext& ctx)
    : Ctx(ctx)
{
}

bool TFilmGalleryBuilder::TryBuild(const NSc::TValue& searchResult) {
    for (const auto& doc : searchResult.TrySelect("searchdata/docs").GetArray()) {
        for (const auto* snippet : FindSnippets(doc, "entity_search", NSerpSnippets::ESnippetSection::ESS_ALL)) {
            const auto& parentCollection = snippet->TrySelect("data/parent_collection");
            const auto& avatarType = parentCollection.TrySelect("avatar_type");
            if (avatarType.GetString() != "vertical_film") {
                continue;
            }

            const auto galleryItems = TryConstructFilmGalleryItems(parentCollection);
            if (galleryItems.ArraySize() > 0) {
                SetupFilmGalleryResponse({galleryItems}, Ctx);
                return true;
            }
        }
    }

    return false;
}

const TVector<TFormHandlerPair> FORM_HANDLER_PAIRS({
    {FORM_NAME_FILM_GALLERY_SCROLL_NEXT, HandleScrollNext},
});

} // NFilmGallery
} // NBASS
