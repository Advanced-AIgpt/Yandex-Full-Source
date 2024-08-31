#include <alice/wonderlogs/rt/processors/static_wonderlogs_creator/tools/table_creator/config.pb.h>

#include <alice/wonderlogs/rt/processors/static_wonderlogs_creator/protos/logviewer.pb.h>

#include <alice/wonderlogs/protos/wonderlogs.pb.h>

#include <library/cpp/getoptpb/getoptpb.h>

#include <mapreduce/yt/interface/client.h>

using namespace NAlice::NWonderlogs;

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);
    TConfig config = NGetoptPb::GetoptPbOrAbort(argc, argv);

    auto client = NYT::CreateClient(config.GetCluster());
    auto writer =
        client->CreateTableWriter<TLogviewer>(config.GetTablePath(), NYT::TTableWriterOptions{}.InferSchema(true));

    writer->Finish();
    return 0;
}
