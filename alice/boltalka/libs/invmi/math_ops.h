#pragma once
#include <contrib/libs/intel/mkl/include/mkl_cblas.h>

#include <util/generic/ylimits.h>
#include <util/system/yassert.h>

#include <library/cpp/dot_product/dot_product.h>

/*inline float DotProduct(const float* lhs, const float* rhs, size_t length) {
    return cblas_sdot(length, lhs, 1, rhs, 1);
}*/

inline float SqrL2Norm(const float* vec, size_t length) {
    return DotProduct(vec, vec, length);
}

inline void AddScaledVector(float* result, float alpha, const float* vec, size_t length) {
    cblas_saxpy(length, alpha, vec, 1, result, 1);
}

template<class TData>
struct TMatrixInfo {
    static const size_t UNDEFINED_STRIDE = Max<size_t>();

    TData* Data = nullptr;
    size_t NumRows = 0;
    size_t NumCols = 0;
    CBLAS_TRANSPOSE TransposeFlag = CblasNoTrans;
    float Scale = 0.0;
    size_t Stride = UNDEFINED_STRIDE;

    size_t GetStride() const {
        return Stride != UNDEFINED_STRIDE ? Stride : NumCols;
    }
    size_t GetRealNumRows() const {
        return TransposeFlag == CblasNoTrans ? NumRows : NumCols;
    }
    size_t GetRealNumCols() const {
        return TransposeFlag == CblasNoTrans ? NumCols : NumRows;
    }
};

inline void MatrixMatrixMultiply(TMatrixInfo<const float> A, TMatrixInfo<const float> B, TMatrixInfo<float> C) {
    Y_VERIFY(A.GetRealNumCols() == B.GetRealNumRows(), "A and B are not compatible");
    Y_VERIFY(A.GetRealNumRows() == C.GetRealNumRows(), "Wrong result NumRows");
    Y_VERIFY(B.GetRealNumCols() == C.GetRealNumCols(), "Wrong result NumCols");

    if (B.NumCols == 1 && B.GetRealNumCols() == B.NumCols) {
        cblas_sgemv(CblasRowMajor,
                    A.TransposeFlag,
                    A.NumRows, A.NumCols,
                    A.Scale, A.Data, A.GetStride(),
                    B.Data, B.GetStride(),
                    C.Scale, C.Data, C.GetStride());
    } else {
        cblas_sgemm(CblasRowMajor,
                    A.TransposeFlag, B.TransposeFlag,
                    A.GetRealNumRows(), B.GetRealNumCols(), A.GetRealNumCols(),
                    A.Scale, A.Data, A.GetStride(),
                    B.Data, B.GetStride(),
                    C.Scale, C.Data, C.GetStride());
    }
}

