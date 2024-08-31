#pragma once

#include "session.h"
#include "status.h"

#include <alice/joker/library/log/log.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/http/io/stream.h>
#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/uri/uri.h>

#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>
#include <util/string/builder.h>

namespace NAlice::NJokerLight {

class TConfig;

class IHttpContext : NNonCopyable::TNonCopyable {
public:
    using THeader = THttpInputHeader;
    using THeaders = TVector<THeader>;

public:
    virtual ~IHttpContext() = default;

    virtual const NUri::TUri& Uri() const = 0;
    virtual const NUri::TUri& ForwardUri() const = 0;
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

    const NUri::TUri& ForwardUri() const override {
        return ForwardUri_;
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

    const TMaybe<TString>& HostHeader() const {
        return HostHeader_;
    }

    const TMaybe<TString>& ProxyHeader() const {
        return ProxyHeader_;
    }

    TString UriAsString() const {
        return Uri_.PrintS();
    }

    TString ForwardUriAsString() const {
        TStringBuilder uri = TStringBuilder() << ForwardUri_.PrintS();
        if (!IsCgiReadFromBody_ && !Cgi.empty()) {
            if (!uri.EndsWith("?")) {
                uri << "?";
            }
            uri << Cgi.Print();
        }
        return uri;
    }

    TString ForwardUriHostPort() const {
        return ForwardUri_.PrintS(NUri::TField::EFlags::FlagHostPort);
    }

    TString CreateRequestHash() const;

    TString CreateHeadersDescription() const;

private:
    bool IsCgiReadFromBody_;
    TMaybe<TString> JokerHeader_;
    TMaybe<TString> HostHeader_;
    TMaybe<TString> ProxyHeader_;
    const TParsedHttpFull Http_;
    const TVector<THeader> Headers_;
    const TString Body_;
    const NUri::TUri Uri_;
    const NUri::TUri ForwardUri_;

public:
    const TString& FirstLine;
    const TCgiParameters Cgi;
    THttpOutput& Output;
};

bool IsServiceHeader(const TString& headerName);

} // namespace NAlice::NJokerLight
