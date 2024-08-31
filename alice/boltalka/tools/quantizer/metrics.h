#pragma once

#include "quantizer.h"

void CalcQuantizationMse(TIntrusivePtr<NYT::IClient> client,
                         TString inputTable,
                         TString outputTable,
                         const THashMap<TString, TVector<TQuantizer>>& column2Quantizers,
                         const TVector<TString>& quantileColumns);

void CalcQuantizationRankingMetricsWithQueries(TIntrusivePtr<NYT::IClient> client,
                                               TString inputTable,
                                               TString outputTable,
                                               TString queryTable,
                                               const THashMap<TString, TVector<TQuantizer>>& column2Quantizers,
                                               const TVector<size_t>& topSizes,
                                               float contextWeight,
                                               const TVector<TString>& replyColumns,
                                               size_t threadCount,
                                               const TVector<TString>& quantileColumns);
