#include "unistat_registry.h"

#include <util/string/join.h>

namespace NNlg {

namespace {

const NUnistat::TIntervals INTERVALS = {
    0, 10, 20, 30, 40, 50, 60, 70, 80, 90,
    100, 110, 120, 130, 140, 150, 160, 170, 180, 190,
    200, 210, 220, 230, 240, 250, 260, 270, 280, 290,
    300, 400, 500, 600, 700, 800, 900, 1000
};

const NUnistat::TIntervals INTERVALS_FOR_CANDIDATES = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47, 48, 49
};

} // namespace

TUnistatRegistry::TUnistatRegistry(const TStringBuf prefix)
    : RequestCount(TUnistat::Instance().DrillFloatHole(TString::Join(prefix, "request"), "summ", NUnistat::TPriority(0)))
    , RequestErrorCount(TUnistat::Instance().DrillFloatHole(TString::Join(prefix, "request_error"), "summ", NUnistat::TPriority(0)))
    , RequestTime(TUnistat::Instance().DrillHistogramHole(TString::Join(prefix, "request_time"), "hgram", NUnistat::TPriority(0), INTERVALS))
    , RequestRerankerTime(TUnistat::Instance().DrillHistogramHole(TString::Join(prefix, "request_reranker_time"), "hgram", NUnistat::TPriority(0), INTERVALS))
    , Seq2SeqRequestCount(TUnistat::Instance().DrillFloatHole(TString::Join(prefix, "seq2seq_request"), "summ", NUnistat::TPriority(0)))
    , Seq2SeqRequestCandidatesCount(TUnistat::Instance().DrillHistogramHole(TString::Join(prefix, "seq2seq_request_candidates"), "hgram", NUnistat::TPriority(0), INTERVALS_FOR_CANDIDATES))
    , Seq2SeqRequestTime(TUnistat::Instance().DrillHistogramHole(TString::Join(prefix, "seq2seq_request_time"), "hgram", NUnistat::TPriority(0), INTERVALS))
    , Seq2SeqRequestGetTime(TUnistat::Instance().DrillHistogramHole(TString::Join(prefix, "seq2seq_request_get_time"), "hgram", NUnistat::TPriority(0), INTERVALS))
    , Seq2SeqRequestEmbeddingTime(TUnistat::Instance().DrillHistogramHole(TString::Join(prefix, "seq2seq_request_embedding_time"), "hgram", NUnistat::TPriority(0), INTERVALS))
    , Seq2SeqRequestErrorCount(TUnistat::Instance().DrillFloatHole(TString::Join(prefix, "seq2seq_error"), "summ", NUnistat::TPriority(0)))
    , Seq2SeqRequestErrorTimeoutCount(TUnistat::Instance().DrillFloatHole(TString::Join(prefix, "seq2seq_error_timeout"), "summ", NUnistat::TPriority(0)))
    , Seq2SeqRequestErrorServerCount(TUnistat::Instance().DrillFloatHole(TString::Join(prefix, "seq2seq_error_server"), "summ", NUnistat::TPriority(0)))
    , Seq2SeqRequestErrorFormatCount(TUnistat::Instance().DrillFloatHole(TString::Join(prefix, "seq2seq_error_format"), "summ", NUnistat::TPriority(0)))
    , Seq2SeqRequestErrorMakeCount(TUnistat::Instance().DrillFloatHole(TString::Join(prefix, "seq2seq_error_make"), "summ", NUnistat::TPriority(0)))
    , BertRequestCount(TUnistat::Instance().DrillFloatHole(TString::Join(prefix, "bert_request"), "summ", NUnistat::TPriority(0)))
    , BertRequestTime(TUnistat::Instance().DrillHistogramHole(TString::Join(prefix, "bert_request_time"), "hgram", NUnistat::TPriority(0), INTERVALS))
    , BertRequestErrorCount(TUnistat::Instance().DrillFloatHole(TString::Join(prefix, "bert_error"), "summ", NUnistat::TPriority(0)))
    , BertRequestErrorTimeoutCount(TUnistat::Instance().DrillFloatHole(TString::Join(prefix, "bert_error_timeout"), "summ", NUnistat::TPriority(0)))
    , BertRequestErrorServerCount(TUnistat::Instance().DrillFloatHole(TString::Join(prefix, "bert_error_server"), "summ", NUnistat::TPriority(0)))
    , BertRequestErrorFormatCount(TUnistat::Instance().DrillFloatHole(TString::Join(prefix, "bert_error_format"), "summ", NUnistat::TPriority(0)))
    , BertRequestErrorMakeCount(TUnistat::Instance().DrillFloatHole(TString::Join(prefix, "bert_error_make"), "summ", NUnistat::TPriority(0)))
    , IndexRequestCount(TUnistat::Instance().DrillFloatHole(TString::Join(prefix, "index_request"), "summ", NUnistat::TPriority(0)))
    , IndexRequestTime(TUnistat::Instance().DrillHistogramHole(TString::Join(prefix, "index_request_time"), "hgram", NUnistat::TPriority(0), INTERVALS))
{
}

} // namespace NNlg
