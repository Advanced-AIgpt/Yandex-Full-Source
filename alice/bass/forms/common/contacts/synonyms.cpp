#include "synonyms.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/resource/resource.h>

#include <util/string/split.h>
#include <util/stream/str.h>

namespace NBASS {

TClusters::TClusters()
{
}

void TClusters::Init() {
    TString data;
    if (!NResource::FindExact("call_name_synonyms.txt", &data)) {
        LOG(ERR) << "Can't find name synonyms resource" << Endl;
        ythrow yexception() << TStringBuf("Can't find name synonyms resource 'call_name_synonyms.txt'");
    }

    TStringInput in(data);
    TString cluster;
    for (size_t i = 0; in.ReadLine(cluster) != 0; ++i) {
        TVector<TString> words;
        Split(cluster, ",", words);
        for (const TString& word: words) {
            WordToIds.insert(std::make_pair(word, i));
            IdToCluster.insert(std::make_pair(i, word));
        }
    }
}

TClusters& TClusters::Instance() {
    return *Singleton<TClusters>();
}

TVector<TString> TClusters::GetCluster(const TString& item) const {
    TVector<TString> result;
    auto idsRange = WordToIds.equal_range(item);
    for (auto idIter = idsRange.first; idIter != idsRange.second; ++idIter) {
        auto clusterRange = IdToCluster.equal_range(idIter->second);
        for (auto sinIter = clusterRange.first; sinIter != clusterRange.second; ++sinIter) {
            result.push_back(sinIter->second);
        }
    }
    return result;
}

}
