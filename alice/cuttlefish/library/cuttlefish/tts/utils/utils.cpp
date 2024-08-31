#include "utils.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <util/charset/utf8.h>

#include <util/generic/hash_set.h>

using namespace NAlice::NCuttlefish::NTtsUtils;

TMaybe<TString> NAlice::NCuttlefish::NTtsUtils::TryNormalizeLang(const TString& lang) {
    static const TString langRu = "ru";
    static const TString langEn = "en";
    static const TString langTr = "tr";
    static const TString langUk = "uk";
    static const TString langAr = "ar";

    if (lang.empty()) {
        // Default language
        return langRu;
    } else if (lang.StartsWith(langRu)) {
        return langRu;
    } else if (lang.StartsWith(langEn)) {
        return langEn;
    } else if (lang.StartsWith(langTr)) {
        return langTr;
    } else if (lang.StartsWith(langUk)) {
        return langUk;
    } else if (lang.StartsWith(langAr)) {
        return langAr;
    } else {
        // Unknown language (failed to normalize)
        return Nothing();
    }
}

TString NAlice::NCuttlefish::NTtsUtils::FormatToMime(const TString& format, NAliceProtocol::TVoiceOptions::EVoiceQuality quality) {
    static const TString mimeOpus = "audio/opus";
    static const TString mimeSpeex = "audio/x-speex";
    static const TString mimeWav = "audio/x-wav";
    static const TString mimePcm8K = "audio/x-pcm;bit=16;rate=8000";
    static const TString mimePcm16K = "audio/x-pcm;bit=16;rate=16000";
    static const TString mimePcm48K = "audio/x-pcm;bit=16;rate=48000";
    static const THashSet<TString> allAllowedMimes = {
        mimeOpus,
        mimeSpeex,
        mimeWav,
        mimePcm8K,
        mimePcm16K,
        mimePcm48K,
    };

    static const TString defaultMime = mimeOpus;

    // Python uniproxy legacy logic
    // Format can be mime already
    if (allAllowedMimes.contains(format)) {
        return format;
    }

    if (format) {
        if (const auto lowerFormat = to_lower(format); lowerFormat == "pcm") {
            switch (quality) {
                case NAliceProtocol::TVoiceOptions::LOW:
                    return mimePcm8K;
                case NAliceProtocol::TVoiceOptions::ULTRAHIGH:
                    return mimePcm48K;
                case NAliceProtocol::TVoiceOptions::HIGH:
                case NAliceProtocol::TVoiceOptions::DEFAULT_QUALITY:
                    return mimePcm16K;
                default:
                    return mimePcm16K;
            }
        } else if (lowerFormat == "opus") {
            return mimeOpus;
        } else if (lowerFormat == "spx") {
            return mimeSpeex;
        } else if (lowerFormat == "wav") {
            return mimeWav;
        }
    }

    return defaultMime;
}

NAliceProtocol::TVoiceOptions::EVoiceQuality NAlice::NCuttlefish::NTtsUtils::VoiceQualityFromString(const TString& voiceQuality) {
    // Copied as is from converter
    // TODO(chegoryu) Do not copypaste this
    NAliceProtocol::TVoiceOptions::EVoiceQuality val;
    if (NAliceProtocol::TVoiceOptions::EVoiceQuality_Parse(ToUpperUTF8(voiceQuality), &val)) {
        return val;
    }
    return NAliceProtocol::TVoiceOptions::ULTRAHIGH;
}

TString NAlice::NCuttlefish::NTtsUtils::VoiceQualityToString(NAliceProtocol::TVoiceOptions::EVoiceQuality voiceQuality) {
    switch (voiceQuality) {
        case NAliceProtocol::TVoiceOptions::LOW:
            return "Low";
        case NAliceProtocol::TVoiceOptions::HIGH:
            return "High";
        case NAliceProtocol::TVoiceOptions::DEFAULT_QUALITY:
        case NAliceProtocol::TVoiceOptions::ULTRAHIGH:
            return "UltraHigh";
        default:
            return "UltraHigh";
    }
}
