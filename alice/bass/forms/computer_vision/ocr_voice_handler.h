#pragma once

#include <alice/bass/forms/computer_vision/context.h>
#include <alice/bass/forms/computer_vision/handler.h>

namespace NBASS {
    class TCVAnswerOcrVoice: public IComputerVisionAnswer {
    public:
        TCVAnswerOcrVoice();

        bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;

        void LoadData() override;

        static inline TStringBuf ForceName() {
            return "computer_vision_force_ocr_voice";
        }

        bool IsNeedOcrData() const override {
            return true;
        }

        TStringBuf GetAnswerId() const override {
            return TStringBuf("ocr_voice");
        }

        void AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const override;

    protected:
        TCVAnswerOcrVoice(const TStringBuf suggestName);
        void Compose(TComputerVisionContext& ctx) const override;
        ECaptureMode AnswerType() const override {
            return ECaptureMode::OCR_VOICE;
        }


    private:
        void AttachButtonMore(TComputerVisionContext& ctx) const;

    private:
        THashSet<TUtf16String> SwearWords;
    };

    class TCVAnswerOcrVoiceSuggest: public TCVAnswerOcrVoice {
    public:
        TCVAnswerOcrVoiceSuggest();
    };

    class TComputerVisionOcrVoiceHandler: public TComputerVisionMainHandler {
    public:
        TComputerVisionOcrVoiceHandler();

        static void Register(THandlersMap* handlers);
        TMaybe<TString> GetForcingString() const;

        static inline TStringBuf FormShortName() {
            return TStringBuf("image_what_is_this_ocr_voice");
        };

        static inline TStringBuf FormName() {
            return TStringBuf("personal_assistant.scenarios.image_what_is_this_ocr_voice");
        };
    };

    class TComputerVisionOcrVoiceSuggestHandler : public TComputerVisionMainHandler {
    public:
        static void Register(THandlersMap* handlers);

        static inline TStringBuf FormShortName() {
            return TStringBuf("image_what_is_this__ocr_voice_suggest");
        };

        static inline TStringBuf FormName() {
            return TStringBuf("personal_assistant.scenarios.image_what_is_this__ocr_voice_suggest");
        };

    protected:
        bool MakeBestAnswer(TComputerVisionContext& cvContext) const override;
        bool IsNeedOcrData(const IComputerVisionAnswer*) const override {
            return true;
        }

    protected:
        TCVAnswerOcrVoice AnswerOcrVoice;
    };

    class TComputerVisionEllipsisOcrVoiceHandler : public TComputerVisionMainHandler {
    public:
        TComputerVisionEllipsisOcrVoiceHandler()
            : TComputerVisionMainHandler(ECaptureMode::OCR_VOICE, false)
        {}

        static void Register(THandlersMap* handlers);

        static inline TStringBuf FormShortName() {
            return TStringBuf("image_what_is_this__ocr_voice");
        };

        static inline TStringBuf FormName() {
            return TStringBuf("personal_assistant.scenarios.image_what_is_this__ocr_voice");
        };

    protected:
        bool MakeBestAnswer(TComputerVisionContext& cvContext) const override;
        bool IsNeedOcrData(const IComputerVisionAnswer*) const override {
            return true;
        }

    protected:
        TCVAnswerOcrVoice AnswerOcrVoice;
    };
}
