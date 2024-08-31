#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer_library.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/tag.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/ocr.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/similars.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/entity.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/common.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/clothes.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/dark.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/porn.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/gruesome.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/barcode.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/face.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/similar_people.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/museum.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/similar_artwork.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/translate.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/ocr_voice.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/market.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/office_lens.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/office_lens_disk.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/similarlike.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/info.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/smart_camera.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/poetry.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/homework.h>

#include <util/generic/hash.h>


using namespace NAlice::NHollywood::NImage;
using namespace NAlice::NHollywood::NImage::NAnswers;

THashMap<TStringBuf, IAnswer*> TAnswerLibrary::NameToAnswer = THashMap<TStringBuf, IAnswer*>();
THashMap<NImages::NCbir::ECbirIntents, IAnswer*> TAnswerLibrary::IntentToAnswer
                                                                = THashMap<NImages::NCbir::ECbirIntents, IAnswer*>();
THashMap<ECaptureMode, IAnswer*> TAnswerLibrary::CaptureModeToAnswer
                                                                = THashMap<ECaptureMode, IAnswer*>();
TAnswerList TAnswerLibrary::StubAnswers = {
        TDark::GetPtr(),
        TGruesome::GetPtr(),
        TPorn::GetPtr(),
        TBarcode::GetPtr(),
        TMuseum::GetPtr(),
};

void TAnswerLibrary::Init() {
    // TODO: Should we use smart pointers?
    RegisterAnswer(TTag::GetPtr());
    RegisterAnswer(TOcr::GetPtr());
    RegisterAnswer(TEntity::GetPtr());
    RegisterAnswer(TSimilars::GetPtr());
    RegisterAnswer(TClothes::GetPtr());
    RegisterAnswer(TFace::GetPtr());
    RegisterAnswer(TBarcode::GetPtr());
    RegisterAnswer(TTranslate::GetPtr());
    RegisterAnswer(TOcrVoice::GetPtr());
    RegisterAnswer(TMarket::GetPtr());
    RegisterAnswer(TSimilarArtwork::GetPtr());
    RegisterAnswer(TSimilarPeople::GetPtr());
    RegisterAnswer(TSimilarPeopleFrontal::GetPtr());
    RegisterAnswer(TOfficeLens::GetPtr());
    RegisterAnswer(TOfficeLensDisk::GetPtr());
    RegisterAnswer(TSimilarLike::GetPtr());
    RegisterAnswer(TInfo::GetPtr());
    RegisterAnswer(TCommon::GetPtr());
    RegisterAnswer(TSmartCamera::GetPtr());
    RegisterAnswer(TPoetry::GetPtr());
    RegisterAnswer(THomeWork::GetPtr());
}

void TAnswerLibrary::RegisterAnswer(IAnswer* answer) {
    NameToAnswer[answer->GetAnswerName()] = answer;

    TMaybe<ECaptureMode> answerCaptureMode = answer->GetCaptureMode();
    if (answerCaptureMode.Defined()) {
        CaptureModeToAnswer[answerCaptureMode.GetRef()] = answer;
    }

    TMaybe<NImages::NCbir::ECbirIntents> answerIntent = answer->GetIntent();
    if (answerIntent.Defined()) {
        IntentToAnswer[answerIntent.GetRef()] = answer;
    }
}

IAnswer* TAnswerLibrary::GetAnswerByName(const TStringBuf answerName) {
    THashMap<TStringBuf, IAnswer*>::const_iterator iter = NameToAnswer.find(answerName);
    if (iter == NameToAnswer.end()) {
        return nullptr;
    }
    return iter->second;
}

IAnswer* TAnswerLibrary::GetAnswerByNameOrDefault(const TStringBuf answerName) {
    IAnswer* answer = GetAnswerByName(answerName);
    if (answer) {
        return answer;
    }

    return GetDefaultAnswer();
}

IAnswer* TAnswerLibrary::GetAnswerByCaptureMode(ECaptureMode captureMode) {
    THashMap<ECaptureMode, IAnswer*>::const_iterator iter = CaptureModeToAnswer.find(captureMode);
    if (iter == CaptureModeToAnswer.end()) {
        return nullptr;
    }
    return iter->second;
}

IAnswer* TAnswerLibrary::GetAnswerByIntent(NImages::NCbir::ECbirIntents intent) {
    THashMap<NImages::NCbir::ECbirIntents, IAnswer*>::const_iterator iter = IntentToAnswer.find(intent);
    if (iter == IntentToAnswer.end()) {
        return nullptr;
    }
    return iter->second;
}

IAnswer* TAnswerLibrary::GetDefaultAnswer() {
    return TCommon::GetPtr();
}

const TAnswerList& TAnswerLibrary::GetStubAnswers() {
    return StubAnswers;
}
