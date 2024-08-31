#pragma once

#include <library/cpp/unistat/unistat.h>

namespace NNlg {

struct TUnistatRegistry {
    NUnistat::IHolePtr RequestCount;
    NUnistat::IHolePtr RequestErrorCount;
    NUnistat::IHolePtr RequestTime;
    NUnistat::IHolePtr RequestRerankerTime;

    NUnistat::IHolePtr Seq2SeqRequestCount;
    NUnistat::IHolePtr Seq2SeqRequestCandidatesCount;
    NUnistat::IHolePtr Seq2SeqRequestTime;
    NUnistat::IHolePtr Seq2SeqRequestGetTime;
    NUnistat::IHolePtr Seq2SeqRequestEmbeddingTime;
    NUnistat::IHolePtr Seq2SeqRequestErrorCount;
    NUnistat::IHolePtr Seq2SeqRequestErrorTimeoutCount;
    NUnistat::IHolePtr Seq2SeqRequestErrorServerCount;
    NUnistat::IHolePtr Seq2SeqRequestErrorFormatCount;
    NUnistat::IHolePtr Seq2SeqRequestErrorMakeCount;

    NUnistat::IHolePtr BertRequestCount;
    NUnistat::IHolePtr BertRequestTime;
    NUnistat::IHolePtr BertRequestErrorCount;
    NUnistat::IHolePtr BertRequestErrorTimeoutCount;
    NUnistat::IHolePtr BertRequestErrorServerCount;
    NUnistat::IHolePtr BertRequestErrorFormatCount;
    NUnistat::IHolePtr BertRequestErrorMakeCount;

    NUnistat::IHolePtr IndexRequestCount;
    NUnistat::IHolePtr IndexRequestTime;

    TUnistatRegistry(const TStringBuf prefix);
};

} // namespace NNlg
