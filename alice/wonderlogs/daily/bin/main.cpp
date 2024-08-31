#include <alice/wonderlogs/daily/bin/config.pb.h>

#include <alice/wonderlogs/daily/lib/asr_prepared.h>
#include <alice/wonderlogs/daily/lib/basket.h>
#include <alice/wonderlogs/daily/lib/dialogs.h>
#include <alice/wonderlogs/daily/lib/differ.h>
#include <alice/wonderlogs/daily/lib/expboxes.h>
#include <alice/wonderlogs/daily/lib/json_wonderlogs.h>
#include <alice/wonderlogs/daily/lib/megamind_prepared.h>
#include <alice/wonderlogs/daily/lib/uniproxy_prepared.h>
#include <alice/wonderlogs/daily/lib/wonderlogs.h>

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/protos/banned_users.pb.h>

#include <library/cpp/getoptpb/getoptpb.h>

#include <mapreduce/yt/common/config.h>
#include <mapreduce/yt/interface/client.h>

#include <util/datetime/parser.h>
#include <util/stream/output.h>
#include <util/system/user.h>

using namespace NAlice::NWonderlogs;

const ui32 DEFAULT_DIFF_CONTEXT = 3;

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);
    TConfig config = NGetoptPb::GetoptPbOrAbort(argc, argv);

    Y_ENSURE(!config.GetYt().GetCluster().empty(), "Cluster must be provided");
    auto client = NYT::CreateClient(config.GetYt().GetCluster());
    if (!config.GetYt().GetPool().empty()) {
        NYT::TConfig::Get()->Pool = config.GetYt().GetPool();
    }

    switch (config.GetOutputTableType()) {
        case TConfig::UNIPROXY_PREPARED: {
            Y_ENSURE(!config.GetInput().GetUniproxyEventsTables().empty(), "Uniproxy events tables must be provided");
            Y_ENSURE(!config.GetOutput().GetUniproxy().GetPreparedTable().empty(),
                     "Path for uniproxy prepared table must be provided");
            Y_ENSURE(!config.GetOutput().GetUniproxy().GetErrorTable().empty(),
                     "Path for uniproxy error table must be provided");

            const auto timestampFrom = ParseDatetimeOrFail(config.GetDatetimeFrom());
            const auto timestampTo = ParseDatetimeOrFail(config.GetDatetimeTo());

            TVector<TString> uniproxyEventsTables{config.GetInput().GetUniproxyEventsTables().begin(),
                                                  config.GetInput().GetUniproxyEventsTables().end()};

            MakeUniproxyPrepared(client, config.GetInput().GetTmpDirectory(), uniproxyEventsTables,
                                 config.GetOutput().GetUniproxy().GetPreparedTable(),
                                 config.GetOutput().GetUniproxy().GetErrorTable(), timestampFrom, timestampTo,
                                 TDuration::Minutes(config.GetRequestsShiftMinutes()));
            break;
        }
        case TConfig::MEGAMIND_PREPARED: {
            Y_ENSURE(!config.GetInput().GetMegamindAnalyticsLogsTables().empty(),
                     "Megamind analytics logs tables must be provided");
            Y_ENSURE(!config.GetInput().GetUniproxyPreparedTable().empty(),
                     "Path for uniproxy prepared table must be provided");
            Y_ENSURE(!config.GetOutput().GetMegamind().GetPreparedTable().empty(),
                     "Path for megamind prepared table must be provided");
            Y_ENSURE(!config.GetOutput().GetMegamind().GetErrorTable().empty(),
                     "Path for megamind error table must be provided");

            const auto timestampFrom = ParseDatetimeOrFail(config.GetDatetimeFrom());
            const auto timestampTo = ParseDatetimeOrFail(config.GetDatetimeTo());

            TVector<TString> megamindAnalyticsLogsTables{config.GetInput().GetMegamindAnalyticsLogsTables().begin(),
                                                         config.GetInput().GetMegamindAnalyticsLogsTables().end()};
            MakeMegamindPrepared(client, config.GetInput().GetTmpDirectory(),
                                 config.GetInput().GetUniproxyPreparedTable(), megamindAnalyticsLogsTables,
                                 config.GetOutput().GetMegamind().GetPreparedTable(),
                                 config.GetOutput().GetMegamind().GetErrorTable(), timestampFrom, timestampTo);
            break;
        }
        case TConfig::WONDERLOGS: {
            Y_ENSURE(!config.GetInput().GetUniproxyPreparedTable().empty(),
                     "Path for uniproxy prepared table must be provided");
            Y_ENSURE(!config.GetInput().GetMegamindPreparedTable().empty(),
                     "Path for megamind prepared table must be provided");
            Y_ENSURE(!config.GetInput().GetAsrPreparedTable().empty(), "Path for asr prepared table must be provided");
            Y_ENSURE(!config.GetOutput().GetWonderlogs().GetTable().empty(),
                     "Path for wonderlogs table must be provided");
            Y_ENSURE(!config.GetOutput().GetWonderlogs().GetPrivateTable().empty(),
                     "Path for private wonderlogs table must be provided");
            Y_ENSURE(!config.GetOutput().GetWonderlogs().GetErrorTable().empty(),
                     "Path for wonderlogs error table must be provided");
            Y_ENSURE(!config.GetOutput().GetWonderlogs().GetRobotTable().empty(),
                     "Path for wonderlogs error table must be provided");

            const auto timestampFrom = ParseDatetimeOrFail(config.GetDatetimeFrom());
            const auto timestampTo = ParseDatetimeOrFail(config.GetDatetimeTo());

            TEnvironment productionEnvironment{
                .UniproxyQloudProjects{config.GetProductionEnvironment().GetUniproxyQloudProjects().begin(),
                                       config.GetProductionEnvironment().GetUniproxyQloudProjects().end()},
                .UniproxyQloudApplications{config.GetProductionEnvironment().GetUniproxyQloudApplications().begin(),
                                           config.GetProductionEnvironment().GetUniproxyQloudApplications().end()},
                .MegamindEnvironments{config.GetProductionEnvironment().GetMegamindEnvironments().begin(),
                                      config.GetProductionEnvironment().GetMegamindEnvironments().end()}};

            MakeWonderlogs(client, config.GetInput().GetTmpDirectory(), config.GetInput().GetUniproxyPreparedTable(),
                           config.GetInput().GetMegamindPreparedTable(), config.GetInput().GetAsrPreparedTable(),
                           config.GetOutput().GetWonderlogs().GetTable(),
                           config.GetOutput().GetWonderlogs().GetPrivateTable(),
                           config.GetOutput().GetWonderlogs().GetRobotTable(),
                           config.GetOutput().GetWonderlogs().GetErrorTable(), timestampFrom, timestampTo,
                           TDuration::Minutes(config.GetRequestsShiftMinutes()), productionEnvironment);
            break;
        }
        case TConfig::DIALOGS: {
            NAlice::NWonderlogs::TBannedUsers bannedUsers;
            {
                const auto& inputBannedUsers = config.GetInput().GetBannedUsers();
                for (auto& [from, to] : {std::tie(inputBannedUsers.GetByIp(), bannedUsers.Ips),
                                         std::tie(inputBannedUsers.GetByUuid(), bannedUsers.Uuids),
                                         std::tie(inputBannedUsers.GetByDeviceId(), bannedUsers.DeviceIds)}) {
                    for (const auto& value : from) {
                        to.insert(value);
                    }
                }
            }

            TMaybe<TEnvironment> productionEnvironment;
            if (config.HasProductionEnvironment()) {
                TEnvironment env;
                env.UniproxyQloudProjects.insert(config.GetProductionEnvironment().GetUniproxyQloudProjects().begin(),
                                                 config.GetProductionEnvironment().GetUniproxyQloudProjects().end());
                env.UniproxyQloudApplications.insert(
                    config.GetProductionEnvironment().GetUniproxyQloudApplications().begin(),
                    config.GetProductionEnvironment().GetUniproxyQloudApplications().end());
                env.MegamindEnvironments.insert(config.GetProductionEnvironment().GetMegamindEnvironments().begin(),
                                                config.GetProductionEnvironment().GetMegamindEnvironments().end());
                productionEnvironment = env;
            }

            if (!config.GetInput().GetWonderlogsTable().empty()) {
                Y_ENSURE(!config.GetInput().GetWonderlogsTable().empty(),
                         "Path for wonderlogs table must be provided");
                Y_ENSURE(!config.GetOutput().GetDialogs().GetTable().empty(),
                         "Path for dialogs table must be provided");
                Y_ENSURE(!config.GetOutput().GetDialogs().GetBannedTable().empty(),
                         "Path for banned dialogs table must be provided");
                Y_ENSURE(!config.GetOutput().GetDialogs().GetErrorTable().empty(),
                         "Path for dialogs error table must be provided");
                MakeDialogs(client, config.GetInput().GetTmpDirectory(), config.GetInput().GetWonderlogsTable(),
                            config.GetOutput().GetDialogs().GetTable(),
                            config.GetOutput().GetDialogs().GetBannedTable(),
                            config.GetOutput().GetDialogs().GetErrorTable(), bannedUsers, productionEnvironment);
            } else if (!config.GetInput().GetWonderlogsTables().empty()) {
                Y_ENSURE(config.GetThreadCount() > 0, "Thread count must be greater than 0");

                TVector<TString> wonderlogsTables;
                for (const auto& wonderlogsTable : config.GetInput().GetWonderlogsTables()) {
                    Y_ENSURE(!wonderlogsTable.empty(), "Path for wonderlogs table must be provided in every input");
                    wonderlogsTables.push_back(wonderlogsTable);
                }
                TVector<TString> outputTables;
                TVector<TString> bannedTables;
                TVector<TString> errorTables;
                constexpr TStringBuf dialogsType = "dialogs";
                constexpr TStringBuf bannedDialogsType = "banned dialogs";
                constexpr TStringBuf errorType = "error dialogs";
                for (auto& [from, to, type] :
                     {std::tie(config.GetOutput().GetMultipleDialogs().GetTables(), outputTables, dialogsType),
                      std::tie(config.GetOutput().GetMultipleDialogs().GetBannedTables(), bannedTables,
                               bannedDialogsType),
                      std::tie(config.GetOutput().GetMultipleDialogs().GetErrorTables(), errorTables, errorType)}) {
                    for (const auto& table : from) {
                        Y_ENSURE(!table.empty(),
                                 TStringBuilder{} << "Path for " << type << " table must be provided in every output");
                        to.push_back(table);
                    }
                }
                Y_ENSURE(wonderlogsTables.size() == outputTables.size(), "Input count must be equal to output count");
                Y_ENSURE(wonderlogsTables.size() == bannedTables.size(),
                         "Input count must be equal to banned output count");
                Y_ENSURE(wonderlogsTables.size() == errorTables.size(),
                         "Input count must be equal to error output count");

                MakeDialogsMultiple(client, config.GetInput().GetTmpDirectory(), wonderlogsTables, outputTables,
                                    bannedTables, errorTables, config.GetThreadCount(), bannedUsers,
                                    productionEnvironment);
            } else {
                Y_FAIL("Either wonderlogs table or tables must be set");
            }

            break;
        }
        case TConfig::ASR_PREPARED: {
            Y_ENSURE(!config.GetInput().GetAsrLogsTables().empty(), "ASR logs tables must be provided");
            Y_ENSURE(!config.GetInput().GetUniproxyPreparedTable().empty(),
                     "Path for uniproxy prepared table must be provided");
            Y_ENSURE(!config.GetOutput().GetAsr().GetPreparedTable().empty(),
                     "Path for ASR prepared table must be provided");
            Y_ENSURE(!config.GetOutput().GetAsr().GetErrorTable().empty(),
                     "Path for ASR error table must be provided");

            const auto timestampFrom = ParseDatetimeOrFail(config.GetDatetimeFrom());
            const auto timestampTo = ParseDatetimeOrFail(config.GetDatetimeTo());

            TVector<TString> asrLogsTables{config.GetInput().GetAsrLogsTables().begin(),
                                           config.GetInput().GetAsrLogsTables().end()};
            MakeAsrPrepared(client, config.GetInput().GetTmpDirectory(), config.GetInput().GetUniproxyPreparedTable(),
                            asrLogsTables, config.GetOutput().GetAsr().GetPreparedTable(),
                            config.GetOutput().GetAsr().GetErrorTable(), timestampFrom, timestampTo,
                            TDuration::Minutes(config.GetRequestsShiftMinutes()));
            break;
        }
        case TConfig::DIFF: {
            Y_ENSURE(!config.GetInput().GetStableTable().empty(), "Stable table must be provided");
            Y_ENSURE(!config.GetInput().GetTestTable().empty(), "Test table must be provided");
            Y_ENSURE(!config.GetOutput().GetDiffTable().empty(), "Path for diff table must be provided");

            MakeDiff(client, config.GetInput().GetTmpDirectory(), config.GetInput().GetStableTable(),
                     config.GetInput().GetTestTable(), config.GetOutput().GetDiffTable(), DEFAULT_DIFF_CONTEXT);
            break;
        }
        case TConfig::DIFF_CHANGED_FIELDS: {
            Y_ENSURE(!config.GetInput().GetStableTable().empty(), "Stable table must be provided");
            Y_ENSURE(!config.GetInput().GetTestTable().empty(), "Test table must be provided");
            Y_ENSURE(!config.GetOutput().GetDiffTable().empty(), "Path for diff table must be provided");

            MakeChangedFieldsDiff(client, config.GetInput().GetTmpDirectory(), config.GetInput().GetStableTable(),
                                  config.GetInput().GetTestTable(), config.GetOutput().GetDiffTable());
            break;
        }
        case TConfig::DIFF_WONDERLOGS: {
            Y_ENSURE(!config.GetInput().GetStableTable().empty(), "Stable table must be provided");
            Y_ENSURE(!config.GetInput().GetTestTable().empty(), "Test table must be provided");
            Y_ENSURE(!config.GetOutput().GetDiffTable().empty(), "Path for diff table must be provided");

            MakeWonderlogsDiff(client, config.GetInput().GetTmpDirectory(), config.GetInput().GetStableTable(),
                               config.GetInput().GetTestTable(), config.GetOutput().GetDiffTable());
            break;
        }
        case TConfig::JSON_WONDERLOGS: {
            Y_ENSURE(!config.GetInput().GetWonderlogsTable().empty(), "Path for wonderlogs table must be provided");

            Y_ENSURE(!config.GetOutput().GetJsonWonderlogs().GetTable().empty(),
                     "Path for json wonderlogs table must be provided");

            MakeJsonWonderlogs(client, config.GetInput().GetTmpDirectory(), config.GetInput().GetWonderlogsTable(),
                               config.GetOutput().GetJsonWonderlogs().GetTable(),
                               config.GetOutput().GetJsonWonderlogs().GetErrorTable());
            break;
        }
        case TConfig::CENSOR_WONDERLOGS: {
            Y_ENSURE(config.GetThreadCount() > 0, "Thread count must be greater than 0");
            Y_ENSURE(!config.GetInput().GetPrivateUsersTable().empty(),
                     "Path for private users table must be provided");

            TVector<TString> wonderlogsTables;
            for (const auto& wonderlogsTable : config.GetInput().GetWonderlogsTables()) {
                Y_ENSURE(!wonderlogsTable.empty(), "Path for wonderlogs table must be provided in every input");
                wonderlogsTables.push_back(wonderlogsTable);
            }
            TVector<TString> outputTables;
            for (const auto& wonderlogsTable : config.GetOutput().GetWonderlogsTables()) {
                Y_ENSURE(!wonderlogsTable.empty(), "Path for wonderlogs table must be provided in every output");
                outputTables.push_back(wonderlogsTable);
            }
            Y_ENSURE(wonderlogsTables.size() == outputTables.size(), "Input count must be equal to output count");
            CensorWonderlogs(client, config.GetInput().GetTmpDirectory(), wonderlogsTables, outputTables,
                             config.GetInput().GetPrivateUsersTable(), config.GetThreadCount());
            break;
        }
        case TConfig::CENSOR_DIALOGS: {
            Y_ENSURE(config.GetThreadCount() > 0, "Thread count must be greater than 0");
            Y_ENSURE(!config.GetInput().GetPrivateUsersTable().empty(),
                     "Path for private users table must be provided");

            TVector<TString> dialogsTables;
            for (const auto& dialogsTable : config.GetInput().GetDialogsTables()) {
                Y_ENSURE(!dialogsTable.empty(), "Path for wonderlogs table must be provided in every input");
                dialogsTables.push_back(dialogsTable);
            }
            TVector<TString> outputTables;
            for (const auto& dialogsTable : config.GetOutput().GetMultipleDialogs().GetTables()) {
                Y_ENSURE(!dialogsTable.empty(), "Path for dialogs table must be provided in every output");
                outputTables.push_back(dialogsTable);
            }
            Y_ENSURE(dialogsTables.size() == outputTables.size(), "Input count must be equal to output count");
            CensorDialogs(client, config.GetInput().GetTmpDirectory(), dialogsTables, outputTables,
                          config.GetInput().GetPrivateUsersTable(), config.GetThreadCount());
            break;
        }
        case TConfig::BASKET_DATA: {
            Y_ENSURE(!config.GetInput().GetWonderlogsTables().empty(), "Path for wonderlogs table must be provided");

            Y_ENSURE(!config.GetOutput().GetBasketData().GetTable().empty(),
                     "Path for basket data table must be provided");
            Y_ENSURE(!config.GetOutput().GetBasketData().GetErrorTable().empty(),
                     "Path for error basket data table must be provided");

            TVector<TString> wonderlogsTables;
            for (const auto& wonderlogsTable : config.GetInput().GetWonderlogsTables()) {
                Y_ENSURE(!wonderlogsTable.empty(), "Path for wonderlogs table must be provided in every input");
                wonderlogsTables.push_back(wonderlogsTable);
            }

            ExtractYsonDataFromWonderlogs(client, wonderlogsTables, config.GetOutput().GetBasketData().GetTable(),
                                          config.GetOutput().GetBasketData().GetErrorTable());
            break;
        }
        case TConfig::CHANGED_WONDERLOGS_SCHEMA: {
            Y_ENSURE(!config.GetInput().GetWonderlogsTable().empty(), "Path for wonderlogs table must be provided");

            Y_ENSURE(!config.GetOutput().GetWonderlogs().GetTable().empty(),
                     "Path for changed wonderlogs schema table must be provided");
            Y_ENSURE(!config.GetOutput().GetWonderlogs().GetErrorTable().empty(),
                     "Path for error changed wonderlogs schema table must be provided");

            ChangeWonderlogsSchema(client, config.GetInput().GetWonderlogsTable(),
                                   config.GetOutput().GetWonderlogs().GetTable(),
                                   config.GetOutput().GetWonderlogs().GetErrorTable());
            break;
        }
        case TConfig::EXPBOXES: {
            Y_ENSURE(!config.GetInput().GetWonderlogsTable().empty(), "Path for wonderlogs table must be provided");

            Y_ENSURE(!config.GetOutput().GetExpboxes().GetTable().empty(),
                     "Path for expboxes schema table must be provided");
            Y_ENSURE(!config.GetOutput().GetExpboxes().GetErrorTable().empty(),
                     "Path for error expboxes table must be provided");

            MakeExpboxes(client, config.GetInput().GetTmpDirectory(), config.GetInput().GetWonderlogsTable(),
                         config.GetOutput().GetExpboxes().GetTable(),
                         config.GetOutput().GetExpboxes().GetErrorTable());
            break;
        }
        default:
            Y_FAIL("OutputTableType must be set");
    }

    return 0;
}
