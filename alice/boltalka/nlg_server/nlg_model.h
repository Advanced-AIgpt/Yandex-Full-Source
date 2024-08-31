#pragma once
#include <cv/imgclassifiers/danet/common/common.h>
#include <cv/imgclassifiers/danet/common/typedefs.h>
#include <cv/imgclassifiers/danet/config/config.h>
#include <cv/imgclassifiers/danet/data_provider/ext/text_utils/text_utils.h>
#include <cv/imgclassifiers/danet/nnlib/tester/ff_tester.h>
#include <cv/imgclassifiers/danet/nnlib/tester/rnn_tester.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NNlgServer {

class INlgModel;
using INlgModelSPtr = TAtomicSharedPtr<INlgModel>;

class INlgModel {
public:
    virtual ~INlgModel() = default;
    virtual TVector<TUtf16String> GetReplies(const TUtf16String &context, ui64 maxLen, double temperature, ui64 numSamples) const = 0;
};

class TEveryContextLengthModel : public INlgModel {
public:
    TEveryContextLengthModel(TVector<INlgModelSPtr> nlgModels);
    TVector<TUtf16String> GetReplies(const TUtf16String &context, ui64 maxLen, double temperature, ui64 numSamples) const override;

private:
    TVector<INlgModelSPtr> NlgModels;
};

}
