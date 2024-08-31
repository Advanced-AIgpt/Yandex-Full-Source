#include "utils.h"

#include <alice/library/json/json.h>

#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

#include <library/cpp/resource/resource.h>

#include <util/charset/wide.h>
#include <util/string/cast.h>
#include <util/string/subst.h>


namespace NAlice::NIot {

namespace {

/* Use the tool alice/bass/tools/cache_normalized to update this file. */
const NSc::TValue NORMALIZED_NAMES = NSc::TValue::FromJsonThrow(NResource::Find("normalized_names.json"));

TString Lower(TStringBuf text) {
    auto wideText = UTF8ToWide(text);
    wideText.to_lower();
    return WideToUTF8(wideText);
}

}

TString NormalizeWithFST(TStringBuf text, ELanguage language) {
    return NNlu::TRequestNormalizer::Normalize(language, ToString(text));
}

TUtf16String PostNormalize(TStringBuf text, ELanguage language) {
    auto result = UTF8ToWide(text);
    if (language == LANG_RUS) {
        SubstGlobal(result, u"%" /* what */, u"процент" /* with */);
        SubstGlobal(result, u"°" /* what */, u"градус" /* with */);
    }
    return result;
}

TString Normalize(TStringBuf text, ELanguage language) {
    const auto& cached = NORMALIZED_NAMES[Lower(text)];
    if (cached.IsString()) {
        return ToString(cached.GetString());
    }

    return WideToUTF8(PostNormalize(NormalizeWithFST(text, language), language));
}

TMaybe<TNluInput> NluInputFromJson(const NSc::TValue& jsonTokens) {
    if (jsonTokens.ArrayEmpty()) {
        return Nothing();
    }

    TVector<TString> tokens;
    TVector<double> nonsenseProbabilities;
    for (const auto& token : jsonTokens.GetArray()) {
        const auto& text = token.TrySelect("Text");
        const auto& nonsenseProb = token.TrySelect("NonsenseProb");
        if (!text.IsString() || text.GetString().Empty() || !nonsenseProb.IsNumber()) {
            return Nothing();
        }

        tokens.emplace_back(text.GetString());
        nonsenseProbabilities.push_back(nonsenseProb.GetNumber());
    }
    return TNluInput(std::move(tokens), std::move(nonsenseProbabilities));
}

bool IsSubType(TStringBuf type, TStringBuf subtype) {
    return type.StartsWith(subtype) && (type.size() == subtype.size() || type[subtype.size()] == '.');
}

bool IsEmpty(const TIoTUserInfo& iotUserInfo) {
    return iotUserInfo.GetDevices().empty() && iotUserInfo.GetScenarios().empty();
}

NSc::TValue LoadAndParseResource(const TStringBuf name) {
    NSc::TValue result;
    if (!NSc::TValue::FromJson(result, NResource::Find(name))) {
        ythrow yexception() << "Can't parse " << name << Endl;
    }
    return result;
}

NAlice::TIoTUserInfo IoTFromIoTValue(const NSc::TValue& iotValue) {
    NAlice::TIoTUserInfo ioTUserInfo;
    google::protobuf::util::JsonStringToMessage(iotValue.ToJson(), &ioTUserInfo);

    return ioTUserInfo;
}

} // namespace NAlice::NIot
