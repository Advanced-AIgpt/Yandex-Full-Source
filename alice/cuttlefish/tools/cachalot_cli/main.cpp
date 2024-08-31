#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <library/cpp/getoptpb/util.h>


#include <alice/cuttlefish/tools/cachalot_cli/args.pb.h>

#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/string/hex.h>


using namespace NYdb;


TCommandLineArgs MakeArgs(int argc, const char **argv);

TDriverConfig MakeConfig(const TCommandLineArgs& args);


int main(int argc, const char **argv) {
    TCommandLineArgs args = MakeArgs(argc, argv);

    TMaybe<TStatus> status;
    if (args.GetAction() == "db-delete") {
        TDriver ydb(MakeConfig(args));
        NTable::TTableClient cli(ydb);
        status = cli.RetryOperationSync([args](NTable::TSession session) {
            return session.DropTable(
                args.GetDatabase() + "/" + args.GetTable(),
                NTable::TDropTableSettings()
                    .OperationTimeout(TDuration::Seconds(3))
            ).GetValueSync();
        });

        if (status && status->IsSuccess()) {
            Cerr << "Status OK" << Endl;
        } else if (status) {
            Cerr << "Status Issues: " << status->GetIssues().ToString() << Endl;
        } else {
            Cerr << "No Action" << Endl;
        }
    } else if (args.GetAction() == "db-create") {
        TDriver ydb(MakeConfig(args));
        NTable::TTableClient cli(ydb);

        status = cli.RetryOperationSync([args](NTable::TSession session) {
            return session.CreateTable(
                args.GetDatabase() + "/" + args.GetTable(),
                NTable::TTableBuilder()
                    .AddNullableColumn("ShardId", EPrimitiveType::Uint64)
                    .AddNullableColumn("Key", EPrimitiveType::Utf8)
                    .AddNullableColumn("Deadline", EPrimitiveType::Timestamp)
                    .AddNullableColumn("Audio", EPrimitiveType::String)
                    .SetPrimaryKeyColumns({"ShardId", "Key"})
                    .SetUniformPartitions(64)
                    .SetTtlSettings("Deadline", TDuration::Seconds(args.GetTtl()))
                    .Build()
                ,
                NTable::TCreateTableSettings()
                    .OperationTimeout(TDuration::Seconds(3))
            ).GetValueSync();
        });
        if (status && status->IsSuccess()) {
            Cerr << "Status OK" << Endl;
        } else if (status) {
            Cerr << "Status Issues: " << status->GetIssues().ToString() << Endl;
        } else {
            Cerr << "No Action" << Endl;
        }
    } else if (args.GetAction() == "shoot") {
        Cerr << "TODO: implement" << Endl;
    }

    return 0;
}


TCommandLineArgs MakeArgs(int argc, const char **argv) {
    NGetoptPb::TGetoptPbSettings optSettings;
    optSettings.DontRequireRequired = false;
    return NGetoptPb::GetoptPbOrAbort(argc, argv, optSettings);
}


TString GetAuthToken() {
    return TString(getenv("YDB_TOKEN"));
}


TDriverConfig MakeConfig(const TCommandLineArgs& args) {
    TDriverConfig config;
    config
        .SetEndpoint(args.GetEndpoint())
        .SetDatabase(args.GetDatabase())
        .SetAuthToken(GetAuthToken())
        .SetClientThreadsNum(1)
        .SetNetworkThreadsNum(1)
    ;
    return config;
}

