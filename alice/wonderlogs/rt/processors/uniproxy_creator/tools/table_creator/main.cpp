#include <alice/wonderlogs/rt/processors/uniproxy_creator/protos/uniproxy_prepared_wrapper.pb.h>

#include <kernel/yt/dynamic/client.h>
#include <kernel/yt/dynamic/table.h>

#include <mapreduce/yt/interface/init.h>

#include <yt/yt/core/misc/shutdown.h>

using namespace NAlice::NWonderlogs;

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    auto client = NYT::NApi::CreateClient("markov");
    {
        NYT::NProtoApi::TTable<TUniproxyPreparedWrapper> table(
            "//home/alice/test/big-rt/states/uniproxy-creator/uniproxy-prepared", client);
        table.Setup(true);
    }
    {
        NYT::NProtoApi::TTable<TUniproxyPreparedWrapper> table(
            "//home/alice/test/big-rt/tables/uniproxy-creator/uniproxy-prepared", client);
        table.Setup(true);
    }

    NYT::Shutdown();
    return 0;
}
