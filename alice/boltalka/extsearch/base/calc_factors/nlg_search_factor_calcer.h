#pragma once
#include <util/generic/fwd.h>
#include <util/generic/string.h>

namespace NNlg {

class TFactorCalcer;

TFactorCalcer* CreateNlgSearchFactorCalcer(const TString& rusListerMapFilename, const TVector<TString>& baseDssmModelNames, const TVector<TString>& factorDssmModelNames);
TFactorCalcer* CreateNlgSearchFactorCalcer(const TVector<TString>& rusListerMap, const TVector<TString>& baseDssmModelNames, const TVector<TString>& factorDssmModelNames);

}
