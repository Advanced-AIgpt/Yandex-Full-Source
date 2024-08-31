#pragma once

#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/tts.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/vector.h>

namespace NAlice::NCuttlefish::NAppHostServices {

struct TTtsBackendRequestInfo {
    TString ItemType_;
    NTts::TBackendRequest Request_;
};

struct TSplitBySpeakersTagsResult {
    TVector<NTts::TAudioPartToGenerate> AudioParts_;
    TMaybe<TString> BackgroundAudioPathForS3_;
};

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/backends_tts/ttsutils.py?rev=r7920271#L90
TSplitBySpeakersTagsResult SplitTextBySpeakerTags(
    const NTts::TRequest& ttsRequest,
    const NAliceProtocol::TAudioOptions& audioOptions,
    const NAliceProtocol::TVoiceOptions& voiceOptions,
    const TString& cacheKeyPrefix,
    bool allowWhisper
);

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/backends_tts/ttsutils.py?rev=r7929449#L123
TString GetCacheKey(
    const TString& cacheKeyPrefix,
    const NTts::TAudioPartToGenerate& audioPartToGenerate
);

TTtsBackendRequestInfo CreateTtsBackendRequestInfo(
    const NTts::TAudioPartToGenerate& audioPart,
    const TString& sessionId,
    const TString& requestId,
    bool enableTtsBackendTimings,
    bool doNotLogTexts,
    ui32 reqSeqNo,
    TString surface
);

}  // namespace NAlice::NCuttlefish::NAppHostServices
