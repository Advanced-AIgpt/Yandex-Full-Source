#pragma once

#include "session.h"

#include <alice/joker/library/log/log.h>
#include <alice/joker/library/status/status.h>

#include <library/cpp/http/io/stream.h>
#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/uri/uri.h>

#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NAlice::NJoker {

class TConfig;

class IHttpContext : NNonCopyable::TNonCopyable {
public:
    using THeader = THttpInputHeader;
    using THeaders = TVector<THeader>;

public:
    virtual ~IHttpContext() = default;

    virtual const NUri::TUri& Uri() const = 0;
    virtual const THeaders& Headers() const = 0;
    virtual const TString& Body() const = 0;

    static TCgiParameters ConstructCgi(const THeaders& headers, const NUri::TUri& uri, const TString& body, bool& isCgiReadFromBody);
};

class THttpContext : public IHttpContext {
public:
    THttpContext(THttpInput& httpInput, THttpOutput& httpOutput);
    ~THttpContext();

    const TParsedHttpFull& Http() const {
        return Http_;
    }

    const NUri::TUri& Uri() const override {
        return Uri_;
    }

    // Sorted; Without internal joker headers
    const THeaders& Headers() const override {
        return Headers_;
    }

    const TString& Body() const override {
        return Body_;
    }

    const TMaybe<TString>& JokerHeader() const {
        return JokerHeader_;
    }

    const TMaybe<TString>& ViaProxy() const {
        return ViaProxy_;
    }

    TString UriAsString() const {
        return Uri_.PrintS();
    }

    TString CreateRequestHash(const TConfig& config) const;

    static void ErrorResponse(THttpOutput& output, const TError& error);

private:
    bool IsCgiReadFromBody_;
    TMaybe<TString> JokerHeader_;
    TMaybe<TString> ViaProxy_;
    const TParsedHttpFull Http_;
    const TVector<THeader> Headers_;
    const TString Body_;
    const NUri::TUri Uri_;

public:
    const TString& FirstLine;
    const TCgiParameters Cgi;
    THttpOutput& Output;
};

} // namespace NAlice::NJoker
