#pragma once
#include <ml/dssm/lib/lstm.h>
#include <ml/dssm/lib/lstm2.h>

#include <util/folder/path.h>

namespace NNlgServer {

using TMatrix = NDssm3::TMatrix<float>;
using TMatrixPtr = NDssm3::TMatrixPtr<float>;
using TLstmParameters = NDssm3::TLstmParameters<float>;
using TLstmParametersPtr = NDssm3::TLstmParametersPtr<float>;
using TLstm2Parameters = NDssm3::TLstm2Parameters<float>;
using TLstm2ParametersPtr = NDssm3::TLstm2ParametersPtr<float>;

struct TFcParameters : public TThrRefBase {
    TMatrixPtr Weights;
    TMatrixPtr Biases;
};
using TFcParametersPtr = TIntrusivePtr<TFcParameters>;

TMatrixPtr LoadMatrix(const TFsPath &path);
TFcParametersPtr LoadFc(const TFsPath &modelDir, const TString &name, bool transposeWeights = true);
TLstmParametersPtr LoadLstm(const TFsPath &modelDir, const TString &name);
TLstm2ParametersPtr LoadLstm2(const TFsPath &modelDir, const TString &name);

}
