#pragma once

#include <mapreduce/yt/interface/client.h>

#include <util/system/types.h>

class TQuantizer {
    float InputMin;
    float OutputMin;
    float OutputMax;
    float Scale;
    float Quantile;
public:
    TQuantizer();
    TQuantizer(float inputMin, float inputMax, float outputMin, float outputMax, float quantile);
    i8 Apply(float val) const;
    TVector<i8> Apply(const TVector<float>& vec) const;
    float GetQuantile() const;
    Y_SAVELOAD_DEFINE(InputMin, OutputMin, OutputMax, Scale, Quantile);
};
