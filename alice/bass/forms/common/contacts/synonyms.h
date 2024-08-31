#pragma once

#include <util/generic/hash.h>
#include <util/generic/singleton.h>
#include <util/generic/vector.h>

namespace NBASS {

class TClusters {
public:
    void Init();
    static TClusters& Instance();

    TVector<TString> GetCluster(const TString& item) const;

private:
    Y_DECLARE_SINGLETON_FRIEND();
    TClusters();

private:
    THashMultiMap<TString, size_t> WordToIds;
    THashMultiMap<size_t, TString> IdToCluster;
};

}
