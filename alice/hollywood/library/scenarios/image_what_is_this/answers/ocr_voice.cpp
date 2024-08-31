#include <alice/hollywood/library/scenarios/image_what_is_this/answers/ocr_voice.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/ocr.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/context.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/utils.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/image_what_is_this_resources.h>

#include <library/cpp/resource/resource.h>

#include <util/string/join.h>
#include <util/string/subst.h>
#include <util/charset/utf8.h>

using namespace NAlice::NHollywood::NImage::NAnswers;

namespace {

constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_ocr_voice";
constexpr TStringBuf SHORT_ANSWER_NAME = "ocr_voice";
constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_ocr_voice";

constexpr TStringBuf CBIRD = "28";

constexpr TStringBuf ALICE_MODE = "voice_text";

}

TOcrVoice::TOcrVoice()
    : IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
{
    IntentButtonIcon = "https://avatars.mds.yandex.net/get-images-similar-mturk/13615/icon-ocr_voice/orig";
    AllowedIntents = {
        NImages::NCbir::ECbirIntents::CI_CLOTHES,
        NImages::NCbir::ECbirIntents::CI_MARKET,
        NImages::NCbir::ECbirIntents::CI_SIMILAR,
        NImages::NCbir::ECbirIntents::CI_OCR,
    };

     LastForceAlternativeSuggest = {
         NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE,
         NImages::NCbir::ECbirIntents::CI_INFO,
     };

     CaptureMode = ECaptureMode::TInput_TImage_ECaptureMode_OcrVoice;
     Intent = NImages::NCbir::ECbirIntents::CI_OCR_VOICE;

     AliceMode = ALICE_MODE;
}

void TOcrVoice::MakeRequests(TImageWhatIsThisApplyContext& ctx) const {
    if (!ctx.GetImageAliceResponse().Defined()) {
        ctx.AddImageAliceRequest();
    }
    if (!ctx.GetCbirFeaturesResponse().Defined()) {
        ctx.AddCbirFeaturesRequest(CBIRD);
    }
}

TOcrVoice* TOcrVoice::GetPtr() {
    static TOcrVoice* answer = new TOcrVoice;
    return answer;
}

bool TOcrVoice::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const {
    const TMaybe<NSc::TValue>& imageAliceResponseMaybe = ctx.GetImageAliceResponse();
    const TMaybe<NSc::TValue>& cbirFeaturesResponseMaybe = ctx.GetCbirFeaturesResponse();
    if (!imageAliceResponseMaybe.Defined() || !cbirFeaturesResponseMaybe.Defined()) {
        return false;
    }

    TOcr* ocrAnswer = TOcr::GetPtr();
    return force || ocrAnswer->GetOcrResultCategory(ctx) > ORC_ANY;
}

bool TOcrVoice::IsSuggestibleAnswer(TImageWhatIsThisApplyContext& ctx) const {
    const TMaybe<NSc::TValue>& imageAliceResponseMaybe = ctx.GetImageAliceResponse();
    if (!imageAliceResponseMaybe.Defined()) {
        return false;
    }

    TOcr* ocrAnswer = TOcr::GetPtr();
    return ocrAnswer->GetOcrResultCategory(ctx) > ORC_ANY;
}

void TOcrVoice::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
    const NAlice::NScenarios::TCallbackDirective* callback = ctx.GetCallback();
    TMaybe<TString> silentModeMaybe;
    if (callback) {
        silentModeMaybe = ctx.ExtractPayloadField(callback, "silent_mode");
    }
    const bool isSilentMode = silentModeMaybe.Defined() && silentModeMaybe.GetRef() == "1";

    AddOcrVoiceAnswer(ctx, isSilentMode);
}

bool TOcrVoice::AddOcrVoiceAnswer(TImageWhatIsThisApplyContext& ctx, bool silentMode) const {
    NSc::TValue text;
    NSc::TArray& texts = text.SetArray().GetArrayMutable();
    TVector<TStringBuf> blockTexts;
    bool isHypPrevBlock = false;
    bool hasAnyText = false;

    for (const auto& ocrBlock : ctx.GetCbirFeaturesResponse().GetRef()["ocr"]["data"]["blocks"].GetArray()) {
        if (!isHypPrevBlock) {
            blockTexts.clear();
        }
        const TStringBuf language = ocrBlock["lang"].GetString();
        const TStringBuf type = ocrBlock["type"].GetString();
        hasAnyText |= type == "main";
        // There is "bak" language since it occurs very often in russian texts
        if ((language != "rus" && language != "bak" && language != "eng")
            || type != "main") {
            continue;
        }
        for (const auto &ocrBox : ocrBlock["boxes"].GetArray()) {
            for (const auto &ocrLangBox : ocrBox["languages"].GetArray()) {
                for (const auto &ocrText : ocrLangBox["texts"].GetArray()) {
                    TStringBuf text = ocrText["text"];
                    if (text.empty()) {
                        continue;
                    }

                    bool isHypCurrentBlock = text.back() == '-';
                    if (isHypCurrentBlock) {
                        text = text.Head(text.size() - 1);
                    }

                    if (isHypPrevBlock) {
                        blockTexts.push_back(text);
                    } else {
                        if (!blockTexts.empty()) {
                            blockTexts.push_back(TStringBuf(" "));
                        }
                        blockTexts.push_back(text);
                    }

                    isHypPrevBlock = isHypCurrentBlock;
                }
            }
        }
        if (!isHypPrevBlock) {
            TString joinedText = JoinSeq("", blockTexts);
            DecreaseSentencesLength(joinedText);
            texts.push_back(TStringBuf(joinedText));
        }
    }

    if (isHypPrevBlock) {
        TString joinedText = JoinSeq("", blockTexts);
        DecreaseSentencesLength(joinedText);
        texts.push_back(TStringBuf(joinedText));
    }

    NSc::TValue filteredTexts;
    if (FixSwear(texts, filteredTexts, ctx)) {
        AddButtons(ctx);
        ctx.AddTextCard("render_ocr_voice_swear", {});
        ctx.GetAnalyticsInfoBuilder().AddObject("ocr_voice_swear", "ocr_voice_swear", "1");
        return true;
    }

    if (texts.empty()) {
        if (!hasAnyText) {
            ctx.AddTextCard("render_ocr_voice_no_text", {});
            ctx.GetAnalyticsInfoBuilder().AddObject("ocr_voice_no_text", "ocr_voice_no_text", "1");
            return false;
        } else {
            AddButtons(ctx);
            ctx.AddTextCard("render_ocr_voice_foreign_text", {});
            ctx.GetAnalyticsInfoBuilder().AddObject("ocr_voice_foreign_text", "ocr_voice_foreign_text", "1");
            return true;
        }
    }

    constexpr int textLen = 50;
    TString outText = CutTexts(filteredTexts, textLen, true);

    if (!ReplaceAsterisks(filteredTexts.GetArrayMutable())) {
        ctx.AddTextCard("render_ocr_voice_no_text", {});
        return false;
    }

    NSc::TValue data;
    data["voice"] = filteredTexts;
    data["text"] = outText;
    data["silent_mode"] = silentMode;

    AddButtons(ctx);
    ctx.AddTextCard("render_ocr_voice_result", data);

    return true;
}

void TOcrVoice::DecreaseSentencesLength(TString& text) const {
    const TString delimiters = ".!?";
    TFindFirstOf<const char> splitFinder(delimiters.Data());
    const size_t maxSentenceSize = 500;

    // Splits sentences every 500 characters by dots because Speechkit can not read long sentences now.
    size_t currentStringPosition = 0;
    while (currentStringPosition < text.size()) {
        const char* sentencePtr = text.begin() + currentStringPosition;
        const char* sentenceEnd = splitFinder.FindFirstOf(sentencePtr, text.end());
        size_t sentenceSize = sentenceEnd - sentencePtr;

        if (sentenceSize <= maxSentenceSize) {
            currentStringPosition += sentenceSize + 1;
            continue;
        }

        TStringBuf subSentence(sentencePtr, maxSentenceSize);

        size_t splitPos = subSentence.rfind(" ");
        if (splitPos == TStringBuf::npos) {
            currentStringPosition += sentenceSize + 1;
            continue;
        }

        text.insert(currentStringPosition + splitPos, ".");
        currentStringPosition = currentStringPosition + splitPos + 2;
    }
}

TString TOcrVoice::CutTexts(const NSc::TArray& texts, size_t len, bool appendEllipsis) const {
    TVector<TString> sentences;
    TString currentText;
    size_t currentLen = 0;
    for (size_t i = 0; i != texts.size(); ++i) {
        TString originalText = TString{texts[i].GetString()};
        if (originalText.empty()) {
            continue;
        }
        TString addedText = CutText(originalText, len - currentLen);

        if (!addedText.empty()) {
            sentences.push_back(addedText);
            currentLen += GetNumberOfUTF8Chars(addedText);
        }

        if (addedText.empty()
                || addedText.size() < originalText.size()) {
            if (appendEllipsis && !sentences.empty()) {
                if (!sentences.back().empty() && sentences.back().back() == '.') {
                    sentences.back() += "..";
                } else {
                    sentences.back() += "...";
                }
            }
            break;
        }
    }
    return JoinRange(" ", sentences.begin(), sentences.end());
}


bool TOcrVoice::ReplaceAsterisks(NSc::TArray &texts) const {
    bool hasAnyText = false;
    for (auto& text : texts) {
        TUtf16String textStr = UTF8ToWide(TString{text.GetString()});
        size_t replacedCount = SubstGlobal(textStr, u"***", u"");
        if (replacedCount) {
            text.SetString(WideToUTF8(textStr));
        }
        if (!hasAnyText) {
            for (const auto c: textStr) {
                if (IsAlnum(c)) {
                    hasAnyText = true;
                }
            }
        }
    }

    return hasAnyText;
}

bool TOcrVoice::FixSwear(const NSc::TArray& texts, NSc::TValue& result, const TImageWhatIsThisApplyContext& ctx) const {
    TString delimiters = ".,; \n";
    const int enoughLongWords = 10;
    const size_t longWordSize = 3;
    bool isContainsEnoughLongWords = IsContainsEnoughLongWords(texts, longWordSize, enoughLongWords, delimiters);

    TFindFirstOf<const char> splitFinder(delimiters.Data());
    const TString replaceString = "***";

    NSc::TArray& resultTextsArray = result.SetArray().GetArrayMutable();

    // Checks every word on obscene. If the text is short, then we refuses to read. Otherwise, we replace these word by '***' in text
    // and by empty string in speech text
    for (const auto& text : texts) {
        TString textStr = TString{text.GetString()};

        size_t currentStringPosition = 0;
        while (currentStringPosition < textStr.size()) {
            const char* currentStartString = textStr.begin() + currentStringPosition;
            const char* delimPosition = splitFinder.FindFirstOf(currentStartString, textStr.end());

            const size_t wordSize = delimPosition - currentStartString;

            TStringBuf word(currentStartString, wordSize);
            if (!word.empty()) {
                if (IsObscene(TString(word), ctx)) {
                    if (isContainsEnoughLongWords) {
                        textStr.replace(currentStringPosition, wordSize, replaceString);
                        currentStringPosition += 4;
                    } else {
                        return true;
                    }
                } else {
                    currentStringPosition += word.size() + 1;
                }
            } else {
                currentStringPosition += 1;
            }
        }
        resultTextsArray.push_back(TStringBuf(textStr));
    }

    return false;
}

bool TOcrVoice::IsContainsEnoughLongWords(const NSc::TArray& texts, size_t wordLength,
                                          int enoughWords, const TString& delimeters) const {
    int countLongWords = 0;
    TFindFirstOf<const char> splitFinder(delimeters.Data());

    // Checks whether is text contains words count that is longer than word length
    for (const auto& text : texts) {
        const TString textStr = TString{text.GetString()};
        const char* currentStringPosition = textStr.begin();
        const char* lastStringPosition = textStr.end();
        while (currentStringPosition < lastStringPosition) {
            const char* delimPosition = splitFinder.FindFirstOf(currentStringPosition, lastStringPosition);
            size_t wordSize = 0;
            if (GetNumberOfUTF8Chars(currentStringPosition, delimPosition - currentStringPosition, wordSize)) {
                if (wordSize >= wordLength) {
                    ++countLongWords;
                    if (countLongWords >= enoughWords) {
                        return true;
                    }
                }
            }
            currentStringPosition = delimPosition + 1;
        }
    }

    return false;
}

bool TOcrVoice::IsObscene(const TString& word, const TImageWhatIsThisApplyContext& ctx) const {
    TString lowerWord = ToLowerUTF8(word);
    const THashSet<TString>& swearWords = ctx.GetResources().GetSwearWords();
    return swearWords.contains(lowerWord);
}

void TOcrVoice::AddButtons(TImageWhatIsThisApplyContext& ctx) const {
    ctx.AddOpenUriButton("open_ocr", ctx.GenerateImagesSearchUrl("ocr", TStringBuf("imageocr"), /* disable ptr */ true));
    ctx.AddRepeatButton("alice.image_what_is_this_ocr_voice", "ocr_voice_repeat");
    // TODO: Passthrough silent_mode from camera
    ctx.AddButton("alice.image_what_is_this_ocr_voice", "ocr_voice_next", {{"silent_mode", "1"}});
}
