#pragma once

#include <contrib/libs/tf/tensorflow/core/framework/tensor.h>
#include <contrib/libs/tf/tensorflow/core/platform/default/integral_types.h>

#include <util/generic/vector.h>
#include <util/system/types.h>

namespace NVins {

template<typename T>
tensorflow::DataType GetTfDataType();

template<>
inline tensorflow::DataType GetTfDataType<float>() {
    return tensorflow::DT_FLOAT;
}

template<>
inline tensorflow::DataType GetTfDataType<i32>() {
    return tensorflow::DT_INT32;
}

template<>
inline tensorflow::DataType GetTfDataType<long long>() {
    return tensorflow::DT_INT64;
}

template<typename T>
tensorflow::Tensor MakeZeroTensor(const tensorflow::TensorShape& shape) {
    tensorflow::Tensor result(GetTfDataType<T>(), shape);
    auto* rawData = result.flat<T>().data();
    Fill(rawData, rawData + result.NumElements(), 0);
    return result;
}

template<typename T>
tensorflow::Tensor MakeScalarTensor(T source) {
    tensorflow::Tensor result(GetTfDataType<T>(), tensorflow::TensorShape());
    result.scalar<T>()() = source;
    return result;
}

template<typename T>
TVector<T> ConvertTensorTo1DimTable(const tensorflow::Tensor& data) {
    i64 dim0Size = data.dim_size(0);

    auto&& dataData = data.tensor<T, 1>();

    TVector<T> result(dim0Size);
    for (i64 dim0 = 0; dim0 < dim0Size; ++dim0) {
        result[dim0] = dataData(dim0);
    }

    return result;
}

template<typename T>
TVector<TVector<T>> ConvertTensorTo2DimTable(const tensorflow::Tensor& data) {
    i64 dim0Size = data.dim_size(0);
    i64 dim1Size = data.dim_size(1);

    auto&& dataData = data.tensor<T, 2>();

    TVector<TVector<T>> result(dim0Size, TVector<T>(dim1Size));
    for (i64 dim0 = 0; dim0 < dim0Size; ++dim0) {
        for (i64 dim1 = 0; dim1 < dim1Size; ++dim1) {
            result[dim0][dim1] = dataData(dim0, dim1);
        }
    }

    return result;
}

template<typename T>
tensorflow::Tensor Convert1DimTableToTensor(const TVector<T>& data) {
    i64 dim0Size = data.size();

    tensorflow::Tensor result = tensorflow::Tensor(GetTfDataType<T>(), {dim0Size});
    auto&& resultData = result.tensor<T, 1>();

    for (i64 dim0 = 0; dim0 < dim0Size; ++dim0) {
        resultData(dim0) = data[dim0];
    }

    return result;
}

template<typename T>
tensorflow::Tensor Convert2DimTableToTensor(const TVector<TVector<T>>& data) {
    const i64 dim0Size = data.size();
    if (dim0Size == 0) {
        return tensorflow::Tensor();
    }
    const i64 dim1Size = data[0].size();

    tensorflow::Tensor result = tensorflow::Tensor(GetTfDataType<T>(), {dim0Size, dim1Size});
    auto&& resultData = result.tensor<T, 2>();

    for (i64 dim0 = 0; dim0 < dim0Size; ++dim0) {
        for (i64 dim1 = 0; dim1 < dim1Size; ++dim1) {
            resultData(dim0, dim1) = data[dim0][dim1];
        }
    }

    return result;
}

template<typename T>
tensorflow::Tensor Convert3DimTableToTensor(const TVector<TVector<TVector<T>>>& data) {
    const i64 dim0Size = data.size();
    if (dim0Size == 0) {
        return tensorflow::Tensor();
    }
    const i64 dim1Size = data[0].size();
    if (dim1Size == 0) {
        return tensorflow::Tensor();
    }
    const i64 dim2Size = data[0][0].size();

    tensorflow::Tensor result = tensorflow::Tensor(
        GetTfDataType<T>(),
        {dim0Size, dim1Size, dim2Size}
    );
    auto&& resultData = result.tensor<T, 3>();

    for (i64 dim0 = 0; dim0 < dim0Size; ++dim0) {
        for (i64 dim1 = 0; dim1 < dim1Size; ++dim1) {
            for (i64 dim2 = 0; dim2 < dim2Size; ++dim2) {
                resultData(dim0, dim1, dim2) = data[dim0][dim1][dim2];
            }
        }
    }

    return result;
}

}; // namespace NVins
