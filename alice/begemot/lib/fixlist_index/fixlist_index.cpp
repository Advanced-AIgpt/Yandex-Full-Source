#include "fixlist_index.h"

#include <library/cpp/regex/pire/pire.h>
#include <library/cpp/regex/pire/pcre2pire.h>
#include <library/cpp/yaml/as/tstring.h>

#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/stream/file.h>
#include <util/string/builder.h>

namespace NBg {
namespace {

constexpr ui32 REGEX_MAX_COMPLEX = 1000;

class TAndMatcher : public TFixlistIndex::IMatcher {
public:
    explicit TAndMatcher(TVector<Ptr>& matchers)
        : Matchers(std::move(matchers))
    {
    }

    bool Match(const TFixlistIndex::TQuery& query) const override {
        return std::all_of(Matchers.begin(), Matchers.end(), [&query] (const auto& matcher) { return matcher->Match(query); });
    }

private:
    TVector<Ptr> Matchers;
};


class TContextMatcher : public TFixlistIndex::IMatcher {
public:
    explicit TContextMatcher(const YAML::Node& context) {
        for (const auto& appId : context["app_ids"]) {
            AppIds.insert(appId.as<TString>());
        }
    }

    bool Match(const TFixlistIndex::TQuery& query) const override {
        if (AppIds.empty())
            return true;

        return AppIds.contains(query.AppId);
    }

private:
    THashSet<TString> AppIds;
};

class TRegexMatcher : public TFixlistIndex::IMatcher {
public:
    explicit TRegexMatcher(const YAML::Node& nlu) {
        NPire::TFsm fsm;
        for (const auto& textNode : nlu) {
            auto text = UTF8ToWide(Pcre2Pire(textNode.as<TString>()));
            if (text.Empty())
                 ythrow yexception() << "Empty text in regex matcher";
            auto fsmCurrent = NPire::TLexer(text)
                .AddFeature(NPire::NFeatures::CaseInsensitive())
                .SetEncoding(NPire::NEncodings::Utf8())
                .Parse()
                .Surround();
            fsm |= fsmCurrent;

            if (fsm.Size() > REGEX_MAX_COMPLEX) {
                Scanners.emplace_back(fsm.Compile<NPire::TScanner>());
                fsm = {};
            }
        }

        if (fsm.Size() > 0) {
            Scanners.emplace_back(fsm.Compile<NPire::TScanner>());
        }
        for (const auto& scanner : Scanners) {
            if (scanner.Empty())
                ythrow yexception() << "Empty scanner in regex matcher";
        }
    }

    bool Match(const TFixlistIndex::TQuery& query) const override {
        return std::any_of(
                Scanners.begin(),
                Scanners.end(),
                [&query] (const auto& r) {
                    return NPire::Runner(r).Begin().Run(query.Query).End();
                });
    }

private:
    TVector<NPire::TScanner> Scanners;
};

class TExactMatcher : public TFixlistIndex::IMatcher {
public:
    explicit TExactMatcher(const YAML::Node& nlu) {
        TStringBuilder regexBuilder;
        for (const auto& textNode : nlu) {
            Queries.insert(textNode.as<TString>());
        }

        if (Queries.empty()) {
            ythrow yexception() << "Empty exact matcher";
        }
    }

    bool Match(const TFixlistIndex::TQuery& query) const override {
        return Queries.contains(query.Query);
    }

private:
    THashSet<TString> Queries;
};

class TGranetMatcher : public TFixlistIndex::IMatcher {
public:
    explicit TGranetMatcher(const YAML::Node& nlu) {
        for (const auto& textNode : nlu) {
            Forms.insert(textNode.as<TString>());
        }

        if (Forms.empty()) {
            ythrow yexception() << "Empty granet matcher";
        }
    }

    bool Match(const TFixlistIndex::TQuery& query) const override {
        for (const auto& fixlistForm : Forms) {
            for (const auto& formFound: query.GranetForms) {
                if (fixlistForm == formFound) {
                    return true;
                }
            }
        }
        return false;
    }
private:
    THashSet<TString> Forms;
};

TFixlistIndex::TIntentToMatchersMap CreateIntentToMatchers(IInputStream* fileStream) {
    TFixlistIndex::TIntentToMatchersMap intentToMatchers;

    const auto rootNode = YAML::Load(fileStream->ReadAll());

    for (const auto& node : rootNode) {
        const auto intentCase = node.first.as<TString>();
        const auto intentDesc = node.second;
        const auto intentName = intentDesc["intent"].IsDefined()
                                ? intentDesc["intent"].as<TString>()
                                : intentCase;

        TVector<TFixlistIndex::IMatcher::Ptr> matchers;
        if (intentDesc["context"].IsDefined()) {
            matchers.emplace_back(new TContextMatcher(intentDesc["context"]));
        }
        if (intentDesc["nlu_regex"].IsDefined()) {
            matchers.emplace_back(new TRegexMatcher(intentDesc["nlu_regex"]));
        }
        if (intentDesc["nlu_exact"].IsDefined()) {
            matchers.emplace_back(new TExactMatcher(intentDesc["nlu_exact"]));
        }
        if (intentDesc["nlu_granet"].IsDefined()) {
            matchers.emplace_back(new TGranetMatcher(intentDesc["nlu_granet"]));
        }

        if (matchers.empty()) {
            ythrow yexception() << "Empty rules for intent case: " << intentCase;
        }

        intentToMatchers[intentName].emplace_back(new TAndMatcher(matchers));
    }

    return intentToMatchers;
}

TVector<TString> MatchAgainstType(const TFixlistIndex::TQuery& query, const TFixlistIndex::TIntentToMatchersMap& intentToMatchers) {
    TVector<TString> result;
    for (const auto& [intent, matchers] : intentToMatchers) {
        for (const auto& matcher : matchers) {
            if (matcher->Match(query)) {
                result.push_back(intent);
            }
        }
    }

    return result;
}

} // namespace

void TFixlistIndex::AddFixlist(const TStringBuf fixlistType, IInputStream* inputStream) {
    TypeToIntentMatchers.emplace(fixlistType, CreateIntentToMatchers(inputStream));
}

TFixlistIndex::TTypeToIntentsMap TFixlistIndex::Match(const TQuery& query) const {
    TTypeToIntentsMap result;
    for (const auto& [type, intentToMatchers] : TypeToIntentMatchers) {
        result.emplace(type, MatchAgainstType(query, intentToMatchers));
    }

    return result;
}

TVector<TString> TFixlistIndex::MatchAgainst(const TQuery& query, const TStringBuf fixlistType) const {
    const auto* intentToMatchers = TypeToIntentMatchers.FindPtr(fixlistType);
    if (!intentToMatchers)
        return {};

    return MatchAgainstType(query, *intentToMatchers);
}

} // namespace NBg
