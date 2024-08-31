#include <alice/wonderlogs/rt/protos/uuid_message_id.pb.h>

#include <kernel/yt/dynamic/client.h>
#include <kernel/yt/dynamic/table.h>

#include <mapreduce/yt/interface/init.h>

#include <yt/yt/core/misc/shutdown.h>

using namespace NAlice::NWonderlogs;

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    auto client = NYT::NApi::CreateClient("markov");
    {
        NYT::NProtoApi::TTable<TUuidMessageId> table(
            "//home/alice/test/big-rt/states/wonderlogs-creator/uuid-message-ids", client);
        table.Setup(true);
    }

    NYT::Shutdown();
    return 0;
}
