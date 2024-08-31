#include <alice/bass/forms/computer_vision/barcode_handler.h>

#include <alice/bass/libs/metrics/metrics.h>

using namespace NBASS;

// TCVAnswerBarcode ------------------------------------------------------------
bool TCVAnswerBarcode::IsSuitable(const TComputerVisionContext& ctx, bool /* force */) const {
    const NSc::TValue& cvAnswer = ctx.GetData();
    return (cvAnswer["Barcode"].ArraySize() > 0 &&
            !cvAnswer.TrySelect("/Barcode[0]/Type").IsNull());
}

void TCVAnswerBarcode::Compose(TComputerVisionContext& ctx) const {
    constexpr double MIN_FRACTION_OF_NONCONTACTS_TEXT = 0.2;
    const NSc::TValue& cvAnswer = ctx.GetData();
    const TString barcodeType = cvAnswer.TrySelect("/Barcode[0]/Type").ForceString();
    NSc::TValue card;
    NSc::TArray& cardContacts = card["contacts"].SetArray().GetArrayMutable();
    TStringBuf textToShow(" ");

    if (TStringBuf("URI") == barcodeType) {
        const NSc::TValue& barcodeUri = cvAnswer.TrySelect("/Barcode[0]/URI");
        ctx.AppendContact(barcodeUri, EContactType::CT_URL, cardContacts);

    } else if (TStringBuf("Telephone") == barcodeType || TStringBuf("SMS") == barcodeType) {
        const NSc::TValue& barcodeNumber = cvAnswer.TrySelect("/Barcode[0]/Number");
        ctx.AppendContact(barcodeNumber, EContactType::CT_PHONE, cardContacts);

    } else if (TStringBuf("Email") == barcodeType) {
        const NSc::TValue& barcodeTo = cvAnswer.TrySelect("/Barcode[0]/To");
        ctx.AppendContact(barcodeTo, EContactType::CT_MAIL, cardContacts);

    } else if (TStringBuf("PlainText") == barcodeType) {
        const TStringBuf textAll = cvAnswer.TrySelect("/Barcode[0]/Text").GetString();
        int textLength = textAll.size();

        auto applyContact = [&](const TStringBuf path, const EContactType type) -> void {
            const NSc::TValue& regexp = cvAnswer.TrySelect(path);
            if (!regexp.IsNull()) {
                ctx.AppendContact(regexp, type, cardContacts);
                textLength -= regexp.GetString().size();
            }
        };
        applyContact("/Barcode[0]/RegExpMail", EContactType::CT_MAIL);
        applyContact("/Barcode[0]/RegExpTel", EContactType::CT_PHONE);
        applyContact("/Barcode[0]/RegExpUrl", EContactType::CT_URL);

        if (textLength > MIN_FRACTION_OF_NONCONTACTS_TEXT * textAll.size()) {
            textToShow = textAll;
        }
    } else {
        ctx.Output("code").SetString("barcode_unknown_type");
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_barcode_unknown_type");
        return;
    }

    ctx.AttachTextCard();
    ctx.Output("barcode_text") = textToShow;
    if (!cardContacts.empty()) {
        ctx.HandlerContext().AddDivCardBlock(TStringBuf("ocr_contact"), std::move(card));
        Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_barcode_contacts");
    } else {
        Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_barcode_text");
    }
    ctx.AddOcrAnswer(EOcrResultCategory::ORC_VAGUE);
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::BARCODE_WRONG);
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::BARCODE_UNWANTED);
}

void TCVAnswerBarcode::AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const {
    ctx.AttachAlternativeIntentsSuggest({NImages::NCbir::ECbirIntents::CI_OCR, NImages::NCbir::ECbirIntents::CI_OCR_VOICE},
                                        {NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE, NImages::NCbir::ECbirIntents::CI_INFO});
}

TComputerVisionBarcodeHandler::TComputerVisionBarcodeHandler()
        : TComputerVisionMainHandler(ECaptureMode::BARCODE, false)
{
}

void TComputerVisionBarcodeHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionBarcodeHandler>();
    };
    handlers->emplace(TComputerVisionBarcodeHandler::FormName(), handler);
}
