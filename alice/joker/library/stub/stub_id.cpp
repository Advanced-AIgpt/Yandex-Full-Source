#include "stub_id.h"

#include <util/generic/serialized_enum.h>
#include <util/string/builder.h>
#include <util/string/split.h>

namespace NAlice::NJoker {
namespace {

bool ValidateId(TStringBuf id) {
    static const auto onEachChar = [](auto ch) {
        static constexpr TStringBuf validChars = "_-:=.@~[]";
        return std::isalnum(ch) || validChars.find(ch) != TStringBuf::npos;
    };
    return AllOf(id, onEachChar);
}

} // namespace

TStubId::TStubId(TString prj, TString parentId, TString reqHash)
    : ProjectId_{std::move(prj)}
    , ParentId_{std::move(parentId)}
    , ReqHash_{std::move(reqHash)}
{
}

TString TStubId::MakeKey(const TString* version) const {
    TStringBuilder key;
    key << ProjectId_ << '/' << ParentId_ << '/' << ReqHash_;
    if (version) {
        key << '/' << *version;
    }
    return key;
}

TString TStubId::MakePath(const TString* version, const TString& suffix) const {
    return TStringBuilder{} << MakeKey(version) << suffix;
}

// static
TErrorOr<TStubId> TStubId::Create(TStringBuf ident, TString* version) {
    // ident must be: project/test_ident/request_hash
    TMaybe<TString> project;
    TMaybe<TString> test;
    TMaybe<TString> reqHash;
    auto consumer = [&](TStringBuf chunk) {
        if (!project.Defined()) { // FIXME add checkers (Validate*).
            project.ConstructInPlace(chunk);
        } else if (!test.Defined()) {
            test.ConstructInPlace(chunk);
        } else if (!reqHash.Defined()) {
            reqHash.ConstructInPlace(chunk);
        } else if (version) {
            *version = chunk;
        }
    };
    StringSplitter(ident).Split('/').Consume(consumer);

    if (!reqHash.Defined()) {
        return TError{TError::EType::Logic} << "bad stub ident format (must be prj/reqid/reqHash)";
    }

    return TStubId{*project, *test, *reqHash};
}

// static
TErrorOr<TStubId> TStubId::Load(const TCgiParameters& cgi, TString reqHash) {
    //LOG(DEBUG) << "StubIdent: " << cgi.Print() << Endl; FIXME

    // prj=PROJECT&test=TEST_IDENT&sess=SESSION_GUID
    if (cgi.empty()) {
        return TError{HTTP_BAD_REQUEST} << "empty stub request";
    }

    TString projectId;
    // FIXME (petrk) Use session for default value in these functions.
    if (auto error = TStubId::ValidateProjectId(cgi.Get("prj"), projectId)) {
        return *error;
    }

    TString parentId;
    if (auto error = TStubId::ValidateTestId(cgi.Get("test"), parentId)) {
        return *error;
    }

    //LOG(DEBUG) << "StubIdentParsed: " << request->AsString() << Endl; FIXME
    return TStubId(std::move(projectId), std::move(parentId), std::move(reqHash));
}

// static
TStatus TStubId::ValidateProjectId(TString src, TString& dst) {
    if (src.empty()) {
        return TError{HTTP_BAD_REQUEST} << "no project id found";
    }

    if (!ValidateId(src)) {
        return TError{HTTP_BAD_REQUEST} << "'prj' is invalid: " << src;
    }

    dst = std::move(src);

    return Success();
}

// static
TStatus TStubId::ValidateTestId(TString src, TString& dst) {
    if (src.empty()) {
        return TError{HTTP_BAD_REQUEST} << "no test id found";
    }

    if (!ValidateId(src)) {
        return TError{HTTP_BAD_REQUEST} << "test id is invalid: " << src;
    }

    dst = std::move(src);

    return Success();
}

} // namespace NAlice::NJoker
