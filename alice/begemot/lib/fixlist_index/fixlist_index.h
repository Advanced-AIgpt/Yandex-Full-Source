#pragma once

#include <util/generic/fwd.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>

namespace NBg {

class TFixlistIndex {
public:
    using Ptr = THolder<TFixlistIndex>;

    struct TQuery {
        TString Query;
        TString AppId;
        THashSet<TString> GranetForms;
    };

    class IMatcher {
    public:
        using Ptr = THolder<IMatcher>;

        virtual bool Match(const TQuery& query) const = 0;
        virtual ~IMatcher() = default;
    };

    using TIntentToMatchersMap = THashMap<TString, TVector<IMatcher::Ptr>>;
    using TTypeToIntentsMap = THashMap<TString, TVector<TString>>;

public:
    TFixlistIndex() = default;
    void AddFixlist(const TStringBuf fixlistType, IInputStream* inputStream);
    TTypeToIntentsMap Match(const TQuery& query) const;
    TVector<TString> MatchAgainst(const TQuery& query, const TStringBuf fixlistType) const;

private:
    THashMap<TString, TIntentToMatchersMap> TypeToIntentMatchers;
};

} // namespace NBg

