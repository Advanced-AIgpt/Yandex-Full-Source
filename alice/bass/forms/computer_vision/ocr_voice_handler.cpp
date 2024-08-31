#include "ocr_voice_handler.h"

#include <library/cpp/resource/resource.h>

using namespace NBASS;
using namespace NImages::NCbir;

namespace NBASS {

    namespace NComputerVisionFlags {
        const TStringBuf ENABLE_OCR_VOICE_MORE = "image_recognizer_enable_ocr_voice_more";
    }

}


TCVAnswerOcrVoice::TCVAnswerOcrVoice(const TStringBuf suggestName)
        : IComputerVisionAnswer(
        suggestName,
        TStringBuf("https://avatars.mds.yandex.net/get-images-similar-mturk/13615/icon-ocr_voice/orig"))
{
}

TCVAnswerOcrVoice::TCVAnswerOcrVoice()
        : TCVAnswerOcrVoice(TComputerVisionOcrVoiceHandler::FormShortName())
{
}

bool TCVAnswerOcrVoice::IsSuitable(const TComputerVisionContext& ctx, bool force) const {
    return force || ctx.GetOcrResultCategory() > ORC_ANY;
}

void TCVAnswerOcrVoice::LoadData() {
    LOG(INFO) << "Loading swear" << Endl;
    const TString resource = NResource::Find("swear");
    TMemoryInput inp(resource);
    for (TUtf16String line; inp.ReadLine(line); ) {
        SwearWords.insert(line);
    }

    LOG(INFO) << "Loading cv swear" << Endl;
    const TString cvResource = NResource::Find("cv_swear");
    TMemoryInput cvInp(cvResource);
    for (TUtf16String line; cvInp.ReadLine(line); ) {
        SwearWords.insert(line);
    }
}

void TCVAnswerOcrVoice::Compose(TComputerVisionContext& ctx) const {
    const TSlot* silentModeSlot = ctx.HandlerContext().GetSlot("silent_mode", "num");
    if (silentModeSlot && silentModeSlot->Value.GetIntNumber() == 1) {
        ctx.Output("silent_mode").SetIntNumber(1);
        LOG(DEBUG) << silentModeSlot->Value.GetIntNumber() << Endl;
    } else {
        ctx.Output("silent_mode").SetIntNumber(0);
        LOG(DEBUG) << 0 << Endl;
    }

    const bool hasOcrText = ctx.AddOcrVoiceAnswer(SwearWords);
    ctx.AttachTextCard();
    ctx.Output("fast_ocr") = ToString(ctx.GetOcrResultCategory());
    if (hasOcrText) {
        ctx.AttachOcrSuggest("Распознанный текст");
        ctx.AttachUpdateFormSuggest("image_what_is_this__ocr_voice_speech",
                                    TComputerVisionOcrVoiceSuggestHandler::FormName(),
                                    TStringBuf("Прочитать еще раз"));
        if (ctx.HasExpFlag(NComputerVisionFlags::ENABLE_OCR_VOICE_MORE)) {
            AttachButtonMore(ctx);
        }
    }
}

void TCVAnswerOcrVoice::AttachButtonMore(TComputerVisionContext& ctx) const {
    NSc::TValue formUpdate;
    formUpdate["name"] = TComputerVisionOcrVoiceHandler::FormName();
    NSc::TValue& slots = formUpdate["slots"].SetArray();

    // Reset current image
    TContext::TSlot emptyAnswerSlot("answer", "image_result");
    emptyAnswerSlot.Value.SetNull();
    slots.Push(emptyAnswerSlot.ToJson(nullptr));

    // Force ocr voice answer
    TContext::TSlot forceAnswerSlot("forcing_answer", "string");
    forceAnswerSlot.Value = ForceName();
    slots.Push(forceAnswerSlot.ToJson(nullptr));

    // Enable silent mode
    TContext::TSlot silentModeSlot("silent_mode", "num");
    silentModeSlot.Value = 1;
    slots.Push(silentModeSlot.ToJson(nullptr));

    NSc::TValue data;
    data["label"] = "Прочитать следующий текст";
    ctx.HandlerContext().AddSuggest("image_what_is_this__ocr_voice_speech", data, formUpdate);
}

void TCVAnswerOcrVoice::AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const {
    ctx.AttachAlternativeIntentsSuggest(
            {ECbirIntents::CI_CLOTHES, ECbirIntents::CI_MARKET, ECbirIntents::CI_SIMILAR, ECbirIntents::CI_OCR},
            {NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE, NImages::NCbir::ECbirIntents::CI_INFO});
}


TCVAnswerOcrVoiceSuggest::TCVAnswerOcrVoiceSuggest()
        : TCVAnswerOcrVoice(TComputerVisionOcrVoiceSuggestHandler::FormShortName())
{
}

// TComputerVisionOcrVoiceHandler ------------------------------------------------
TComputerVisionOcrVoiceHandler::TComputerVisionOcrVoiceHandler()
    : TComputerVisionMainHandler(ECaptureMode::OCR_VOICE, false)
{}

void TComputerVisionOcrVoiceHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionOcrVoiceHandler>();
    };
    handlers->emplace(FormName(), handler);
}

TMaybe<TString> TComputerVisionOcrVoiceHandler::GetForcingString() const {
    return MakeMaybe<TString>(TCVAnswerOcrVoice::ForceName());
}

// TComputerVisionOcrVoiceSuggestHandler ------------------------------------------------
void TComputerVisionOcrVoiceSuggestHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionOcrVoiceSuggestHandler>();
    };
    handlers->emplace(FormName(), handler);
}

bool TComputerVisionOcrVoiceSuggestHandler::MakeBestAnswer(TComputerVisionContext& cvContext) const {
    cvContext.Switch(NComputerVisionForms::IMAGE_WHAT_IS_THIS);
    return AnswerOcrVoice.TryApplyTo(cvContext, /* force */ true);
}

// TComputerVisionEllipsisOcrVoiceHandler ------------------------------------------------
void TComputerVisionEllipsisOcrVoiceHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionEllipsisOcrVoiceHandler>();
    };
    handlers->emplace(FormName(), handler);
}

bool TComputerVisionEllipsisOcrVoiceHandler::MakeBestAnswer(TComputerVisionContext& cvContext) const {
    cvContext.Switch(NComputerVisionForms::IMAGE_WHAT_IS_THIS);
    return AnswerOcrVoice.TryApplyTo(cvContext, /* force */ true);
}
