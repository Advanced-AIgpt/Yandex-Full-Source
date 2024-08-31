#pragma once

#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/tts.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>

// Utils with inherit legacy logic from python_uniproxy
namespace NAlice::NCuttlefish::NTtsUtils {
    // https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/backends_tts/ttsutils.py?rev=r7915116#L9
    // If lang is empty, then return "ru" as default
    // If lang starts with known prefix, then cut everything after prefix
    // Otherwise return Nothing
    TMaybe<TString> TryNormalizeLang(const TString& lang);

    // https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/backends_tts/ttsutils.py?rev=r7915116#L16
    TString FormatToMime(const TString& format, NAliceProtocol::TVoiceOptions::EVoiceQuality quality);

    NAliceProtocol::TVoiceOptions::EVoiceQuality VoiceQualityFromString(const TString& voiceQuality);
    TString VoiceQualityToString(NAliceProtocol::TVoiceOptions::EVoiceQuality voiceQuality);
}
