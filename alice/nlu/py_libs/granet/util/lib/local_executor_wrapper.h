#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <alice/nlu/granet/lib/granet.h>
#include <alice/nlu/granet/lib/parser/result.h>
#include <alice/nlu/libs/interval/interval.h>

namespace NParallelGranet {

using namespace NGranet;
using TParts = TVector<std::pair<NNlu::TInterval, bool>>;

TVector<TString> ParseInParallel(const TGrammar::TRef& grammar, TVector<TIntrusivePtr<TSample> >& samples, int nThreads, int blockSize=1000, bool needValues=false, bool needTypes=false);

TVector<TParts> ParseToAlmostMatchedPartsInParallel(const TGrammar::TRef& grammar, TVector<TIntrusivePtr<TSample> >& samples, int nThreads, int blockSize=1000);

TVector<TParserFormResult::TConstRef> ParseToResultsInParallel(const TGrammar::TRef& grammar, TVector<TSample::TRef>& samples, int nThreads, int blockSize=1000);

}
