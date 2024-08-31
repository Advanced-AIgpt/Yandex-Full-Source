#include <alice/hollywood/library/scenarios/image_what_is_this/utils.h>

#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/string/split.h>
#include <util/string/join.h>
#include <util/charset/utf8.h>
#include <util/charset/wide.h>

#include <dict/dictutil/scripts.h>

#include <library/cpp/string_utils/url/url.h>
#include <library/cpp/uri/uri.h>

using ECaptureMode = NAlice::NScenarios::TInput::TImage::ECaptureMode;
using TAnalyticsAction = NAlice::NScenarios::TAnalyticsInfo::TAction;

namespace {
    const THashMap<TString, TString> CameraModeToDescription = {
        {"doc_scanner", "сканирования документов"},
        {"translate", "перевода"},
        {"smartcamera", "умной камеры"},
        {"poetry", "поэзии"},
        {"homework", "гдз"},
        {"gdz", "гдз"},
        {"voice_text", "озвучки текста"},
        {"text", "распознавания текста"},
    };
}


namespace NAlice::NHollywood::NImage {

TStringBuf CleanUrl(const TStringBuf& url) {
    TStringBuf cleanedUrl = CutSchemePrefix(url);
    const size_t wwwLen = 4;
    if (cleanedUrl.StartsWith("www.")) {
        cleanedUrl = cleanedUrl.Tail(wwwLen);
    }

    return cleanedUrl;
}

TString GetHostname(const TStringBuf& url) {
    NUri::TUri uri;
    if (uri.Parse(url, NUri::TUri::FeaturesRecommended) == NUri::TState::EParsed::ParsedOK) {
        // TODO: Should we manually convert from idna?
        return TString{uri.GetField(NUri::TUri::TField::FieldHost)};
    }

    return "";
}

TString CutText(const TString& text, size_t len) {
    TVector<TString> words;
    Split(text, " ", words);
    TString cuttedText;
    size_t currentLen = 0;
    auto endWord = words.begin();
    for ( ; endWord != words.end(); ++endWord) {
        size_t wordLen = GetNumberOfUTF8Chars(*endWord);
        if (currentLen + wordLen <= len) {
            if (currentLen != 0) {
                currentLen += 1;
            }
            currentLen += wordLen;
        } else {
            break;
        }
    }
    cuttedText = JoinRange(" ", words.begin(), endWord);

    return cuttedText;
}

TAnalyticsAction CaptureModeToAnalyticsAction(const TStringBuf cameraMode, bool isStartImageRecognizer) {
    TString cameraModeIdAction = isStartImageRecognizer ? "start_image_recognizer" : "open_uri";
    cameraModeIdAction += "_" + ToString(cameraMode);

    TString cameraModeNameAction = isStartImageRecognizer ? "start image recognizer" : "open uri";
    cameraModeNameAction += " " + ToString(cameraMode);

    TString cameraModeDescriptionAction = "Открываем камеру";
    if (CameraModeToDescription.contains(cameraMode)) {
        cameraModeDescriptionAction += " в режиме " + CameraModeToDescription.at(cameraMode);
    }

    TAnalyticsAction action;
    action.SetId(cameraModeIdAction);
    action.SetName(cameraModeNameAction);
    action.SetHumanReadable(cameraModeDescriptionAction);
    return action;
}

TString CaptureModeToString(ECaptureMode captureMode) {
    switch(captureMode) {
        case ECaptureMode::TInput_TImage_ECaptureMode_OcrVoice:
            return "voice_text";
        case ECaptureMode::TInput_TImage_ECaptureMode_Ocr:
            return "text";
        case ECaptureMode::TInput_TImage_ECaptureMode_Photo:
            return "photo";
        case ECaptureMode::TInput_TImage_ECaptureMode_Market:
            return "market";
        case ECaptureMode::TInput_TImage_ECaptureMode_Document:
            return "document";
        case ECaptureMode::TInput_TImage_ECaptureMode_Clothes:
            return "clothes";
        case ECaptureMode::TInput_TImage_ECaptureMode_Details:
            return "details";
        case ECaptureMode::TInput_TImage_ECaptureMode_SimilarLike:
            return "similar_like";
        case ECaptureMode::TInput_TImage_ECaptureMode_SimilarPeople:
            return "similar_people";
        case ECaptureMode::TInput_TImage_ECaptureMode_SimilarPeopleFrontal:
            return "similar_people_frontal";
        case ECaptureMode::TInput_TImage_ECaptureMode_Barcode:
            return "barcode";
        case ECaptureMode::TInput_TImage_ECaptureMode_Translate:
            return "text";
        case ECaptureMode::TInput_TImage_ECaptureMode_SimilarArtwork:
            return "similar_artwork";
        case ECaptureMode::TInput_TImage_ECaptureMode_SmartCamera:
            return "photo";
        default:
            return "photo";
    }
}

bool IsCyrillic(const TStringBuf text) {
    const TUtf16String textWide = UTF8ToWide(text);
    const TScriptMask scriptMask = ClassifyScript(textWide.data(), textWide.size());
    return scriptMask == TScriptMask::CYRILLIC;
};


}
