#include <alice/wonderlogs/rt/processors/megamind_creator/protos/megamind_prepared_wrapper.pb.h>

#include <kernel/yt/dynamic/client.h>
#include <kernel/yt/dynamic/table.h>

#include <mapreduce/yt/interface/init.h>

#include <yt/yt/core/misc/shutdown.h>

using namespace NAlice::NWonderlogs;

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    auto client = NYT::NApi::CreateClient("markov");
    {
        NYT::NProtoApi::TTable<TMegamindPreparedWrapper> table(
            "//home/alice/test/big-rt/states/megamind-creator/megamind-prepared", client);
        table.Setup(true);
    }
    {
        NYT::NProtoApi::TTable<TMegamindPreparedWrapper> table(
            "//home/alice/test/big-rt/tables/megamind-creator/megamind-prepared", client);
        table.Setup(true);
    }

    NYT::Shutdown();
    return 0;
}
