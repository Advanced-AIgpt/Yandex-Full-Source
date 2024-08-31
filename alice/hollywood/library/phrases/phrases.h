#pragma once

#include <alice/hollywood/library/phrases/proto/phrases.pb.h>

#include <util/generic/hash.h>

namespace NAlice {

class TPhraseCollection {
public:
    using TTagChecker = std::function<bool(TStringBuf tag)>;
    using TPhraseConsumer = std::function<bool(const TPhraseGroup::TPhrase& phrase, const TVector<TStringBuf>& tags, double probability)>;
    using TPhraseGroups = THashMap<TString, TPhraseGroup>;

    TPhraseCollection(const TPhrasesCorpus& groups);

    const TPhraseGroup* FindPhraseGroup(const TStringBuf name) const {
        return PhraseGroups.FindPtr(name);
    }

    void FindPhrases(TStringBuf name, const TTagChecker& tagChecker, const TPhraseConsumer& consumer) const;

private:
    const TPhraseGroups PhraseGroups;
};

} // namespace NAlice
