#pragma once

#include <alice/hollywood/library/phrases/phrases.h>

#include <util/generic/hash_set.h>

namespace NAlice::NTesting {

THashSet<TString> CheckPhrasesCorpus(const TPhrasesCorpus& corpus, const TPhraseCollection::TTagChecker& tagChecker,
                                     bool checkEmpty = false, const THashSet<TString>& maybeEmpty = {});

} // namespace NAlice::NTesting
