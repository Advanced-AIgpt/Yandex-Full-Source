#include "dssm_pool.h"

namespace NNlg {

TDssmPool::TDssmPool(const TString& modelFilename, size_t threadCount)
    : TDssmPool(NDssm3::TModelFactory<float>::Load(modelFilename), threadCount)
{
}

TDssmPool::TDssmPool(NDssm3::TModelFactoryPtr<float> modelFactory, size_t threadCount) {
    for (size_t i = 0; i < threadCount; ++i) {
        Models.Enqueue(new NNlg::TDssmModel(modelFactory));
    }
}

TVector<TVector<float>> TDssmPool::Fprop(const TVector<TString>& context,
                                         const TString& reply,
                                         const TVector<TString>& output) {
    return FpropBatch({context}, {reply}, output);
}

TVector<TVector<float>> TDssmPool::FpropBatch(const TVector<TVector<TString>>& contexts,
                                              const TVector<TString>& replies,
                                              const TVector<TString>& output) {
   NNlg::TDssmModelPtr model;
   bool success = Models.Dequeue(&model);
   Y_VERIFY(success, "You must not call this method from more than threadCount threads");
   auto result = model->FpropBatch(contexts, replies, output);
   Models.Enqueue(model);
   return result;
}

}
