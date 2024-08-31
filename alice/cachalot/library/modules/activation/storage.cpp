#include <alice/cachalot/library/modules/activation/storage.h>
#include <alice/cachalot/library/modules/activation/yql_requests.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/storage/mock.h>
#include <alice/cachalot/library/storage/ydb_operation.h>

#include <alice/cuttlefish/library/logging/dlog.h>

#include <util/digest/city.h>
#include <util/string/printf.h>


namespace NCachalot {


bool TActivationStorageKey::operator==(const TActivationStorageKey& other) const {
    return UserId == other.UserId;
}


bool TActivationStorageData::TSpotterFeatures::IsDeaf() const {
    return FuzzyEquals(AvgRMS, 0.0);
}


// Returns true iff `other` is strictly better.
bool TActivationStorageData::Compare(
    const TActivationStorageData& other,
    bool requireValidation,
    bool requireAvgRms
) const {
    requireAvgRms = requireAvgRms && !(SpotterFeatures.IsDeaf() || other.SpotterFeatures.IsDeaf());

    // Here we assume that `0.0 < 0.0` is false.
    // requireValidation means following: truly better record must be validated.
    return other.MakeTuple(requireValidation, requireAvgRms) < MakeTuple(false, requireAvgRms);
}

std::tuple<bool, double, TInstant, TString> TActivationStorageData::MakeTuple(
    bool requireValidation,
    bool requireAvgRms
) const {
    return std::make_tuple(
        requireValidation ? !SpotterFeatures.Validated : false,  // less is better
        requireAvgRms ? -SpotterFeatures.AvgRMS : 0.0,  // less is better
        ActivationAttemptTime,
        DeviceId
    );
}


TInstant TFreshnessManager::GetFreshnessThreshold(TInstant timestamp) const {
    return timestamp - TDuration::MilliSeconds(FreshnessDeltaMilliSeconds);
}


class TActivationYdbOperationBase :
    public TYdbOperationBase<TActivationYdbStorage::TSingleRowResponse, TActivationStorageKey, TActivationStorageData> {
public:
    using TResponse = TActivationYdbStorage::TSingleRowResponse;
    using TBase = TYdbOperationBase<TResponse, TActivationStorageKey, TActivationStorageData>;

    TActivationYdbOperationBase()
        : TBase(&TMetrics::GetInstance().ActivationMetrics.YdbMetrics)
    {
    }

    void SetTtl(int64_t seconds) {
        TtlSeconds = seconds;
    }

    void SetData(TActivationStorageData data) {
        Data = std::move(data);
    }

    void SetFreshnessManager(TFreshnessManager mngr) {
        FreshnessManager = std::move(mngr);
    }

    void SetOperationOptions(TActivationOperationOptions value) {
        OperationOptions = std::move(value);
    }

protected:
    uint64_t CalcUserIdHash(TStringBuf userId) const {
        return CityHash64(userId.data(), userId.size());
    }

    TActivationStorageYqlCodesPtr GetCodes() const {
        return TActivationStorageYqlCodesStorage::GetInstance().GetCodes({
            .Flags = OperationOptions,
            .DatabaseName = DatabaseName,
        });
    }

    NYdb::TParamsBuilder GetBasicQueryParamBuilder(NYdb::TParamsBuilder paramsBuilder) const {
        const TInstant freshnessThreshold = FreshnessManager.GetFreshnessThreshold(Data.ActivationAttemptTime);

        return std::move(paramsBuilder
            .AddParam("$user_id_hash").Uint64(CalcUserIdHash(Key.UserId)).Build()
            .AddParam("$user_id").String(Key.UserId).Build()
            .AddParam("$freshness_threshold").Timestamp(freshnessThreshold).Build());
    }

    void LogResponse() {
        static const TActivationStorageData defaultData = {.DeviceId = "unknown"};
        const TActivationStorageData& data = LastResponse.Data.GetOrElse(defaultData);

        Log<NEvClass::ActivationYdbOperationResponse>(
            LastResponse.Status,
            LastResponse.Key.UserId,

            data.DeviceId,
            data.ActivationAttemptTime.ToString(),
            data.SpotterFeatures.AvgRMS,
            data.SpotterFeatures.Validated,

            LastResponse.ExtraData.RecordWithZeroRmsFound,
            LastResponse.ExtraData.LeaderFound,
            LastResponse.ExtraData.SpotterValidatedBy.GetDeviceId(),

            LastResponse.Stats.SchedulingTime,
            LastResponse.Stats.FetchingTime,
            LastResponse.Stats.ErrorMessage
        );
    }

    bool ProcessValidSpotterSearchResult(
        const NYdb::NTable::TDataQueryResult& queryResult,
        size_t queryPosition
    ) {
        Y_ASSERT(queryPosition < queryResult.GetResultSets().size());
        Y_ASSERT(queryResult.IsSuccess());

        NYdb::TResultSetParser parser = queryResult.GetResultSetParser(queryPosition);
        if (parser.TryNextRow()) {
            const TMaybe<TString> deviceId = parser.ColumnParser("DeviceId").GetOptionalString();
            LastResponse.ExtraData.SpotterValidatedBy.SetCertain(deviceId.GetOrElse("InvalidValueFromYdb"));

            if (LastResponse.Data.Defined()) {
                LastResponse.Data.GetRef().SpotterFeatures.Validated = true;
            }

            return true;
        }
        return false;
    }

protected:
    int64_t TtlSeconds = 4 * 24 * 60 * 60;
    TActivationStorageData Data;
    TFreshnessManager FreshnessManager;
    TActivationOperationOptions OperationOptions;
};


class TActivationAnnouncementYdbOperation : public TActivationYdbOperationBase {
public:
    using TBase = TActivationYdbOperationBase;
    using TResponse = TBase::TResponse;

protected:
    TString GetQueryTemplate() const override {
        return GetCodes()->Annoncement;
    }

    void ProcessQueryResult(
        NThreading::TPromise<NYdb::TStatus>* status,
        const NYdb::NTable::TDataQueryResult& queryResult
    ) override {
        LastResponse.SetKey(Key);
        LastResponse.SetStats(Stats);
        LastResponse.SetStatus(EResponseStatus::NO_CONTENT);

        if (queryResult.IsSuccess()) {
            if (queryResult.GetResultSets().size() != 5) {
                LastResponse.SetStatus(EResponseStatus::INTERNAL_ERROR);
                Metrics->OnExecuteError(StartTime);
            } else {
                for (size_t pos = 0; pos < 4; ++pos) {
                    NYdb::TResultSetParser parser = queryResult.GetResultSetParser(pos);
                    if (parser.TryNextRow()) {
                        if (pos == 2) {
                            LastResponse.ExtraData.RecordWithZeroRmsFound = true;
                        }
                        if (pos == 3) {
                            LastResponse.ExtraData.LeaderFound = true;
                            LastResponse.ExtraData.SpotterValidatedBy.SetLeader();
                        }
                        if (!LastResponse.Data.Defined()) {
                            TActivationStorageData& data = LastResponse.Data.ConstructInPlace();

                            data.DeviceId = parser.ColumnParser(
                                "DeviceId").GetOptionalString().GetOrElse(UNKNOWN_DEVICE_ID);
                            data.ActivationAttemptTime = parser.ColumnParser(
                                "ActivationAttemptTime").GetOptionalTimestamp().GetOrElse(TInstant::Seconds(0));
                            data.SpotterFeatures.AvgRMS = parser.ColumnParser(
                                "AvgRMS").GetOptionalFloat().GetOrElse(-1.0);

                            LastResponse.SetStatus(EResponseStatus::OK);
                            Metrics->OnOk(StartTime);
                        }
                    }
                }

                ProcessValidSpotterSearchResult(queryResult, 4);

                if (LastResponse.Status != EResponseStatus::OK) {
                    Metrics->OnNotFound(StartTime);
                }
            }
        } else {
            LastResponse.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
            LastResponse.SetError(queryResult.GetIssues().ToString());
            Metrics->OnExecuteError(StartTime);
        }

        LogResponse();
        status->SetValue(queryResult);
    }

    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override {
        const TInstant ttlDeadline = TDuration::Seconds(TtlSeconds).ToDeadLine(Data.ActivationAttemptTime);
        return GetBasicQueryParamBuilder(std::move(paramsBuilder))
            .AddParam("$device_id").String(Data.DeviceId).Build()
            .AddParam("$activation_attempt_time").Timestamp(Data.ActivationAttemptTime).Build()
            .AddParam("$avg_rms").Float(Data.SpotterFeatures.AvgRMS).Build()
            .AddParam("$spotter_validated").Bool(Data.SpotterFeatures.Validated).Build()
            .AddParam("$ttl").Timestamp(ttlDeadline).Build()
            .Build();
    }
};


class TActivationTryAcquireLeadershipYdbOperation : public TActivationYdbOperationBase {
public:
    using TBase = TActivationYdbOperationBase;
    using TResponse = TBase::TResponse;

protected:
    TString GetQueryTemplate() const override {
        return GetCodes()->TryAcquireLeadership;
    }

    void ProcessQueryResult(
        NThreading::TPromise<NYdb::TStatus>* status,
        const NYdb::NTable::TDataQueryResult& queryResult
    ) override {
        LastResponse.SetKey(Key).SetStats(Stats);

        if (queryResult.IsSuccess()) {
            if (queryResult.GetResultSets().size() != 3) {
                LastResponse.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
                LastResponse.SetError("Invalid response from ydb");
                Metrics->OnExecuteError(StartTime);
            } else {
                if (ProcessValidSpotterSearchResult(queryResult, 1)) {
                    DLOG("[TActivationTryAcquireLeadershipYdbOperation] Found valid spotter: " <<
                         LastResponse.ExtraData.SpotterValidatedBy.GetDeviceId());
                }

                LastResponse.SetStatus(EResponseStatus::OK);
                Metrics->OnOk(StartTime);
            }
        } else {
            const NYql::TIssues& issues = queryResult.GetIssues();
            if (IsUnvalidatedAssertionError(issues)) {
                LastResponse.ExtraData.SpotterValidatedBy.SetNobody();
            }
            LastResponse.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
            LastResponse.SetError(issues.ToString());
            Metrics->OnExecuteError(StartTime);
        }

        LogResponse();
        status->SetValue(queryResult);
    }

    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override {
        const TInstant ttlDeadline = TDuration::Seconds(TtlSeconds).ToDeadLine(Data.ActivationAttemptTime);
        return GetBasicQueryParamBuilder(std::move(paramsBuilder))
            .AddParam("$device_id").String(Data.DeviceId).Build()
            .AddParam("$activation_attempt_time").Timestamp(Data.ActivationAttemptTime).Build()
            .AddParam("$avg_rms").Float(Data.SpotterFeatures.AvgRMS).Build()
            .AddParam("$ttl").Timestamp(ttlDeadline).Build()
            .Build();
    }

    bool ScanYdbIssues(const NYql::TIssues& issues, const TVector<TString>& markers) const {
        bool found = false;
        for (const NYql::TIssue& issue : issues) {
            NYql::WalkThroughIssues(issue, false, [&markers, &found](const NYql::TIssue& issue, ui16 /* level */) {
                for (const TString& marker : markers) {
                    if (issue.Message.Contains(marker)) {
                        found = true;
                        break;
                    }
                }
            });

            if (found) {
                break;
            }
        }

        return found;
    }

    bool IsUnvalidatedAssertionError(const NYql::TIssues& issues) const {
        static const TVector<TString>& markers = {
            "There are no validated records",
        };
        return ScanYdbIssues(issues, markers);
    }

    bool IsAssertionError(const NYql::TIssues& issues) const override {
        static const TVector<TString>& markers = {
            "constraint violation",
            "There are no validated records",
            "There is better record",
        };
        return ScanYdbIssues(issues, markers);
    }
};


class TActivationGetLeaderYdbOperation : public TActivationYdbOperationBase {
public:
    using TBase = TActivationYdbOperationBase;
    using TResponse = TBase::TResponse;

protected:
    TString GetQueryTemplate() const override {
        return GetCodes()->GetLeader;
    }

    void ProcessQueryResult(
        NThreading::TPromise<NYdb::TStatus>* status,
        const NYdb::NTable::TDataQueryResult& queryResult
    ) override {
        LastResponse.SetKey(Key);
        LastResponse.SetStats(Stats);
        LastResponse.SetStatus(EResponseStatus::NO_CONTENT);

        if (queryResult.IsSuccess()) {
            if (queryResult.GetResultSets().size() != 3) {
                LastResponse.SetStatus(EResponseStatus::INTERNAL_ERROR);
                Metrics->OnExecuteError(StartTime);
            } else {
                for (size_t pos = 0; pos < 2; ++pos) {
                    NYdb::TResultSetParser parser = queryResult.GetResultSetParser(pos);
                    if (parser.TryNextRow()) {
                        TActivationStorageData data;
                        data.DeviceId = parser.ColumnParser("DeviceId").GetOptionalString().GetOrElse(UNKNOWN_DEVICE_ID);
                        data.ActivationAttemptTime = parser.ColumnParser(
                            "ActivationAttemptTime").GetOptionalTimestamp().GetOrElse(TInstant::Seconds(0));
                        data.SpotterFeatures.AvgRMS = parser.ColumnParser("AvgRMS").GetOptionalFloat().GetOrElse(-1.0);

                        LastResponse.SetData(std::move(data));
                        LastResponse.SetStatus(EResponseStatus::OK);
                        Metrics->OnOk(StartTime);

                        break;
                    } else {
                        Metrics->OnNotFound(StartTime);
                    }
                }

                ProcessValidSpotterSearchResult(queryResult, 2);
            }
        } else {
            LastResponse.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
            LastResponse.SetError(queryResult.GetIssues().ToString());
            Metrics->OnExecuteError(StartTime);
        }
        status->SetValue(queryResult);
    }

    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override {
        return GetBasicQueryParamBuilder(std::move(paramsBuilder)).Build();
    }
};


class TActivationClenupLeadersdbOperation : public TActivationYdbOperationBase {
public:
    using TBase = TActivationYdbOperationBase;
    using TResponse = TBase::TResponse;

protected:
    TString GetQueryTemplate() const override {
        return GetCodes()->CleanupLeaders;
    }

    void ProcessQueryResult(
        NThreading::TPromise<NYdb::TStatus>* status,
        const NYdb::NTable::TDataQueryResult& queryResult
    ) override {
        LastResponse.SetKey(Key);
        LastResponse.SetStats(Stats);
        LastResponse.SetStatus(EResponseStatus::OK);

        if (queryResult.IsSuccess()) {
            Metrics->OnOk(StartTime);
        } else {
            LastResponse.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
            LastResponse.SetError(queryResult.GetIssues().ToString());
            Metrics->OnExecuteError(StartTime);
        }
        status->SetValue(queryResult);
    }

    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override {
        return GetBasicQueryParamBuilder(std::move(paramsBuilder))
            .AddParam("$device_id").String(Data.DeviceId).Build()
            .AddParam("$activation_attempt_time").Timestamp(Data.ActivationAttemptTime).Build()
            .Build();
    }
};


TActivationYdbStorage::TActivationYdbStorage(const NCachalot::TYdbSettings& settings)
    : Ydb(settings)
    , OperationTimeout(TDuration::Seconds(settings.ReadTimeoutSeconds()))
{
}

NThreading::TFuture<TActivationYdbStorage::TSingleRowResponse> TActivationYdbStorage::MakeAnnouncement(
    const TActivationStorageKey& key,
    const TActivationStorageData& data,
    const TActivationStorageRequestOptions& options,
    TChroniclerPtr chronicler
) {
    auto operation = MakeIntrusive<TActivationAnnouncementYdbOperation>();
    operation->SetTtl(DefaultTtlSeconds);
    operation->SetKey(key);
    operation->SetData(data);
    operation->MutableOperationSettings().MaxRetries(5)
                                         .OperationTimeout(OperationTimeout)
                                         .ClientTimeout(OperationTimeout + TDuration::MilliSeconds(100));
    operation->SetFreshnessManager(options.FreshnessManager.GetOrElse(this->FreshnessManager));
    operation->SetDatabase(Ydb.GetSettings().Database());
    operation->SetLogger(chronicler);
    operation->SetOperationOptions(options.Flags);
    return operation->Execute(Ydb.GetClient());
}

NThreading::TFuture<TActivationYdbStorage::TSingleRowResponse> TActivationYdbStorage::TryAcquireLeadership(
    const TActivationStorageKey& key,
    const TActivationStorageData& data,
    const TActivationStorageRequestOptions& options,
    TChroniclerPtr chronicler
) {
    auto operation = MakeIntrusive<TActivationTryAcquireLeadershipYdbOperation>();
    operation->SetTtl(DefaultTtlSeconds);
    operation->SetKey(key);
    operation->SetData(data);
    operation->MutableOperationSettings().MaxRetries(3)
                                         .OperationTimeout(OperationTimeout)
                                         .ClientTimeout(OperationTimeout + TDuration::MilliSeconds(100));
    operation->SetFreshnessManager(options.FreshnessManager.GetOrElse(this->FreshnessManager));
    operation->SetDatabase(Ydb.GetSettings().Database());
    operation->SetLogger(chronicler);
    operation->SetOperationOptions(options.Flags);
    return operation->Execute(Ydb.GetClient());
}

NThreading::TFuture<TActivationYdbStorage::TSingleRowResponse> TActivationYdbStorage::GetLeader(
    const TActivationStorageKey& key,
    const TActivationStorageData& data,
    const TActivationStorageRequestOptions& options,
    TChroniclerPtr chronicler
) {
    auto operation = MakeIntrusive<TActivationGetLeaderYdbOperation>();
    operation->SetKey(key);
    operation->SetData(data);
    operation->MutableOperationSettings().MaxRetries(3)
                                         .OperationTimeout(OperationTimeout)
                                         .ClientTimeout(OperationTimeout + TDuration::MilliSeconds(100));
    operation->SetFreshnessManager(options.FreshnessManager.GetOrElse(this->FreshnessManager));
    operation->SetDatabase(Ydb.GetSettings().Database());
    operation->SetLogger(chronicler);
    operation->SetOperationOptions(options.Flags);
    return operation->Execute(Ydb.GetClient());
}

NThreading::TFuture<TActivationYdbStorage::TSingleRowResponse> TActivationYdbStorage::ClenupLeaders(
    const TActivationStorageKey& key,
    const TActivationStorageData& data,
    const TActivationStorageRequestOptions& options,
    TChroniclerPtr chronicler
) {
    auto operation = MakeIntrusive<TActivationClenupLeadersdbOperation>();
    operation->SetKey(key);
    operation->SetData(data);
    operation->MutableOperationSettings().MaxRetries(10)
                                         .OperationTimeout(OperationTimeout)
                                         .ClientTimeout(OperationTimeout + TDuration::MilliSeconds(100));
    operation->SetFreshnessManager(options.FreshnessManager.GetOrElse(this->FreshnessManager));
    operation->SetDatabase(Ydb.GetSettings().Database());
    operation->SetLogger(chronicler);
    operation->SetOperationOptions(options.Flags);
    return operation->Execute(Ydb.GetClient());
}


namespace {
    TActivationStorageMock::TSingleRowResponse AddExtraToResponse(
        TActivationStorageMock::TSingleRowResponse&& rsp, IActivationStorage::TExtraData data
    ) {
        rsp.ExtraData = data;
        return rsp;
    }
}


TActivationStorageMock::TActivationStorageMock()
    : AnnouncementStorage(MakeIntrusive<TKvStorageMock<TActivationStorageKey, TActivationStorageData, TExtraData>>())
    , LeadershipStorage(MakeIntrusive<TKvStorageMock<TActivationStorageKey, TActivationStorageData, TExtraData>>())
{
}

NThreading::TFuture<TActivationStorageMock::TSingleRowResponse> TActivationStorageMock::MakeAnnouncement(
    const TActivationStorageKey& key,
    const TActivationStorageData& data,
    const TActivationStorageRequestOptions& options,
    TChroniclerPtr /* chronicler */
) {
    IActivationStorage::TExtraData extraData;

    TSingleRowResponse betterAnnouncement = FindBetterAnnouncement(key, data, options);
    TSingleRowResponse bestAnnouncementWithValidRms =
        FindBestAnnouncement(key, data, options, /* requireValidation = */ false, EElectionMode::NonDeaf);
    TSingleRowResponse bestAnnouncementWithZeroRms =
        FindBestAnnouncement(key, data, options, /* requireValidation = */ false, EElectionMode::Deaf);

    using TUnderlyingKvStorage = TKvStorageMock<TActivationStorageKey, TActivationStorageData, TExtraData>;
    if (auto* leadershipStorage = dynamic_cast<TUnderlyingKvStorage*>(LeadershipStorage.Get())) {
        extraData.LeaderFound = leadershipStorage->Contains(key);
    }

    if (auto* announcementStorage = dynamic_cast<TUnderlyingKvStorage*>(AnnouncementStorage.Get())) {
        announcementStorage->Upsert(key, data, DefaultTtlSeconds, [&](const TActivationStorageData& row) {
            if (row.DeviceId == data.DeviceId) {
                DLOG("Updating record (" << key.UserId << ", " << row.DeviceId << ")");
                return true;
            } else {
                return false;
            }
        });
    }

    ClenupLeaders(key, data, options, nullptr);

    extraData.RecordWithZeroRmsFound = (bestAnnouncementWithZeroRms.Status == EResponseStatus::OK);

    if (betterAnnouncement.Status == EResponseStatus::OK) {
        return MakeAsyncResponse(AddExtraToResponse(std::move(betterAnnouncement), extraData));
    } else if (bestAnnouncementWithValidRms.Status == EResponseStatus::OK) {
        return MakeAsyncResponse(AddExtraToResponse(std::move(bestAnnouncementWithValidRms), extraData));
    } else {
        return MakeAsyncResponse(AddExtraToResponse(std::move(bestAnnouncementWithZeroRms), extraData));
    }
}

NThreading::TFuture<TActivationStorageMock::TSingleRowResponse> TActivationStorageMock::TryAcquireLeadership(
    const TActivationStorageKey& key,
    const TActivationStorageData& data,
    const TActivationStorageRequestOptions& options,
    TChroniclerPtr /* chronicler */
) {
    TSingleRowResponse response;
    response.SetKey(key);

    {
        const auto announcements = AnnouncementStorage->Get(key).GetValue();
        bool validSpotterFound = false;

        const TFreshnessManager& freshnessManager = options.FreshnessManager.GetOrElse(this->FreshnessManager);
        const TInstant freshnessThreshold = freshnessManager.GetFreshnessThreshold(data.ActivationAttemptTime);

        for (const TActivationStorageData& row : announcements.Rows) {
            if (row.ActivationAttemptTime >= freshnessThreshold && row.SpotterFeatures.Validated) {
                validSpotterFound = true;
                response.ExtraData.SpotterValidatedBy.SetCertain(row.DeviceId);
                break;
            }
        }

        if (!validSpotterFound) {
            response.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
            response.SetError("There are no validated records");
            return MakeAsyncResponse(std::move(response));
        }
    }

    const TSingleRowResponse betterAnnouncement = FindBetterAnnouncement(key, data, options);
    if (betterAnnouncement.Data.Defined()) {
        response.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
        response.SetError("There is better record");
        return MakeAsyncResponse(std::move(response));
    }

    using TUnderlyingKvStorage = TKvStorageMock<TActivationStorageKey, TActivationStorageData, TExtraData>;
    if (auto* leadershipStorage = dynamic_cast<TUnderlyingKvStorage*>(LeadershipStorage.Get())) {
        if (leadershipStorage->InsertUnique(key, data)) {
            response.SetStatus(EResponseStatus::OK);
        } else {
            response.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
            response.SetError("Leader already elected");
        }
    }
    return MakeAsyncResponse(std::move(response));
}

NThreading::TFuture<TActivationStorageMock::TSingleRowResponse> TActivationStorageMock::GetLeader(
    const TActivationStorageKey& key,
    const TActivationStorageData& data,
    const TActivationStorageRequestOptions& options,
    TChroniclerPtr /* chronicler */
) {
    TSingleRowResponse response;
    response.SetStatus(EResponseStatus::NO_CONTENT);

    const TFreshnessManager& freshnessManager = options.FreshnessManager.GetOrElse(this->FreshnessManager);
    const TInstant freshnessThreshold = freshnessManager.GetFreshnessThreshold(data.ActivationAttemptTime);
    const TSingleRowResponse leaderRow = LeadershipStorage->GetSingleRow(key).GetValue();
    if (leaderRow.Status == EResponseStatus::OK) {
        const TActivationStorageData& leaderData = leaderRow.Data.GetRef();
        if (leaderData.ActivationAttemptTime >= freshnessThreshold) {
            response.SetStatus(EResponseStatus::OK);
            response.SetData(leaderData);
        } else {
            DLOG("Leader record (" << key.UserId << ", " << leaderData.DeviceId << ") is too old");
        }
    }

    if (response.Status != EResponseStatus::OK) {
        DLOG("Searching for leader of " << key.UserId << " in announcements");
        response = FindBestAnnouncement(key, data, options,
            /* requireValidation = */ false,
            /* electionMode = */ options.Flags.IgnoreRms ? EElectionMode::IgnoreRms : EElectionMode::Normal);
    }
    return MakeAsyncResponse(std::move(response));
}

NThreading::TFuture<TActivationStorageMock::TSingleRowResponse> TActivationStorageMock::ClenupLeaders(
    const TActivationStorageKey& key,
    const TActivationStorageData& data,
    const TActivationStorageRequestOptions& options,
    TChroniclerPtr /* chronicler */
) {
    TSingleRowResponse response;
    response.SetStatus(EResponseStatus::OK);

    using TUnderlyingKvStorage = TKvStorageMock<TActivationStorageKey, TActivationStorageData, TExtraData>;
    if (auto* leadershipStorage = dynamic_cast<TUnderlyingKvStorage*>(LeadershipStorage.Get())) {
        const TFreshnessManager& freshnessManager = options.FreshnessManager.GetOrElse(this->FreshnessManager);
        const TInstant freshnessThreshold = freshnessManager.GetFreshnessThreshold(data.ActivationAttemptTime);
        leadershipStorage->DelIf(key, [&] (const TActivationStorageData& row) {
            if (row.ActivationAttemptTime < freshnessThreshold) {
                DLOG("Removing record (" << key.UserId << ", " << row.DeviceId << ")");
                return true;
            } else {
                DLOG("Keeping record (" << key.UserId << ", " << row.DeviceId << ")");
                return false;
            }
        });
    }

    return MakeAsyncResponse(std::move(response));
}


TActivationStorageMock::TSingleRowResponse TActivationStorageMock::FindBestAnnouncement(
    const TActivationStorageKey& key,
    const TActivationStorageData& data,
    const TActivationStorageRequestOptions& options,
    bool requireValidation,
    EElectionMode electionMode
) const {
    const TFreshnessManager& freshnessManager = options.FreshnessManager.GetOrElse(this->FreshnessManager);
    const TInstant freshnessThreshold = freshnessManager.GetFreshnessThreshold(data.ActivationAttemptTime);
    const auto fasterAnnouncements = AnnouncementStorage->Get(key).GetValue();

    TSingleRowResponse bestAnnouncement;
    const TMaybe<TActivationStorageData>& bestData = bestAnnouncement.Data;
    bestAnnouncement.SetStatus(EResponseStatus::NO_CONTENT);
    bestAnnouncement.SetKey(key);

    const bool requireAvgRmsCmp = (electionMode != EElectionMode::IgnoreRms);

    for (const TActivationStorageData& row : fasterAnnouncements.Rows) {
        if (electionMode == EElectionMode::Deaf && !row.SpotterFeatures.IsDeaf()) {
            continue;
        }
        if (electionMode == EElectionMode::NonDeaf && row.SpotterFeatures.IsDeaf()) {
            continue;
        }
        if (row.ActivationAttemptTime >= freshnessThreshold) {
            const bool empty = !bestData.Defined();
            const bool needAssign = empty && (!requireValidation || row.SpotterFeatures.Validated);
            const bool needUpdate = !empty && bestData->Compare(row, requireValidation, requireAvgRmsCmp);
            if (needAssign || needUpdate) {
                bestAnnouncement.SetStatus(EResponseStatus::OK);
                bestAnnouncement.SetData(row);
            }
        }
    }

    return bestAnnouncement;
}


TActivationStorageMock::TSingleRowResponse TActivationStorageMock::FindBetterAnnouncement(
    const TActivationStorageKey& key,
    const TActivationStorageData& data,
    const TActivationStorageRequestOptions& options
) const {
    const bool requireValidation = false;
    DLOG("[FindBetterAnnouncement] requireValidation = " << requireValidation);

    TSingleRowResponse bestAnnouncement = FindBestAnnouncement(key, data, options, requireValidation,
        /* electionMode = */ options.Flags.IgnoreRms ? EElectionMode::IgnoreRms : EElectionMode::Normal);
    if (
        bestAnnouncement.Status == EResponseStatus::OK &&
        data.Compare(bestAnnouncement.Data.GetRef(), requireValidation)
    ) {
        DLOG("Record for " << bestAnnouncement.Data->DeviceId << " is better than record for " << data.DeviceId);
        return bestAnnouncement;
    } else {
        TSingleRowResponse emptyResp;
        emptyResp.SetStatus(EResponseStatus::NO_CONTENT);
        emptyResp.SetKey(key);
        return emptyResp;
    }
}


TIntrusivePtr<IActivationStorage> MakeActivationStorage(const NCachalot::TYdbSettings& settings) {
    if (settings.IsFake()) {
        return MakeIntrusive<TActivationStorageMock>();
    }
    return MakeIntrusive<TActivationYdbStorage>(settings);
}


}   // namespace NCachalot
