#include "tts_generate_response.h"

#include <util/generic/singleton.h>
#include <util/string/ascii.h>
#include <util/system/byteorder.h>
#include <util/system/hostname.h>

#include <vector>

using namespace NAlice::NTts;
using namespace NJson;
using namespace NAlice::NCuttlefish::NAppHostServices;

void NAlice::NCuttlefish::NAppHostServices::TtsGenerateResponseTimingsToJson(const TTS::GenerateResponse::Timings& timings, bool fromCache, NJson::TJsonValue& payload) {
    payload = NJson::JSON_MAP;

    payload["from_cache"] = fromCache;

    if (timings.timings().size()) {
        auto& jTimings = payload.InsertValue(TStringBuf("timings"), NJson::JSON_ARRAY);
        for (auto &t : timings.timings()) {
            auto& jT = jTimings.AppendValue(NJson::JSON_MAP);
            jT[TStringBuf("time")] = t.time();
            jT[TStringBuf("phoneme")] = t.phoneme();
            if (t.has_word()) {
                jT[TStringBuf("word")] = t.word();
            }
        }
    }

    if (timings.has_utterance()) {
        auto& utterance = timings.utterance();
        auto& jUtterance = payload.InsertValue(TStringBuf("utterance"), NJson::JSON_ARRAY);
        if (utterance.words().size()) {
            auto& jWords = jUtterance.InsertValue(TString("words"), NJson::JSON_ARRAY);
            for (auto& w : utterance.words()) {
                auto& jw = jWords.AppendValue(NJson::JSON_MAP);
                jw[TStringBuf("word")] = w.word();
                auto& phonemes = w.phonemes();
                if (phonemes.size()) {
                    auto& jPhonemes = jw.InsertValue(TStringBuf("phonemes"), NJson::JSON_ARRAY);
                    for (auto& p : phonemes) {
                        jPhonemes.AppendValue(p);
                    }
                }
            }
        }
    }

}
