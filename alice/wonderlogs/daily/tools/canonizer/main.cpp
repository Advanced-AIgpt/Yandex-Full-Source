#include <alice/wonderlogs/daily/tools/canonizer/config.pb.h>

#include <alice/wonderlogs/daily/lib/asr_prepared.h>
#include <alice/wonderlogs/daily/lib/dialogs.h>
#include <alice/wonderlogs/daily/lib/expboxes.h>
#include <alice/wonderlogs/daily/lib/json_wonderlogs.h>
#include <alice/wonderlogs/daily/lib/megamind_prepared.h>
#include <alice/wonderlogs/daily/lib/uniproxy_prepared.h>
#include <alice/wonderlogs/daily/lib/wonderlogs.h>
#include <alice/wonderlogs/protos/asr_prepared.pb.h>

#include <alice/wonderlogs/protos/banned_user.pb.h>
#include <alice/wonderlogs/protos/error_threshold_config.pb.h>
#include <alice/wonderlogs/protos/megamind_prepared.pb.h>
#include <alice/wonderlogs/protos/private_user.pb.h>
#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>
#include <alice/wonderlogs/protos/wonderlogs.pb.h>

#include <alice/megamind/protos/speechkit/request.pb.h>

#include <alice/library/json/json.h>

#include <google/protobuf/util/json_util.h>

#include <library/cpp/getoptpb/getoptpb.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/yson/json2yson.h>
#include <library/cpp/yson/node/node_io.h>

#include <mapreduce/yt/common/config.h>
#include <mapreduce/yt/interface/client.h>

#include <util/folder/path.h>

#include <cstdlib>

using namespace NAlice::NWonderlogs;

namespace {

template <class T, typename TComparator = void>
void DownloadProto(NYT::IClientPtr client, const TString& ytPath, const TString& filepath,
                   TComparator* comparator = nullptr) {
    TVector<T> rows;
    for (auto reader = client->CreateTableReader<T>(ytPath); reader->IsValid(); reader->Next()) {
        rows.push_back(reader->GetRow());
    }

    if constexpr (!std::is_same_v<void, TComparator>) {
        if (comparator) {
            Sort(rows, *comparator);
        }
    }
    TFileOutput out{filepath};
    bool first = true;
    for (const auto& row : rows) {
        if (!first) {
            out << Endl;
        }
        first = false;
        TString s;
        google::protobuf::util::MessageToJsonString(row, &s);
        out << s;
    }
}

void DownloadYson(NYT::IClientPtr client, const TString& ytPath, const TString& filepath,
                  std::function<bool(const NYT::TNode&, const NYT::TNode&)>* comparator = nullptr) {
    NYT::TNode rows = NYT::TNode::CreateList();
    for (auto reader = client->CreateTableReader<NYT::TNode>(ytPath); reader->IsValid(); reader->Next()) {
        rows.AsList().push_back(reader->GetRow());
    }
    if (comparator) {
        Sort(rows.AsList(), *comparator);
    }
    bool first = true;
    TFileOutput out{filepath};
    for (const auto& row : rows.AsList()) {
        if (!first) {
            out << Endl;
        }
        first = false;
        NYT::NodeToYsonStream(row, &out, NYson::EYsonFormat::Pretty);
        out << ';';
    }
}

TString TmpDir(const TString& wonderDir) {
    return wonderDir + "/tmp";
}

TString UniproxyRawLogs(const TString& rawLogsDir) {
    return rawLogsDir + "/uniproxy";
}

TString UniproxyPrepared(const TString& wonderDir) {
    return wonderDir + "/uniproxy-prepared";
}

TString UniproxyError(const TString& wonderDir) {
    return wonderDir + "/error/uniproxy-prepared";
}

TString MegamindPrepared(const TString& wonderDir) {
    return wonderDir + "/megamind-prepared";
}

TString MegamindRawLogs(const TString& rawLogsDir) {
    return rawLogsDir + "/private-megamind";
}

TString MegamindError(const TString& wonderDir) {
    return wonderDir + "/error/megamind-prepared";
}

TString AsrPrepared(const TString& wonderDir) {
    return wonderDir + "/asr-prepared";
}

TString AsrRawLogs(const TString& rawLogsDir) {
    return rawLogsDir + "/asr";
}

TString AsrError(const TString& wonderDir) {
    return wonderDir + "/error/asr-prepared";
}

TString Wonderlogs(const TString& wonderDir) {
    return wonderDir + "/logs";
}

TString WonderlogsError(const TString& wonderDir) {
    return wonderDir + "/error/logs";
}

TString RobotWonderlogs(const TString& wonderDir) {
    return wonderDir + "/robot-logs";
}

TString PrivateWonderlogs(const TString& wonderDir) {
    return wonderDir + "/private-logs";
}

TString CensoredWonderlogs(const TString& wonderDir) {
    return wonderDir + "/censored-logs";
}

TString AddDate(const TString& path, const TString& date) {
    return path + "/" + date;
}

TString Dialogs(const TString& wonderDir) {
    return wonderDir + "/dialogs";
}

TString DialogsError(const TString& wonderDir) {
    return wonderDir + "/error/dialogs";
}

TString RobotDialogs(const TString& wonderDir) {
    return wonderDir + "/robot-dialogs";
}

TString RobotDialogsError(const TString& wonderDir) {
    return wonderDir + "/error/robot-dialogs";
}

TString RobotBannedDialogs(const TString& wonderDir) {
    return wonderDir + "/robot-banned-dialogs";
}

TString BannedDialogs(const TString& wonderDir) {
    return wonderDir + "/banned-dialogs";
}

TString CensoredDialogs(const TString& wonderDir) {
    return wonderDir + "/censored-dialogs";
}

TString PrivateUsers(const TString& rawLogsDir) {
    return rawLogsDir + "/private-users";
}

TString Add1(const TString& path) {
    return path + "-1";
}
TString Add2(const TString& path) {
    return path + "-2";
}

TString Expboxes(const TString& wonderDir) {
    return wonderDir + "/expboxes";
}

TString ExpboxesError(const TString& wonderDir) {
    return wonderDir + "/error/expboxes";
}

} // namespace

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);
    TConfig config = NGetoptPb::GetoptPbOrAbort(argc, argv);

    Y_ENSURE(!config.GetYt().GetCluster().empty(), "Cluster must be provided");
    auto client = NYT::CreateClient(config.GetYt().GetCluster());
    if (!config.GetYt().GetPool().empty()) {
        NYT::TConfig::Get()->Pool = config.GetYt().GetPool();
    }

    Y_ENSURE(config.GetStableSandboxResourceId() != 0, "Stable Sandbox resource must be provided");

    const auto wonderDir = config.GetYt().GetWonderDir();
    const auto rawLogsDir = config.GetYt().GetRawLogsDir();
    const auto date = config.GetDate();

    const auto shiftMinutes = TDuration::Minutes(10);

    const auto tmpDir = TmpDir(wonderDir);
    const auto uniproxyPrepared = AddDate(UniproxyPrepared(wonderDir), date);
    const auto uniproxyError = AddDate(UniproxyError(wonderDir), date);
    const TVector<TString> uniproxyRawLogs{AddDate(UniproxyRawLogs(rawLogsDir), date)};

    const auto megamindPrepared = AddDate(MegamindPrepared(wonderDir), date);
    const auto megamindError = AddDate(MegamindError(wonderDir), date);
    const TVector<TString> megamindRawLogs{AddDate(MegamindRawLogs(rawLogsDir), date)};

    const auto asrPrepared = AddDate(AsrPrepared(wonderDir), date);
    const auto asrError = AddDate(AsrError(wonderDir), date);
    const TVector<TString> asrRawLogs{AddDate(AsrRawLogs(rawLogsDir), date)};

    const auto wonderlogs = AddDate(Wonderlogs(wonderDir), date);
    const auto wonderlogsError = AddDate(WonderlogsError(wonderDir), date);
    const auto robotWonderlogs = AddDate(RobotWonderlogs(wonderDir), date);
    const auto privateWonderlogs = AddDate(PrivateWonderlogs(wonderDir), date);

    const auto dialogs = AddDate(Dialogs(wonderDir), date);
    const auto dialogsError = AddDate(DialogsError(wonderDir), date);
    const auto bannedDialogs = AddDate(BannedDialogs(wonderDir), date);

    const auto robotDialogs = AddDate(RobotDialogs(wonderDir), date);
    const auto robotDialogsError = AddDate(RobotDialogsError(wonderDir), date);
    const auto robotBannedDialogs = AddDate(RobotBannedDialogs(wonderDir), date);

    const auto wonderlogs1 = Add1(wonderlogs);
    const auto wonderlogs2 = Add2(wonderlogs);

    const auto censoredWonderlogs1 = Add1(AddDate(CensoredWonderlogs(wonderDir), date));
    const auto censoredWonderlogs2 = Add2(AddDate(CensoredWonderlogs(wonderDir), date));

    const auto dialogs1 = Add1(dialogs);
    const auto dialogs2 = Add2(dialogs);

    const auto censoredDialogs1 = Add1(AddDate(CensoredDialogs(wonderDir), date));
    const auto censoredDialogs2 = Add2(AddDate(CensoredDialogs(wonderDir), date));

    const auto bannedDialogs1 = Add1(AddDate(BannedDialogs(wonderDir), date));
    const auto bannedDialogs2 = Add2(AddDate(BannedDialogs(wonderDir), date));

    const auto dialogsError1 = Add1(AddDate(DialogsError(wonderDir), date));
    const auto dialogsError2 = Add2(AddDate(DialogsError(wonderDir), date));

    const auto privateUsers = AddDate(PrivateUsers(rawLogsDir), date);

    const auto expboxes = AddDate(Expboxes(wonderDir), date);
    const auto expboxesError = AddDate(ExpboxesError(wonderDir), date);

    // TODO(ran1s) make better
    const auto timestampFrom = ParseDatetimeOrFail(config.GetDate() + " 00:00:00+0300");
    const auto timestampTo = timestampFrom + TDuration::Days(1);

    const size_t threadCount = 2;

    NAlice::NWonderlogs::TBannedUsers bannedUsers;
    {
        const auto& inputBannedUsers = config.GetBannedUsers();
        for (auto& [from, to] : {std::tie(inputBannedUsers.GetByIp(), bannedUsers.Ips),
                                 std::tie(inputBannedUsers.GetByUuid(), bannedUsers.Uuids),
                                 std::tie(inputBannedUsers.GetByDeviceId(), bannedUsers.DeviceIds)}) {
            for (const auto& value : from) {
                to.insert(value);
            }
        }
    }

    const i64 splitServerTimeMs = config.GetSplitServerTimeMs();

    // TODO(ran1s) either get from options or from reactor
    TEnvironment environment{.UniproxyQloudProjects = {"voice-ext", "alice", "unknown"},
                             .UniproxyQloudApplications = {"uniproxy"},
                             .MegamindEnvironments = {"megamind_standalone_man", "megamind_standalone_vla",
                                                      "megamind_standalone_sas", "stable"}};

    Cout << "Started uniproxy prepared" << Endl;
    MakeUniproxyPrepared(client, tmpDir, uniproxyRawLogs, uniproxyPrepared, uniproxyError, timestampFrom, timestampTo,
                         shiftMinutes);
    Cout << "Finished uniproxy prepared" << Endl;

    Cout << "Started megamind prepared" << Endl;
    MakeMegamindPrepared(client, tmpDir, uniproxyPrepared, megamindRawLogs, megamindPrepared, megamindError,
                         timestampFrom, timestampTo);
    Cout << "Finished megamind prepared" << Endl;

    Cout << "Started asr prepared" << Endl;
    MakeAsrPrepared(client, tmpDir, uniproxyPrepared, asrRawLogs, asrPrepared, asrError, timestampFrom, timestampTo,
                    shiftMinutes);
    Cout << "Finished asr prepared" << Endl;

    Cout << "Started wonderlogs" << Endl;
    MakeWonderlogs(client, tmpDir, uniproxyPrepared, megamindPrepared, asrPrepared, wonderlogs, privateWonderlogs,
                   robotWonderlogs, wonderlogsError, timestampFrom, timestampTo, shiftMinutes, environment);
    Cout << "Finished wonderlogs" << Endl;

    Cout << "Started dialogs" << Endl;
    MakeDialogs(client, tmpDir, wonderlogs, dialogs, bannedDialogs, dialogsError, bannedUsers, environment);
    Cout << "Finished dialogs" << Endl;

    Cout << "Started robot dialogs" << Endl;
    MakeDialogs(client, tmpDir, robotWonderlogs, robotDialogs, robotBannedDialogs, robotDialogsError,
                /* bannedUsers= */ {}, /* productionEnvironment= */ {});
    Cout << "Finished robot dialogs" << Endl;

    Cout << "Started expboxes" << Endl;
    MakeExpboxes(client, tmpDir, wonderlogs, expboxes, expboxesError);
    Cout << "Finished expboxes" << Endl;

    {
        Cout << "Started splitting wonderlogs" << Endl;
        auto writer1 =
            client->CreateTableWriter<TWonderlog>(wonderlogs1, NYT::TTableWriterOptions{}.InferSchema(true));
        auto writer2 =
            client->CreateTableWriter<TWonderlog>(wonderlogs2, NYT::TTableWriterOptions{}.InferSchema(true));
        for (auto reader = client->CreateTableReader<TWonderlog>(wonderlogs); reader->IsValid(); reader->Next()) {
            const auto& wonderlog = reader->GetRow();
            if (wonderlog.GetServerTimeMs() < splitServerTimeMs) {
                writer1->AddRow(wonderlog);
            } else {
                writer2->AddRow(wonderlog);
            }
        }
        Cout << "Finished splitting wonderlogs" << Endl;
    }
    Cout << "Started censoring wonderlogs" << Endl;
    client->Create(CensoredWonderlogs(wonderDir), NYT::ENodeType::NT_MAP,
                   NYT::TCreateOptions{}.Recursive(true).Force(true));
    CensorWonderlogs(client, tmpDir, {wonderlogs1, wonderlogs2}, {censoredWonderlogs1, censoredWonderlogs2},
                     privateUsers, threadCount);
    Cout << "Finished censoring wonderlogs" << Endl;

    Cout << "Started dialogs multiple" << Endl;
    MakeDialogsMultiple(client, tmpDir, {wonderlogs1, wonderlogs2}, {dialogs1, dialogs2},
                        {bannedDialogs1, bannedDialogs2}, {dialogsError1, dialogsError2}, threadCount,
                        /* bannedUsers= */ {}, /* productionEnvironment= */ {});
    Cout << "Finished dialogs multiple" << Endl;

    Cout << "Started censoring dialogs" << Endl;
    client->Create(CensoredDialogs(wonderDir), NYT::ENodeType::NT_MAP,
                   NYT::TCreateOptions{}.Recursive(true).Force(true));
    CensorDialogs(client, tmpDir, {dialogs1, dialogs2}, {censoredDialogs1, censoredDialogs2}, privateUsers,
                  threadCount);
    Cout << "Finished censoring dialogs" << Endl;

    const TFsPath outputDir(config.GetOutputDir());
    const auto stableDir = outputDir / "stable";
    const auto testDir = outputDir / "test";
    stableDir.MkDir();
    testDir.MkDir();

    const auto uuidAndMessageIdComparator = [](const auto& lhs, const auto& rhs) {
        if (lhs.GetUuid() != rhs.GetUuid()) {
            return lhs.GetUuid() < rhs.GetUuid();
        }

        return lhs.GetMessageId() < rhs.GetMessageId();
    };

    const auto uuidAndSkRequestIdComparator = [](const auto& lhs, const auto& rhs) {
        if (lhs.GetUuid() != rhs.GetUuid()) {
            return lhs.GetUuid() < rhs.GetUuid();
        }
        return lhs.GetSpeechkitRequest().GetHeader().GetRequestId() <
               rhs.GetSpeechkitRequest().GetHeader().GetRequestId();
    };

    const auto uuidAndMessageIdAndMessageComparator = [](const auto& lhs, const auto& rhs) {
        if (lhs.GetUuid() != rhs.GetUuid()) {
            return lhs.GetUuid() < rhs.GetUuid();
        }

        if (lhs.GetMessageId() != rhs.GetMessageId()) {
            return lhs.GetMessageId() < rhs.GetMessageId();
        }

        return lhs.GetMessage() < rhs.GetMessage();
    };

    const auto uniproxyRawLogsFile = testDir / "uniproxy_events.yson";
    const auto uniproxyPreparedFile = testDir / "uniproxy_prepared.jsonlines";
    const auto uniproxyErrorFile = testDir / "uniproxy_error.jsonlines";

    const auto megamindRawLogsFile = testDir / "megamind_analytics_logs.yson";
    const auto megamindPreparedFile = testDir / "megamind_prepared.jsonlines";
    const auto megamindErrorFile = testDir / "megamind_error.jsonlines";

    const auto asrRawLogsFile = testDir / "asr_logs.yson";
    const auto asrPreparedFile = testDir / "asr_prepared.jsonlines";
    const auto asrErrorFile = testDir / "asr_error.jsonlines";

    const auto wonderlogsFile = testDir / "wonderlogs.jsonlines";
    const auto robotWonderlogsFile = testDir / "robot_wonderlogs.jsonlines";
    const auto privateWonderlogsFile = testDir / "private_wonderlogs.jsonlines";
    const auto wonderlogsErrorFile = testDir / "wonderlogs_error.jsonlines";

    const auto wonderlogs1File = testDir / "wonderlogs1.jsonlines";
    const auto wonderlogs2File = testDir / "wonderlogs2.jsonlines";

    const auto censoredWonderlogs1File = testDir / "censored_wonderlogs1.jsonlines";
    const auto censoredWonderlogs2File = testDir / "censored_wonderlogs2.jsonlines";

    const auto dialogsFile = testDir / "dialogs.yson";
    const auto dialogsErrorFile = testDir / "dialogs_error.yson";
    const auto bannedDialogsFile = testDir / "banned_dialogs.yson";

    const auto expboxesFile = testDir / "expboxes.yson";
    const auto expboxesErrorFile = testDir / "expboxes_error.yson";

    const auto robotDialogsFile = testDir / "robot_dialogs.yson";
    const auto robotDialogsErrorFile = testDir / "robot_dialogs_error.yson";

    const auto dialogs1File = testDir / "dialogs1.yson";
    const auto dialogs2File = testDir / "dialogs2.yson";

    const auto censoredDialogs1File = testDir / "censored_dialogs1.yson";
    const auto censoredDialogs2File = testDir / "censored_dialogs2.yson";

    const auto privateUsersFile = testDir / "private_users.jsonlines";

    Cout << "Downloading tables started" << Endl;

    DownloadYson(client, uniproxyRawLogs[0], uniproxyRawLogsFile);
    DownloadProto<TUniproxyPrepared>(client, uniproxyPrepared, uniproxyPreparedFile, &uuidAndMessageIdComparator);
    DownloadProto<TUniproxyPrepared::TError>(client, uniproxyError, uniproxyErrorFile,
                                             &uuidAndMessageIdAndMessageComparator);

    DownloadYson(client, megamindRawLogs[0], megamindRawLogsFile);
    DownloadProto<TMegamindPrepared>(client, megamindPrepared, megamindPreparedFile, &uuidAndSkRequestIdComparator);
    DownloadProto<TMegamindPrepared::TError>(client, megamindError, megamindErrorFile, &uuidAndMessageIdComparator);

    DownloadYson(client, asrRawLogs[0], asrRawLogsFile);
    DownloadProto<TAsrPrepared>(client, asrPrepared, asrPreparedFile, &uuidAndMessageIdComparator);
    DownloadProto<TAsrPrepared::TError>(client, asrError, asrErrorFile, &uuidAndMessageIdComparator);

    DownloadProto<TWonderlog>(client, wonderlogs, wonderlogsFile);
    DownloadProto<TWonderlog>(client, robotWonderlogs, robotWonderlogsFile);
    DownloadProto<TWonderlog>(client, privateWonderlogs, privateWonderlogsFile);
    DownloadProto<TWonderlog::TError>(client, wonderlogsError, wonderlogsErrorFile, &uuidAndMessageIdComparator);

    DownloadProto<TWonderlog>(client, wonderlogs1, wonderlogs1File);
    DownloadProto<TWonderlog>(client, wonderlogs2, wonderlogs2File);

    DownloadProto<TWonderlog>(client, censoredWonderlogs1, censoredWonderlogs1File);
    DownloadProto<TWonderlog>(client, censoredWonderlogs2, censoredWonderlogs2File);

    DownloadYson(client, dialogs, dialogsFile);
    DownloadYson(client, dialogsError, dialogsErrorFile);
    DownloadYson(client, bannedDialogs, bannedDialogsFile);

    DownloadYson(client, expboxes, expboxesFile);
    DownloadYson(client, expboxesError, expboxesErrorFile);

    DownloadYson(client, robotDialogs, robotDialogsFile);
    DownloadYson(client, robotDialogsError, robotDialogsErrorFile);

    DownloadYson(client, dialogs1, dialogs1File);
    DownloadYson(client, dialogs2, dialogs2File);

    DownloadYson(client, censoredDialogs1, censoredDialogs1File);
    DownloadYson(client, censoredDialogs2, censoredDialogs2File);

    DownloadProto<TPrivateUser>(client, privateUsers, privateUsersFile);

    Cout << "Downloading tables finished" << Endl;

    const TVector allFiles = {uniproxyRawLogsFile,
                              uniproxyPreparedFile,
                              uniproxyErrorFile,
                              megamindRawLogsFile,
                              megamindPreparedFile,
                              megamindErrorFile,
                              asrRawLogsFile,
                              asrPreparedFile,
                              asrErrorFile,
                              wonderlogsFile,
                              robotWonderlogsFile,
                              privateWonderlogsFile,
                              wonderlogsErrorFile,
                              wonderlogs1File,
                              wonderlogs2File,
                              censoredWonderlogs1File,
                              censoredWonderlogs2File,
                              dialogsFile,
                              dialogsErrorFile,
                              bannedDialogsFile,
                              robotDialogsFile,
                              robotDialogsErrorFile,
                              dialogs1File,
                              dialogs2File,
                              censoredDialogs1File,
                              censoredDialogs2File,
                              privateUsersFile,
                              expboxesFile,
                              expboxesErrorFile};

    const auto wonderlogsArchiveFile = testDir / "wonderlogs.tar.gz";
    auto makeArchiveBuilder = TStringBuilder{} << "tar -czvf " << wonderlogsArchiveFile.GetPath() << " --directory "
                                               << testDir.GetPath();

    for (const auto& file : allFiles) {
        makeArchiveBuilder << " " << file.Basename();
    }
    const auto makeArchiveCommand = ToString(makeArchiveBuilder);
    Cout << "Creating archive" << Endl;
    Cout << makeArchiveCommand << Endl;
    std::system(makeArchiveCommand.c_str());
    Cout << "Archive has been created" << Endl;

    Cout << "Uploading archive" << Endl;
    const auto uploadArchiveCommand =
        ToString(TStringBuilder{} << "ya upload " << wonderlogsArchiveFile.GetPath() << " --ttl=inf");
    Cout << uploadArchiveCommand << Endl;
    std::system(uploadArchiveCommand.c_str());
    Cout << "Archive has been uploaded" << Endl;

    Cout << "Downloading stable archive" << Endl;
    const auto downloadArchiveComand =
        ToString(TStringBuilder{} << "ya download " << config.GetStableSandboxResourceId()
                                  << " --output=" << stableDir.GetPath() << " --untar --overwrite");
    Cout << downloadArchiveComand << Endl;
    std::system(downloadArchiveComand.c_str());
    Cout << "Stable archive has been downloaded" << Endl;

    Cout << "Please run commands and check diff:" << Endl;
    for (const auto& file : allFiles) {
        Cout << "git diff " << stableDir / file.Basename() << " " << file.GetPath() << Endl;
    }

    return 0;
}
