#include "util.h"

#include <contrib/libs/re2/re2/re2.h>
#include <library/cpp/xml/document/xml-document.h>
#include <util/generic/string.h>
#include <util/stream/str.h>
#include <util/string/printf.h>

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

TStringBuf GetMdsFileExtension(const TStringBuf format) {
    if (format.Contains("wav") || format.Contains("pcm")) {
        return "wav";
    } else if (format.Contains("opus")) {
        return "opus";
    } else if (format.Contains("spx") || format.Contains("speex")) {
        return "spx";
    }
    return "ogg";
}

} // namespace

// Old version: https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/backends_tts/ttsutils.py?rev=r7664149#L185-193
ui16 ChannelsFromMime(const TString& mime) {
    static const re2::RE2 re(R"(channels=(\d+))");

    int channels;
    if (re2::RE2::PartialMatch(mime, re, &channels)) {
        return channels;
    }
    return 1;
}

// Old version: https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/extlog/sessionlog.py?rev=7520915#L26-42
TString ConstructMdsFilename(const NAliceProtocol::TRequestContext& requestContext, bool isSpotter) {
    static const TStringBuf pattern = "%s_%s_%d%s.%s";

    const TStringBuf format = requestContext.GetAudioOptions().GetFormat().Contains("pcm") ? "audio/x-wav"sv : requestContext.GetAudioOptions().GetFormat();
    const auto& header = requestContext.GetHeader();
    return Sprintf(
        pattern.data(),
        header.GetSessionId().data(),
        header.GetMessageId().data(),
        header.GetStreamId(),
        isSpotter ? "_spotter" : "",
        GetMdsFileExtension(format).data()
    );
}

TString GetKeyFromMdsSaveResponse(const TString& content) {
    const NXml::TDocument xml(content, NXml::TDocument::String);
    const NXml::TConstNode root = xml.Root();
    return root.Attr<TString>("key");
}

}  // namespace NAlice::NCuttlefish::NAppHostServices

