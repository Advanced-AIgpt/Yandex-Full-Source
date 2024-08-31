#include "layers.h"

#include <util/stream/file.h>
#include <util/string/cast.h>

namespace NNlgServer {

static TVector<size_t> GetDims(TStringBuf s) {
    TVector<size_t> dims;
    for (TStringBuf tok; s.NextTok(' ', tok); ) {
        dims.push_back(FromString<size_t>(tok));
    }
    if (dims.size() == 1) {
        dims.insert(dims.begin(), 1);
    }
    return dims;
}

TMatrixPtr LoadMatrix(const TFsPath &path) {
    TFileInput in(path);
    TString line;
    in.ReadLine(line);

    auto dims = GetDims(line);
    Y_VERIFY(dims.size() == 2);

    TMatrixPtr matrix = new TMatrix(dims[0], dims[1]);
    for (size_t i = 0; i < dims[0]; ++i) {
        for (size_t j = 0; j < dims[1]; ++j) {
            in >> (*matrix)[i][j];
        }
    }
    return matrix;
}

static void LoadFc(const TFsPath &modelDir, const TString &name, TMatrixPtr *weights, TMatrixPtr *biases, bool transposeWeights) {
    TVector<TMatrixPtr> matrices;
    size_t numInputs = 0;
    size_t numOutputs = 0;
    for (size_t i = 0; ; ++i) {
        TFsPath path = modelDir / ("layer_" + name + "_" + ToString(i) + ".wmtx");
        if (!path.Exists()) {
            break;
        }
        matrices.push_back(LoadMatrix(path));
        size_t curNumInputs = matrices.back()->GetNumRows();
        size_t curNumOutputs = matrices.back()->GetNumColumns();
        if (numOutputs == 0) {
            numOutputs = curNumOutputs;
        }
        Y_VERIFY(numOutputs == curNumOutputs);
        numInputs += curNumInputs;
    }
    if (transposeWeights) {
        *weights = new TMatrix(numOutputs, numInputs);
        float *weightsData = (*weights)->data();
        for (size_t i = 0; i < numOutputs; ++i) {
            for (const auto &matrix : matrices) {
                for (size_t j = 0; j < matrix->GetNumRows(); ++j) {
                    *weightsData++ = (*matrix)[j][i];
                }
            }
        }
    } else {
        *weights = new TMatrix(numInputs, numOutputs);
        float *weightsData = (*weights)->data();
        for (const auto &matrix : matrices) {
            memcpy(weightsData, matrix->data(), matrix->size() * sizeof(float));
            weightsData += matrix->size();
        }
    }

    *biases = LoadMatrix(modelDir / ("layer_" + name + "_biases.wmtx"));
    Y_VERIFY((*biases)->GetNumRows() == 1);
    if (transposeWeights) {
        Y_VERIFY((*biases)->GetNumColumns() == (*weights)->GetNumRows());
    } else {
        Y_VERIFY((*biases)->GetNumColumns() == (*weights)->GetNumColumns());
    }
}

TFcParametersPtr LoadFc(const TFsPath &modelDir, const TString &name, bool transposeWeights) {
    TFcParametersPtr fc = new TFcParameters();
    LoadFc(modelDir, name, &fc->Weights, &fc->Biases, transposeWeights);
    return fc;
}

TLstmParametersPtr LoadLstm(const TFsPath &modelDir, const TString &name) {
    TLstmParametersPtr params = new TLstmParameters(0, 0);
    LoadFc(modelDir, name + "_pre_input_gate", &params->InputGateMatrix, &params->InputGateBiases, true);
    LoadFc(modelDir, name + "_pre_forget_gate", &params->ForgetGateMatrix, &params->ForgetGateBiases, true);
    LoadFc(modelDir, name + "_pre_output_gate", &params->OutputGateMatrix, &params->OutputGateBiases, true);
    LoadFc(modelDir, name + "_pre_cell_value", &params->NewStateMatrix, &params->NewStateBiases, true);
    return params;
}

TLstm2ParametersPtr LoadLstm2(const TFsPath &modelDir, const TString &name) {
    TLstmParametersPtr params = LoadLstm(modelDir, name);
    TLstm2ParametersPtr params2 = new TLstm2Parameters(params->GetMemorySize(), params->GetInputSize());

    float *weightsData = params2->InputToGatesAndStateMatrix->data();
    for (const auto &matrix : { params->InputGateMatrix, params->OutputGateMatrix, params->ForgetGateMatrix, params->NewStateMatrix }) {
        memcpy(weightsData, matrix->data(), matrix->size() * sizeof(float));
        weightsData += matrix->size();
    }

    float *biasesData = params2->InputToGatesAndStateBiases->data();
    for (const auto &biases : { params->InputGateBiases, params->OutputGateBiases, params->ForgetGateBiases, params->NewStateBiases }) {
        memcpy(biasesData, biases->data(), biases->size() * sizeof(float));
        biasesData += biases->size();
    }

    return params2;
}

}
