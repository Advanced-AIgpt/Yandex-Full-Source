#pragma once

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice::NMegamind {

class TRTLoggerTokenConstructor {
public:
    TRTLoggerTokenConstructor(const TString& ruid);

    /** Check if value is valid v2 reqid and set.
     * Must be '(ui64)$(.+)$(.+)'
     */
    void SetAppHostReqId(TStringBuf value);

    /** Check if the given header is valid v1 or v2.
     * Set for future building token.
     */
    bool CheckHeader(TStringBuf name, TStringBuf value);

    TString GetToken() const;

private:
    TString Ruid_;
    TMaybe<TStringBuf> TokenV1_;
    TMaybe<TStringBuf> TokenV2_;
};

} // namespace NAlice::NMegamind
