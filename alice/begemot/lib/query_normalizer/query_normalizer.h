#include <alice/nlu/libs/interval/interval.h>
#include <alice/nlu/libs/token_interval_inflector/token_interval_inflector.h>

#include <contrib/libs/re2/re2/re2.h>

#include <util/generic/string.h>

#include <memory>

namespace NBg {

struct TPrefixNormalizationRule {
    std::shared_ptr<re2::RE2> ExpectedPrefix;
    TString SourceCase;
};

TString NormalizeQueryByPrefix(const TVector<TPrefixNormalizationRule>& rules,
                               const TVector<TString>& tokens,
                               const NNlu::TInterval& queryInterval,
                               const NAlice::TTokenIntervalInflector& inflector);

TString NormalizeSearchQueryByPrefix(const TVector<TString>& tokens,
                                     const NNlu::TInterval& queryInterval,
                                     const NAlice::TTokenIntervalInflector& inflector);

} // namespace NBg