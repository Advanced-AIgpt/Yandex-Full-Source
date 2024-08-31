#include "collect_learn.h"

#include <library/cpp/logger/global/global.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/library/operation_tracker/operation_tracker.h>
#include <mapreduce/yt/util/temp_table.h>
#include <mapreduce/yt/util/ypath_join.h>

#include <util/draft/date.h>
#include <util/generic/hash_set.h>
#include <util/generic/size_literals.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>

namespace NAlice {
    namespace {
        constexpr TStringBuf GENERAL_BASKETS = "//home/voice/dialog/toloka/alice_quality_v2";
        constexpr TStringBuf QUASAR_BASKETS = "//home/voice/dialog/toloka/quasar_quality_results";
        constexpr TStringBuf FULL_LOGS = "//home/voice/vins/logs/dialogs";
        constexpr TStringBuf VOICE_LOGS = "//home/voice-speechbase/uniproxy/logs_v4";
        constexpr TStringBuf ID_MASK = "ffffffff-ffff-ffff";
        constexpr std::array<TStringBuf, 3> DEVICES{
            TStringBuf("browser_prod"),
            TStringBuf("search_app_prod"),
            TStringBuf("stroka"),
        };
        constexpr TStringBuf SESSION_LOGS = "//home/voice/dialog/sessions/";

        TString GetMaskedRequestId(TString id)
        {
            return id.replace(0, ID_MASK.size(), ID_MASK);
        }

        class TCollectSessionMapper: public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
        public:
            TCollectSessionMapper() = default;
            explicit TCollectSessionMapper(const THashSet<TString>& neededIdsSet, ui32 maxRequestDepth = 2)
                : NeededIdsSet(neededIdsSet)
                , MaxRequestDepth(maxRequestDepth)
            {
            }

            void Do(TReader* reader, TWriter* writer) override
            {
                for (; reader->IsValid(); reader->Next()) {
                    const auto& curRow = reader->GetRow();
                    const auto& session = curRow["session"];
                    for (ui32 depth = 0; depth < MaxRequestDepth && depth < session.AsList().size(); ++depth) {
                        const auto& request = session[depth];
                        const auto& requestId = request["req_id"].AsString();
                        if (NeededIdsSet.contains(requestId)) {
                            if (depth == 0) {
                                const auto& oldId = requestId;
                                const auto newId = GetMaskedRequestId(oldId);
                                writer->AddRow(CreateRow(curRow, session[0], 0, oldId, newId, newId));
                            } else {
                                const auto& request0 = session[depth - 1];
                                const auto& oldId0 = request0["req_id"].AsString();
                                const auto& oldId1 = requestId;
                                const auto newId0 = GetMaskedRequestId(oldId0);
                                const auto newId1 = GetMaskedRequestId(oldId1);
                                const auto sessionId = TString::Join(newId0, "__", newId1);
                                writer->AddRow(CreateRow(curRow, session[depth - 1], 0, oldId0, newId0, sessionId));
                                writer->AddRow(CreateRow(curRow, session[depth], 1, oldId1, newId1, sessionId));
                            }
                        }
                    }
                }
            }

        public:
            Y_SAVELOAD_JOB(NeededIdsSet, MaxRequestDepth);

        private:
            THashSet<TString> NeededIdsSet;
            ui32 MaxRequestDepth;

        private:
            static NYT::TNode CreateRow(const NYT::TNode& curRow, const NYT::TNode& request, const ui32 sessionSequence,
                const TString& lastReqId, const TString& reqId, const TString& sessionId)
            {
                NYT::TNode newRow;
                NYT::TNode experiments;
                for (const auto& experiment : curRow["experiments"].AsList()) {
                    experiments[experiment.AsString()] = true;
                }
                newRow["real_req_id"] = request["req_id"];
                newRow["real_session_id"] = curRow["session_id"];
                newRow["request_id"] = reqId;
                newRow["session_id"] = sessionId;
                newRow["session_sequence"] = sessionSequence;
                newRow["experiments"] = experiments;
                newRow["text"] = request["_query"];
                newRow["app_preset"] = curRow["app"];
                newRow["last_req_id"] = lastReqId;
                newRow["vins_intent"] = request["intent"];
                newRow["fetcher_mode"] = request["type"]; // TODO find out what to do with click
                newRow["toloka_intent"] = "";
                return newRow;
            }
        };
        REGISTER_MAPPER(TCollectSessionMapper);

        class TAddDeviceStateAndVoiceMapper: public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
        public:
            TAddDeviceStateAndVoiceMapper() = default;
            explicit TAddDeviceStateAndVoiceMapper(const THashSet<TString>& idSet)
                : RequestIdSet(idSet)
            {
            }

            void Do(TReader* reader, TWriter* writer) override
            {
                for (; reader->IsValid(); reader->Next()) {
                    const auto& row = reader->GetRow();
                    auto tableIndex = reader->GetTableIndex();
                    bool gotEndOfUtt = false;
                    if (tableIndex == 0) { // basket
                        writer->AddRow(row);
                    } else if (tableIndex == 1) { // full logs
                        if (!RequestIdSet.contains(row["request"]["request_id"].AsString())) {
                            continue;
                        }
                        NYT::TNode resRow;
                        resRow["device_state"] = row["request"]["device_state"];
                        resRow["real_req_id"] = row["request"]["request_id"];
                        writer->AddRow(resRow);
                    } else if (tableIndex == 2) { // voice logs
                        if (row["mds_key"].IsNull() || row["mds_key"].AsString().size() == 0 || row["vins_request_id"].IsNull()) {
                            continue;
                        }
                        if (!RequestIdSet.contains(row["vins_request_id"].AsString()) || gotEndOfUtt) {
                            continue;
                        }
                        NYT::TNode resRow;
                        resRow["voice_url"] = VoiceUrlPrefix + row["mds_key"].AsString();
                        resRow["real_req_id"] = row["vins_request_id"];
                        resRow["end_of_utt"] = row["end_of_utt"];
                        gotEndOfUtt = row["end_of_utt"].AsBool();
                        writer->AddRow(resRow);
                    }
                }
            }

        public:
            Y_SAVELOAD_JOB(RequestIdSet);

        private:
            THashSet<TString> RequestIdSet;
            const TString VoiceUrlPrefix = "https://speechbase-yt.voicetech.yandex.net/getfile/";
        };
        REGISTER_MAPPER(TAddDeviceStateAndVoiceMapper);

        class TAddDeviceStateAndVoiceReducer: public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
        public:
            void Do(TReader* reader, TWriter* writer) override
            {
                NYT::TNode resRow;
                NYT::TNode deviceState;
                TString voiceURL = "";
                for (; reader->IsValid(); reader->Next()) {
                    const auto row = reader->GetRow();
                    if (row.HasKey("device_state")) {
                        deviceState = row;
                    } else if (row.HasKey("voice_url")) {
                        if (voiceURL.size() == 0 || row["end_of_utt"].AsBool()) {
                            voiceURL = row["voice_url"].AsString();
                        }
                    } else {
                        resRow = row;
                    }
                }
                if (deviceState.HasKey("device_state")) {
                    resRow["device_state"] = deviceState["device_state"];
                }
                resRow["voice_url"] = voiceURL;
                writer->AddRow(resRow);
            }
        };
        REGISTER_REDUCER(TAddDeviceStateAndVoiceReducer);

        void FinishTransaction(const NYT::IOperationPtr operation, THashMap<NYT::IOperationPtr,
            NYT::ITransactionPtr>& transactions, THashMap<NYT::IOperationPtr, TDate>& dates)
        {
            const auto transaction = transactions.at(operation);
            switch (operation->GetBriefState()) {
                case NYT::EOperationBriefState::Completed:
                    transaction->Commit();
                    break;
                case NYT::EOperationBriefState::Aborted:
                case NYT::EOperationBriefState::Failed:
                    transaction->Abort();
                    ERROR_LOG << "error on date: " << dates[operation].ToStroka();
                    break;
                case NYT::EOperationBriefState::InProgress:
                    Y_UNREACHABLE();
            }
            dates.erase(operation);
            transactions.erase(operation);
        }

        THashSet<TString> CollectDateIdsGeneral(const TString& date, NYT::IClientBasePtr client)
        {
            TVector<TString> paths(DEVICES.size());
            for (size_t i = 0; i < DEVICES.size(); ++i) {
                paths[i] = NYT::JoinYPaths(TString(GENERAL_BASKETS), DEVICES[i], date);
            }
            THashSet<TString> ids;
            for (const auto& path : paths) {
                auto reader = client->CreateTableReader<NYT::TNode>(path);
                for (; reader->IsValid(); reader->Next()) {
                    const auto& id = reader->GetRow()["info"]["request_id"].AsString();
                    ids.insert(id);
                }
            }
            return ids;
        }

        THashSet<TString> CollectDateIdsQuasar(const TString& date, NYT::IClientBasePtr client)
        {
            const auto path = NYT::JoinYPaths(QUASAR_BASKETS, date);
            THashSet<TString> ids;
            auto reader = client->CreateTableReader<NYT::TNode>(path);
            for (; reader->IsValid(); reader->Next()) {
                const auto& id = reader->GetRow()["req_id"].AsString();
                ids.insert(id);
            }
            return ids;
        }

        THashSet<TString> CollectContextIds(const TString& path, NYT::IClientBasePtr client)
        {
            THashSet<TString> ids;
            auto reader = client->CreateTableReader<NYT::TNode>(path);
            for (; reader->IsValid(); reader->Next()) {
                const auto& id = reader->GetRow()["real_req_id"].AsString();
                ids.insert(id);
            }
            return ids;
        }

        NYT::IOperationPtr CollectDate(const TString& date, const NYT::TYPath& outputPath,
            NYT::IClientBasePtr client, const THashSet<TString>& idSet, ui32 maxContextDepth)
        {
            const NYT::TYPath inputPath = NYT::JoinYPaths(SESSION_LOGS, date);
            auto op = client->Map(
                            NYT::TMapOperationSpec()
                                .AddInput<NYT::TNode>(inputPath)
                                .AddOutput<NYT::TNode>(outputPath),
                            new TCollectSessionMapper(idSet, maxContextDepth),
                            NYT::TOperationOptions()
                                .Wait(false)
                                .MountSandboxInTmpfs(true));
            return op;
        }

        NYT::IOperationPtr AddDeviceStateAndVoice(const TString& date, const NYT::TYPath& basket,
                                                  const THashSet<TString> idSet, NYT::IClientBasePtr client)
        {
            const NYT::TYPath fullLogsPath = NYT::JoinYPaths(FULL_LOGS, date);
            const NYT::TYPath voiceLogsPath = NYT::JoinYPaths(VOICE_LOGS, date);
            auto op = client->MapReduce(
                            NYT::TMapReduceOperationSpec()
                                .ReduceBy({"real_req_id"})
                                .AddInput<NYT::TNode>(basket) // 0
                                .AddInput<NYT::TNode>(fullLogsPath) // 1
                                .AddInput<NYT::TNode>(voiceLogsPath) // 2
                                .AddOutput<NYT::TNode>(basket)
                                .DataSizePerMapJob(2_GB),
                            new TAddDeviceStateAndVoiceMapper(idSet),
                            new TAddDeviceStateAndVoiceReducer,
                            NYT::TOperationOptions()
                                .Wait(false)
                                .MountSandboxInTmpfs(true));
            return op;
        }
    }

    void CollectDates(const TDate& startDate, const TDate& endDate, const NYT::TYPath& outputPath, const NYT::TYPath& tmpDir,
        NYT::IClientPtr& client, const EPlatformType platform, const ui32 maxContextDepth, const ui32 parallelOperations)
    {
        TVector<TString> tablesToMerge;
        {
            NYT::TOperationTracker tracker;
            THashMap<NYT::IOperationPtr, NYT::ITransactionPtr> operations;
            THashMap<NYT::IOperationPtr, TDate> dates;
            for (TDate curDate = startDate; curDate <= endDate; ++curDate) {
                TString dateString = curDate.ToStroka("%Y-%m-%d");
                INFO_LOG << "collecting date " << dateString << Endl;
                const auto transaction = client->StartTransaction();
                // collect ids for next date while waiting for an operation to finish
                THashSet<TString> idSet;
                switch (platform) {
                    case EPlatformType::Quasar:
                        DEBUG_LOG << "collecting quasar" << Endl;
                        idSet = CollectDateIdsQuasar(dateString, transaction);
                        break;
                    case EPlatformType::General:
                        DEBUG_LOG << "collecting general" << Endl;
                        idSet = CollectDateIdsGeneral(dateString, transaction);
                        break;
                }
                if (operations.size() >= parallelOperations) {
                    DEBUG_LOG << "waiting for operation to finish" << Endl;
                    const auto fineshedOp = tracker.WaitOneCompletedOrError();
                    FinishTransaction(fineshedOp, operations, dates);
                }
                const NYT::TYPath dateTable = NYT::JoinYPaths(tmpDir, dateString);
                tablesToMerge.push_back(dateTable);
                const auto operation = CollectDate(dateString, dateTable,
                                                    transaction, idSet, maxContextDepth);
                tracker.AddOperation(operation);
                operations.emplace(operation, transaction);
                dates.emplace(operation, curDate);
            }
            // finish all current transactions
            auto finalOperations = tracker.WaitAllCompletedOrError();
            for (const auto& operation : finalOperations) {
                FinishTransaction(operation, operations, dates);
            }
        }
        INFO_LOG << "adding device states" << Endl;
        {
            NYT::TOperationTracker tracker;
            THashMap<NYT::IOperationPtr, NYT::ITransactionPtr> operations;
            THashMap<NYT::IOperationPtr, TDate> dates;
            for (TDate curDate = startDate; curDate <= endDate; ++curDate) {
                TString dateString = curDate.ToStroka("%Y-%m-%d");
                const auto transaction = client->StartTransaction();
                THashSet<TString> idSet;
                TString dateTable = NYT::JoinYPaths(tmpDir, dateString);
                idSet = CollectContextIds(dateTable, client);
                if (operations.size() >= parallelOperations) {
                    DEBUG_LOG << "waiting for operation to finish" << Endl;
                    const auto fineshedOp = tracker.WaitOneCompletedOrError();
                    FinishTransaction(fineshedOp, operations, dates);
                }
                const auto operation = AddDeviceStateAndVoice(dateString, dateTable, idSet, transaction);
                tracker.AddOperation(operation);
                operations.emplace(operation, transaction);
                dates.emplace(operation, curDate);
            }
            // finish all current transactions
            auto finalOperations = tracker.WaitAllCompletedOrError();
            for (const auto& operation : finalOperations) {
                FinishTransaction(operation, operations, dates);
            }
        }
        INFO_LOG << "merging tables" << Endl;
        {
            const auto mergeTransaction = client->StartTransaction();
            try {
                NYT::TMergeOperationSpec mergeSpec;
                mergeSpec.CombineChunks(true);
                for (const TString& path: tablesToMerge) {
                    if (mergeTransaction->Exists(path)) {
                        mergeSpec.AddInput(path);
                    }
                }
                mergeSpec.Output(outputPath);
                mergeTransaction->Merge(mergeSpec);
                mergeTransaction->Commit();
            } catch (...) {
                mergeTransaction->Abort();
                throw;
            }
        }
        INFO_LOG << "removing temp tables" << Endl;
        {
            const auto removeTransaction = client->StartTransaction();
            for (const TString& path: tablesToMerge) {
                if (removeTransaction->Exists(path)) {
                    removeTransaction->Remove(path);
                }
            }
            removeTransaction->Commit();
        }
    }
}
