#include "factor_names.h"

#include <kernel/externalrelev/relev.h>
#include <kernel/generated_factors_info/simple_factors_info.h>

namespace NNlg {
    class TNlgFactorsInfo : public TSimpleSearchFactorsInfo<NNlg::TFactorInfo> {
    public:
        TNlgFactorsInfo()
            : TSimpleSearchFactorsInfo<NNlg::TFactorInfo>(FI_FACTOR_COUNT, GetFactorsInfo())
        {
        }
    };

    const IFactorsInfo* GetNlgFactorsInfo() {
        return Singleton<TNlgFactorsInfo>();
    }
}
