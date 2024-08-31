#pragma once

#include <alice/megamind/library/config/protos/classification_config.pb.h>
#include <alice/megamind/library/config/protos/config.pb.h>
#include <alice/megamind/library/factor_storage/factor_storage.h>
#include <alice/megamind/library/request/request.h>

#include <util/system/types.h>

namespace NAlice {

TConfig CreateTestConfig(const ui16 port);

TConfig GetRealConfig();
NMegamind::TClassificationConfig GetRealClassificationConfig();

inline TFactorStorage CreateStorage() {
    return NMegamind::CreateFactorStorage(NMegamind::CreateFactorDomain());
}

inline TRequest CreateRequestFromSkr(const TSpeechKitRequest& skr, TMaybe<bool> disableVoiceSession = {},
                                     TMaybe<bool> disableShouldListen = {}) {
    return CreateRequest(IEvent::CreateEvent(skr.Event()), skr,
                         /* iotUserInfo= */  Nothing(),
                         /* requestSource= */ {},
                         /* semanticFrames= */ {},
                         /* recognizedActionEffectFrames= */ {},
                         /* stackEngineCore= */ {},
                         /* parameters= */ {},
                         /* contactsList= */ Nothing(),
                         /* origin= */ Nothing(),
                         /* lastWhisperTimeMs= */ 0,
                         /* whisperTtlMs= */ 0,
                         /* callbackOwnerScenario= */ Nothing(),
                         /* whisperConfig= */ Nothing(),
                         /* logger= */ TRTLogger::NullLogger(),
                         /* isWarmUp= */ false,
                         /* allParsedSemanticFrames= */ {},
                         /* disableVoiceSession= */ disableVoiceSession,
                         /* disableShouldListen= */ disableShouldListen);
}

} // namespace NAlice
