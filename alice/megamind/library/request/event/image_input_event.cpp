#include "image_input_event.h"

#include <google/protobuf/struct.pb.h>
#include <util/generic/hash.h>
#include <util/generic/mapfindptr.h>

namespace NAlice {

namespace {

TString ExtractImageUrl(const TEvent& event) {
    const auto& fields = event.GetPayload().fields();
    const auto it = fields.find("img_url");
    if (it == fields.end()) {
        ythrow IEvent::TInvalidEvent{} << "no image url";
    }
    return it->second.string_value();
}

TString ExtractCropCoordinates(const TEvent& event) {
    const auto& fields = event.GetPayload().fields();
    const auto iter = fields.find("crop_coordinates");
    return iter != fields.end() ? iter->second.string_value() : "";
}

using EImageCaptureMode = NScenarios::TInput::TImage::ECaptureMode;

static const THashMap<TString, EImageCaptureMode> ImageCaptureModes = {
    {"voice_text", EImageCaptureMode::TInput_TImage_ECaptureMode_OcrVoice},
    {"text", EImageCaptureMode::TInput_TImage_ECaptureMode_Ocr},
    {"photo", EImageCaptureMode::TInput_TImage_ECaptureMode_Photo},
    {"market", EImageCaptureMode::TInput_TImage_ECaptureMode_Market},
    {"document", EImageCaptureMode::TInput_TImage_ECaptureMode_Document},
    {"clothes", EImageCaptureMode::TInput_TImage_ECaptureMode_Clothes},
    {"details", EImageCaptureMode::TInput_TImage_ECaptureMode_Details},
    {"similar_like", EImageCaptureMode::TInput_TImage_ECaptureMode_SimilarLike},
    {"similar_people", EImageCaptureMode::TInput_TImage_ECaptureMode_SimilarPeople},
    {"similar_people_frontal", EImageCaptureMode::TInput_TImage_ECaptureMode_SimilarPeopleFrontal},
    {"barcode", EImageCaptureMode::TInput_TImage_ECaptureMode_Barcode},
    {"translate", EImageCaptureMode::TInput_TImage_ECaptureMode_Translate},
    {"similar_artwork", EImageCaptureMode::TInput_TImage_ECaptureMode_SimilarArtwork}
};

EImageCaptureMode ExtractImageCaptureMode(const TEvent& event) {
    if (const auto* captureMode = MapFindPtr(event.GetPayload().fields(), "capture_mode")) {
        if (const auto* mode = MapFindPtr(ImageCaptureModes, captureMode->string_value())) {
            return *mode;
        }
    }
    return EImageCaptureMode::TInput_TImage_ECaptureMode_Undefined;
}

} // namespace

void TImageInputEvent::FillScenarioInput(const TMaybe<TString>& /* normalizedUtterance */, NScenarios::TInput* input) const {
    input->MutableImage()->SetUrl(ExtractImageUrl(SpeechKitEvent()));
    input->MutableImage()->SetCaptureMode(ExtractImageCaptureMode(SpeechKitEvent()));
    input->MutableImage()->SetCropCoordinates(ExtractCropCoordinates(SpeechKitEvent()));
}

} // namespace NAlice
