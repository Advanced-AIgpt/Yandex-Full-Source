#include "rtlogtoken.h"

#include <alice/library/network/headers.h>

#include <util/string/ascii.h>
#include <util/string/builder.h>
#include <util/string/split.h>

namespace NAlice::NMegamind {
namespace {

// TODO (petrk, akhruslan) remove it when uniproxy fixes this bug.
// Temporary crutch to solve problem with python strings
TStringBuf FixBuggyRtLogToken(TStringBuf token) {
    TStringBuf content = token;
    if (content.SkipPrefix(TStringBuf("b'")) && content.ChopSuffix(TStringBuf("'"))) {
        return content;
    }
    return token;
}

bool IsTokenValid(TStringBuf token) {
    const auto items = StringSplitter(token).Split('$').ToList<TStringBuf>();
    ui64 reqTimestamp = 0;
    return items.size() == 3 && TryFromString<ui64>(items[0], reqTimestamp) && !items[1].empty() && !items[2].empty();
}

} // namespace

TRTLoggerTokenConstructor::TRTLoggerTokenConstructor(const TString& ruid)
    : Ruid_{ruid}
{
}

bool TRTLoggerTokenConstructor::CheckHeader(TStringBuf name, TStringBuf value) {
    if (AsciiEqualsIgnoreCase(name, NNetwork::HEADER_X_RTLOG_TOKEN)) {
        // Check rtlog-token from header (for init rtlogger).
        value = FixBuggyRtLogToken(value);
        if (IsTokenValid(value)) {
            TokenV1_ = value;
        }
    } else if (AsciiEqualsIgnoreCase(name, NNetwork::HEADER_X_YANDEX_REQ_ID)) {
        // Check rtlog-token from header (for init rtlogger).
        // this version should only be used for requests via apphost
        SetAppHostReqId(FixBuggyRtLogToken(value));
    }

    return TokenV1_.Defined() && TokenV2_.Defined();
}

void TRTLoggerTokenConstructor::SetAppHostReqId(TStringBuf value) {
    if (IsTokenValid(value)) {
        TokenV2_ = value;
    }
}

TString TRTLoggerTokenConstructor::GetToken() const {
    if (TokenV2_.Defined() && !Ruid_.Empty()) {
        return TStringBuilder{} << *TokenV2_ << '-' << Ruid_;
    }
    return TString{TokenV1_.Defined() ? *TokenV1_ : ""};
}

} // namespace NAlice::NMegamind
