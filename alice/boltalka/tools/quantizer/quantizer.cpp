#include "quantizer.h"

#include <util/generic/algorithm.h>

#include <cmath>

TQuantizer::TQuantizer()
    : InputMin(-1.)
    , OutputMin(Min<i8>())
    , OutputMax(Max<i8>())
    , Scale((OutputMax - OutputMin) / 2.)
    , Quantile(0.)
{
}

TQuantizer::TQuantizer(float inputMin, float inputMax, float outputMin, float outputMax, float quantile)
    : OutputMin(outputMin)
    , OutputMax(outputMax)
    , Quantile(quantile)
{
    inputMax = Max(inputMax, inputMin * (OutputMax / OutputMin));
    InputMin = inputMax * (OutputMin / OutputMax);
    Scale = (OutputMax - OutputMin) / (inputMax - InputMin);
}

i8 TQuantizer::Apply(float val) const {
    return (i8)round(Max(Min(Scale * (val - InputMin) + OutputMin, OutputMax), OutputMin));
}

TVector<i8> TQuantizer::Apply(const TVector<float>& vec) const {
    TVector<i8> vecOut(vec.size());
    for (size_t i = 0; i < vecOut.size(); ++i) {
        vecOut[i] = TQuantizer::Apply(vec[i]);
    }
    return vecOut;
}

float TQuantizer::GetQuantile() const {
    return Quantile;
}
