#include "barcode.h"
#include "ocr.h"

namespace {
    constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_barcode";
    constexpr TStringBuf SHORT_ANSWER_NAME = "barcode";
    constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_barcode";

    constexpr TStringBuf ALICE_SMART_MODE = "smartcamera";
}

namespace NAlice::NHollywood::NImage::NComputerVisionFeedbackOptions {

constexpr TStringBuf BARCODE_WRONG = "feedback_negative_images__barcode_wrong";
constexpr TStringBuf BARCODE_UNWANTED = "feedback_negative_images__barcode_unwanted";

}

namespace NAlice::NHollywood::NImage::NAnswers {

    TBarcode::TBarcode(): IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
    {
        Intent = NImages::NCbir::ECbirIntents::CI_BARCODE;
        CaptureMode = ECaptureMode::TInput_TImage_ECaptureMode_Barcode;
        AllowedIntents = {
            NImages::NCbir::ECbirIntents::CI_OCR,
            NImages::NCbir::ECbirIntents::CI_OCR_VOICE
        };
        LastForceAlternativeSuggest = {
            NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE,
            NImages::NCbir::ECbirIntents::CI_INFO
        };

        AliceSmartMode = ALICE_SMART_MODE;
    }

    bool TBarcode::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool) const {
        const TMaybe<NSc::TValue>& imageAliceResponseMaybe = ctx.GetImageAliceResponse();
        if (!imageAliceResponseMaybe.Defined()) {
            return false;
        }

        const NSc::TValue& imageAliceResponse = imageAliceResponseMaybe.GetRef();
        return (imageAliceResponse["Barcode"].ArraySize() > 0 && !imageAliceResponse.TrySelect("/Barcode[0]/Type").IsNull());
    }

    void TBarcode::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
        const NSc::TValue& cvAnswer = ctx.GetImageAliceResponse().GetRef();
        const TString barcodeType = cvAnswer.TrySelect("/Barcode[0]/Type").ForceString();
        NSc::TValue card;
        NSc::TArray& cardContacts = card["contacts"].SetArray().GetArrayMutable();
        TStringBuf textToShow(" ");
        if (barcodeType == "URI") {
            const NSc::TValue& uri = cvAnswer.TrySelect("/Barcode[0]/URI");
            cardContacts.push_back(ctx.GenerateContact(uri, EContactType::CT_URL));
        } else if (barcodeType == "Telephone" || barcodeType == "SMS") {
            const NSc::TValue& number = cvAnswer.TrySelect("/Barcode[0]/Number");
            cardContacts.push_back(ctx.GenerateContact(number, EContactType::CT_PHONE));
        } else if (barcodeType == "Email") {
            const NSc::TValue& mailTo = cvAnswer.TrySelect("/Barcode[0]/To");
            cardContacts.push_back(ctx.GenerateContact(mailTo, EContactType::CT_MAIL));
        } else if (barcodeType == "PlainText") {
            const TStringBuf textAll = cvAnswer.TrySelect("/Barcode[0]/Text").GetString();
            int textLength = textAll.size();
            auto applyContact = [&](const TStringBuf path, const EContactType type) -> void {
                const NSc::TValue& regexp = cvAnswer.TrySelect(path);
                if (!regexp.IsNull()) {
                    cardContacts.push_back(ctx.GenerateContact(regexp, type));
                }
            };
            applyContact("/Barcode[0]/RegExpMail", EContactType::CT_MAIL);
            applyContact("/Barcode[0]/RegExpTel", EContactType::CT_PHONE);
            applyContact("/Barcode[0]/RegExpUrl", EContactType::CT_URL);
            constexpr double MIN_FRACTION_OF_NONCONTACTS_TEXT = 0.2;
            if (textLength > MIN_FRACTION_OF_NONCONTACTS_TEXT * textAll.size()) {
                textToShow = textAll;
            }

            ctx.GetAnalyticsInfoBuilder().AddObject("barcode_text", "barcode_text", TString(textAll));
        } else {
            ctx.AddError("barcode_unknown_type");
            ctx.StatIncCounter("hollywood_computer_vision_result_error_barcode_unknown_type");
            return;
        }

        NSc::TValue textCardData;
        textCardData["text"] = textToShow;
        ctx.AddTextCard("render_barcode", textCardData);
        if (!cardContacts.empty()) {
            ctx.StatIncCounter("hollywood_computer_vision_result_answer_barcode_contacts");
            ctx.AddDivCardBlock(TStringBuf("ocr_contact"), std::move(card));
        } else {
            ctx.StatIncCounter("hollywood_computer_vision_result_answer_barcode_text");
        }

        TOcr* ocrAnswer = TOcr::GetPtr();
        ocrAnswer->AddOcrAnswer(ctx, EOcrResultCategory::ORC_VAGUE);
        ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::BARCODE_WRONG);
        ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::BARCODE_UNWANTED);
    }

    TBarcode* TBarcode::GetPtr() {
        static TBarcode* answer = new TBarcode;
        return answer;
    }

}
