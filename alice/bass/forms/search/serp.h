#pragma once

#include <alice/bass/libs/client/client_info.h>
#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/source_request/source_request.h>

#include <alice/bass/util/error.h>

#include <alice/library/websearch/websearch.h>

#include <library/cpp/neh/neh.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS {

class TContext;


namespace NSerpSnippets {

// области поиска сниппета в документе
enum ESnippetSection {
    ESS_NONE = 0,
    ESS_SNIPPETS_PRE = 1,
    ESS_SNIPPETS_MAIN = 2,
    ESS_SNIPPETS_POST = 4,
    ESS_SNIPPETS_FULL = 8,
    ESS_CONSTRUCT = 16,
    ESS_FLAT = 32,
    ESS_SNIPPETS_ALL = ESS_SNIPPETS_PRE | ESS_SNIPPETS_MAIN | ESS_SNIPPETS_POST | ESS_SNIPPETS_FULL,
    ESS_ALL = ESS_SNIPPETS_ALL | ESS_CONSTRUCT | ESS_FLAT
};

inline ESnippetSection operator | (const ESnippetSection& s1, const ESnippetSection& s2) {
    return static_cast<ESnippetSection>(static_cast<int>(s1) | static_cast<int>(s2));
}

const NSc::TValue& FindSnippet(NSc::TValue& doc, TStringBuf snippetType, ESnippetSection section);
const NSc::TValue& FindAnySnippet(NSc::TValue& doc, const TVector<TStringBuf>& snippetTypes, ESnippetSection section);
const NSc::TValue& FindFirstSnippet(const NSc::TValue& doc, ESnippetSection section);
TVector<const NSc::TValue*> FindSnippets(const NSc::TValue& doc, ESnippetSection section);
TVector<const NSc::TValue*> FindSnippets(const NSc::TValue& doc, TStringBuf snippetType, ESnippetSection section);

TString RemoveHiLight(TStringBuf str);
TString JoinListFact(const TString& text, const TVector<TString>& items, bool isOrdered, bool isTts = false);
} // namespace NSerpSnippets


namespace NSerp {

namespace NImpl {
ui64 IncSearchCounter(TContext& ctx, NAlice::TWebSearchBuilder::EService service);
} // namespace NImpl

enum class ESearchPlatform {
    DEFAULT,
    TOUCH,
    DESKTOP
};

inline constexpr TStringBuf SUMMARIZATION = "summarization";

struct TSearchFeatures {
    ESearchPlatform Platform = ESearchPlatform::DEFAULT;
};

bool IsTouchSearch(const NAlice::TClientInfo& clientInfo,
                   const TSearchFeatures& searchFeatures = Default<TSearchFeatures>());

TResultValue ParseSearchResponse(NHttpFetcher::TResponse::TConstRef response, NSc::TValue* result);

TResultValue MakeRequest(TStringBuf query, TContext& context, const TCgiParameters& cgi, NSc::TValue* result, NAlice::TWebSearchBuilder::EService service);

NHttpFetcher::TRequestPtr PrepareMusicSearchRequest(
    TStringBuf query,
    TContext& context,
    const TCgiParameters& cgi,
    NHttpFetcher::IMultiRequest::TRef multiRequest,
    TString& encodedAliceMeta
);

NHttpFetcher::TRequestPtr PrepareSearchRequest(
    TStringBuf query,
    TContext& context,
    const TCgiParameters& cgi,
    NAlice::TWebSearchBuilder::EService service,
    NHttpFetcher::IMultiRequest::TRef multiRequest = nullptr
);

NHttpFetcher::TRequestPtr PrepareSearchRequest(
    TStringBuf query,
    TContext& context,
    const TCgiParameters& cgi,
    const TString& internalFlags,
    const TVector<TString>& upperSrcParams,
    NAlice::TWebSearchBuilder::EService service,
    NHttpFetcher::IMultiRequest::TRef multiRequest = nullptr);

inline void AddTemplateDataCgi(TCgiParameters& cgi) {
    cgi.InsertUnescaped(TStringBuf("flags"), TStringBuf("alice_full_tmpl_data"));
}

inline bool IsPornoSerp(const NSc::TValue& searchResult) {
    return searchResult.TrySelect("/search/is_porno_query").GetNumber() == 1;
}

inline bool IsInfectedDoc(NSc::TValue& doc) {
    return !NSerpSnippets::FindSnippet(doc, TStringBuf("infected"), NSerpSnippets::ESS_SNIPPETS_FULL).IsNull();
}

const NSc::TValue& GetVoiceInfo(const NSc::TValue& snippet, TStringBuf tld);
NSc::TValue GetFilteredVoiceInfo(const NSc::TValue& snippet, TStringBuf tld, const TContext& ctx);
TStringBuf GetVoiceTTS(const NSc::TValue& snippet, TStringBuf tld);

TStringBuf GetHostName(const NSc::TValue& snippet);


class TSnippetIterator {
public:
    using TPredicate = std::function<bool(const NSc::TValue&)>;

public:
    TSnippetIterator(const NSc::TValue& result, TPredicate predicate, TStringBuf key = "searchdata")
        : Docs(result[key]["docs"].GetArray())
        , Predicate(predicate)
        , DocIt(Docs.cbegin())
        , CurSnippet(nullptr)
    {
        Next();
    }

    operator bool() const {
        return !!CurSnippet;
    }

    const NSc::TValue& operator*() const {
        return *CurSnippet;
    }

    const NSc::TValue* operator->() const {
        return CurSnippet;
    }

    bool operator++() {
        return Next();
    }

    bool Next();

private:
    const NSc::TArray& Docs;
    TPredicate Predicate;
    NSc::TArray::const_iterator DocIt;
    const NSc::TValue* CurSnippet;
};

} // namespace NSerp
} // namespace NBASS
