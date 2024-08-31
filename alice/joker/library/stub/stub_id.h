#pragma once

#include <alice/joker/library/status/status.h>

#include <library/cpp/cgiparam/cgiparam.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice::NJoker {

class TStubId {
public:
    static TStatus ValidateProjectId(TString src, TString& dst);
    static TStatus ValidateTestId(TString src, TString& dst);

    static TErrorOr<TStubId> Load(const TCgiParameters& cgi, TString reqHash);
    static TErrorOr<TStubId> Create(TStringBuf ident, TString* version = nullptr);

public:
    TStubId(TString prj, TString parentId, TString reqHash);

    const TString& ProjectId() const {
        return ProjectId_;
    }

    const TString& ParentId() const {
        return ParentId_;
    }

    const TString& ReqHash() const {
        return ReqHash_;
    }

    TString MakeKey(const TString* version) const;
    TString MakePath(const TString* version, const TString& suffix) const;

private:
    TString ProjectId_;
    TString ParentId_;
    TString ReqHash_;
};

} // namespace NAlice::NJoker
