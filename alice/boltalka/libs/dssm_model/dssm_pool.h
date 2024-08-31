#pragma once
#include "dssm_model.h"

#include <util/thread/lfqueue.h>

namespace NNlg {

class TDssmPool : public TThrRefBase {
public:
    TDssmPool(const TString& modelFilename, size_t threadCount);
    TDssmPool(NDssm3::TModelFactoryPtr<float> modelFactory, size_t threadCount);

    TVector<TVector<float>> Fprop(const TVector<TString>& context,
                                  const TString& reply,
                                  const TVector<TString>& output);
    TVector<TVector<float>> FpropBatch(const TVector<TVector<TString>>& contexts,
                                       const TVector<TString>& replies,
                                       const TVector<TString>& output);
private:
    TLockFreeQueue<NNlg::TDssmModelPtr> Models;
};

using TDssmPoolPtr = TIntrusivePtr<TDssmPool>;

}
