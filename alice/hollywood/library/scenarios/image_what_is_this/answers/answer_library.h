#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>
#include <web/src_setup/lib/setup/images_cbir_postprocess/intents_classifier/intents/intents.h>

namespace NAlice::NHollywood::NImage::NAnswers {

class IAnswer;

using TAnswerList = TVector<IAnswer*>;

class TAnswerLibrary {
public:
    static void Init();
    static void RegisterAnswer(IAnswer* answer);
    static IAnswer* GetAnswerByName(const TStringBuf answerName);
    static IAnswer* GetAnswerByNameOrDefault(const TStringBuf answerName);
    static IAnswer* GetAnswerByCaptureMode(ECaptureMode captureMode);
    static IAnswer* GetAnswerByIntent(NImages::NCbir::ECbirIntents intent);
    static IAnswer* GetDefaultAnswer();
    static const TAnswerList& GetStubAnswers();

private:
    static THashMap<TStringBuf, IAnswer*> NameToAnswer;
    static THashMap<NImages::NCbir::ECbirIntents, IAnswer*> IntentToAnswer;
    static THashMap<ECaptureMode, IAnswer*> CaptureModeToAnswer;
    static TAnswerList StubAnswers;
};

}
