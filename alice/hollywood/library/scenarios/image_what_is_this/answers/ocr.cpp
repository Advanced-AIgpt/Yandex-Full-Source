#include <alice/hollywood/library/scenarios/image_what_is_this/answers/ocr.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/context.h>

#include <alice/library/url_builder/url_builder.h>

#include <util/charset/wide.h>

using namespace NAlice::NHollywood::NImage;
using namespace NAlice::NHollywood::NImage::NAnswers;

namespace NAlice::NHollywood::NImage::NFlags {

const TString DISABLE_AUTO_OCR = "image_recognizer_disable_auto_ocr";

}

namespace NAlice::NHollywood::NImage::NComputerVisionFeedbackOptions {

constexpr TStringBuf OCR_POOR = "feedback_negative_images__ocr_poor";
constexpr TStringBuf OCR_TRANSLATE = "feedback_negative_images__ocr_translate";
constexpr TStringBuf OCR_UNWANTED = "feedback_negative_images__ocr_unwanted";

}

namespace {

constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_ocr";
constexpr TStringBuf SHORT_ANSWER_NAME = "ocr";
constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_ocr";

constexpr TStringBuf ALICE_MODE = "text";

}

TOcr::TOcr()
    : IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
{
    Intent = NImages::NCbir::ECbirIntents::CI_OCR;
    IntentButtonIcon = "https://avatars.mds.yandex.net/get-images-similar-mturk/40186/icon-ocr/orig";
    CaptureMode = ECaptureMode::TInput_TImage_ECaptureMode_Ocr;

    AllowedIntents = {
        NImages::NCbir::ECbirIntents::CI_CLOTHES,
        NImages::NCbir::ECbirIntents::CI_MARKET,
        NImages::NCbir::ECbirIntents::CI_SIMILAR,
        NImages::NCbir::ECbirIntents::CI_OCR_VOICE,
    };

    LastForceAlternativeSuggest = {
        NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE,
        NImages::NCbir::ECbirIntents::CI_INFO,
    };

    AliceMode = ALICE_MODE;
}

bool TOcr::AddOcrAnswer(TImageWhatIsThisApplyContext& ctx, const EOcrResultCategory minToSuggest, bool force) const {
    const EOcrResultCategory ocrResult = GetOcrResultCategory(ctx);
    if (ocrResult > EOcrResultCategory::ORC_NONE) {
        if (ocrResult >= minToSuggest && force) {
            const TString uriButton = ctx.GenerateImagesSearchUrl("ocr", TStringBuf("imageocr"),
                                                                  /* disable ptr */ true);
            ctx.AddOpenUriButton("ocr_find_translate", uriButton);
            // TODO:
            //AttachOcrSuggest();
        }
        // TODO:
        // Result["fast_ocr"] = ToString(ocrResult);
        return true;
    }
    return false;
}

EOcrResultCategory TOcr::GetOcrResultCategory(const TImageWhatIsThisApplyContext& ctx) const {
    constexpr size_t minWordsCountVague = 2;
    constexpr size_t minWordsCountCertain = 8;
    constexpr size_t minLinesCountCertain = 3;

    if (!ctx.GetImageAliceResponse().Defined()) {
        return ORC_NONE;
    }
    const NSc::TValue& textLines = ctx.GetImageAliceResponse().GetRef()["FastOcr"]["fulltext"];
    size_t wordsCount = 0;
    const TUtf16String wideDelim(u" ");
    const TSetDelimiter<const TChar> delim(wideDelim.data());
    for (const auto& item : textLines.GetArray()) {
        const auto text = UTF8ToWide(item["Text"].ForceString());
        TVector<TUtf16String> tokens;
        TContainerConsumer<TVector<TUtf16String>> consumer(&tokens);
        SplitString(text.begin(), text.end(), delim, consumer);
        wordsCount += tokens.size();
        if (wordsCount >= minWordsCountCertain) {
            return (textLines.ArraySize() >= minLinesCountCertain) ? ORC_CERTAIN : ORC_VAGUE;
        }
    }
    if (wordsCount >= minWordsCountVague) {
        return ORC_VAGUE;
    }
    return (wordsCount > 0) ? ORC_ANY : ORC_NONE;
}

TOcr* TOcr::GetPtr() {
    static TOcr* answer = new TOcr;
    return answer;
}

bool TOcr::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const {
    return GetOcrResultCategory(ctx) >
        (force ? EOcrResultCategory::ORC_NONE : EOcrResultCategory::ORC_ANY);
}

void TOcr::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::OCR_POOR);
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::OCR_TRANSLATE);
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::OCR_UNWANTED);
    AddOcrAnswer(ctx, EOcrResultCategory::ORC_VAGUE, /*force*/ true);

    ctx.AddButton("alice.image_what_is_this_ocr_voice", "switch_to_ocr_voice", {});

    NSc::TValue contactsCard = GetContactsOCR(ctx);
    if (!AddContactsCard(std::move(contactsCard), ctx)) {
        ctx.AddTextCard("render_ocr_text", {});
        if (!ctx.HasFlag(NFlags::DISABLE_AUTO_OCR)) {
            ctx.RedirectTo("ocr", "imageocr", /* disable ptr */ true);
        }
    }
}

bool TOcr::RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const {
    if (ctx.GetImageAliceResponse().Defined()) {
        ctx.AddTextCard("render_cannot_apply_ocr_error", {});
        return true;
    }
    return false;
}

NSc::TValue TOcr::GetContactsOCR(TImageWhatIsThisApplyContext& ctx) const {
    static const TMap<TStringBuf, EContactType> metaToContactType = {
        {TStringBuf("url"), EContactType::CT_URL},
        {TStringBuf("phone"), EContactType::CT_PHONE},
        {TStringBuf("email"), EContactType::CT_MAIL},
    };

    NSc::TValue card;
    NSc::TArray& cardContacts = card["contacts"].SetArray().GetArrayMutable();
    for (const auto& entity : ctx.GetImageAliceResponse().GetRef().TrySelect("/FastOcr")["entities"].GetArray()) {
        const TStringBuf meta = entity["type"].GetString();
        const TMap<TStringBuf, EContactType>::const_iterator metaTypeIter = metaToContactType.find(meta);
        if (metaTypeIter == metaToContactType.end()) {
            continue;
        }
        cardContacts.push_back(ctx.GenerateContact(entity["text_clean"], metaTypeIter->second));
        if (cardContacts.size() == 3) {
            return card;
        }
    }
    return cardContacts.size() > 0 ? card : NSc::Null();
}

bool TOcr::AddContactsCard(NSc::TValue card, TImageWhatIsThisApplyContext& ctx) const {
    const NSc::TArray& contacts = card["contacts"].GetArray();
    if (contacts.empty()) {
        return false;
    }
    if (contacts.size() == 1) {
        const EContactType contactType = FromString(contacts.front()["type"].GetString());
        if (contactType == EContactType::CT_URL) {
            ctx.AddTextCard("render_url_contact", {});
        } else if (contactType == EContactType::CT_PHONE) {
            ctx.AddTextCard("render_phone_contact", {});
        } else if (contactType == EContactType::CT_MAIL) {
            ctx.AddTextCard("render_email_contact", {});
        }
    } else {
        ctx.AddTextCard("render_contacts_result", {});
    }

    ctx.AddDivCardBlock("ocr_contact", std::move(card));
    return true;
}
