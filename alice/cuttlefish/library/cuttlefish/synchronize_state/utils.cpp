#include "utils.h"

#include <util/generic/hash_set.h>
#include <util/generic/vector.h>


namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

    class TTestIdsBuilder {
    public:
        explicit TTestIdsBuilder(TStringBuilder* output)
            : Output(output)
        {
        }

        void AddTestId(const TString& testId) {
            // multiple test-ids are separated by '_':
            // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/abt/uaas/#kakjamogupopastvtrebuemyevyborki

            if (!Started()) {
                (*Output) << "&test-id=" << testId;
                AddedTestIds.insert(testId);
            } else {
                if (!AddedTestIds.contains(testId)) {
                    (*Output) << '_' << testId;
                    AddedTestIds.insert(testId);
                }
            }
        }

        bool Started() const {
            return !AddedTestIds.empty();
        }

    private:
        TStringBuilder* Output;
        THashSet<TString> AddedTestIds;
    };

}


bool AppendTestIds(
    TStringBuilder* path,
    const NAliceProtocol::TSessionContext& ctx,
    const NAliceProtocol::TAbFlagsProviderOptions& options
) {
    // Uses O(1) additional memory.
    TTestIdsBuilder builder(path);

    if (const auto& exps = ctx.GetExperiments(); exps.UaasTestsSize()) {
        for (const TString& id : exps.GetUaasTests()) {
            builder.AddTestId(id);
        }
    }

    if (ctx.HasConnectionInfo()) {
        const NAliceProtocol::TConnectionInfo& connInfo = ctx.GetConnectionInfo();
        if (connInfo.TestIdsSize()) {
            for (const TString& id : connInfo.GetTestIds()) {
                builder.AddTestId(id);
            }
        }
    }

    if (options.TestIdsSize()) {
        for (const TString& id : options.GetTestIds()) {
            builder.AddTestId(id);
        }
    }

    return builder.Started();
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
