#pragma once

#include <mapreduce/yt/interface/client.h>

using TColumnToQuantiles = THashMap<TString, TVector<float>>;
using TColumnToBorders = THashMap<TString, TVector<std::pair<float, float>>>;

void CalcEmbeddingsBorders(TIntrusivePtr<NYT::IClient> client,
                           TString inputTable,
                           TColumnToQuantiles* column2Quantiles,
                           TString bordersTable);

TColumnToBorders ReadEmbeddingsBorders(TIntrusivePtr<NYT::IClient> client,
                                       TColumnToQuantiles* column2Quantiles,
                                       TString bordersTable);
