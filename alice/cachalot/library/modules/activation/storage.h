#pragma once

#include <alice/cachalot/library/modules/activation/ana_log.h>
#include <alice/cachalot/library/modules/activation/common.h>

#include <alice/cachalot/library/storage/storage.h>
#include <alice/cachalot/library/storage/ydb.h>

#include <alice/cachalot/library/config/application.cfgproto.pb.h>

#include <util/generic/ymath.h>
#include <util/str_stl.h>

#include <tuple>

namespace NCachalot {


struct TActivationStorageKey {
    TString UserId;

    // For THashMap in TKvStorageMock.
    bool operator==(const TActivationStorageKey& other) const;
};


struct TActivationStorageData {
public:
    struct TSpotterFeatures {
        double AvgRMS = 0.0;
        bool Validated = false;

        bool IsDeaf() const;
    };

    TString DeviceId;
    TInstant ActivationAttemptTime;
    TSpotterFeatures SpotterFeatures;

public:
    // Returns true iff `other` is strictly better.
    bool Compare(const TActivationStorageData& other, bool requireValidation, bool requireAvgRms = true) const;

private:
    std::tuple<bool, double, TInstant, TString> MakeTuple(bool requireValidation, bool requireAvgRms) const;
};


struct TFreshnessManager {
    TInstant GetFreshnessThreshold(TInstant timestamp) const;

    static constexpr int64_t DefaultFreshnessDeltaMilliSeconds = 2500;
    int64_t FreshnessDeltaMilliSeconds = DefaultFreshnessDeltaMilliSeconds;
};


struct TActivationStorageRequestOptions {
    TActivationOperationOptions Flags;
    TMaybe<TFreshnessManager> FreshnessManager;
};


class IActivationStorage : public TThrRefBase {
public:
    struct TExtraData {
        bool RecordWithZeroRmsFound = false;
        bool LeaderFound = false;
        TSpotterValidationDevice SpotterValidatedBy;
    };

    using TSingleRowResponse = TSingleRowStorageResponse<TActivationStorageKey, TActivationStorageData, TExtraData>;

public:
    virtual NThreading::TFuture<TSingleRowResponse> MakeAnnouncement(const TActivationStorageKey& key,
        const TActivationStorageData& data, const TActivationStorageRequestOptions& options,
        TChroniclerPtr chronicler) = 0;

    virtual NThreading::TFuture<TSingleRowResponse> TryAcquireLeadership(const TActivationStorageKey& key,
        const TActivationStorageData& data, const TActivationStorageRequestOptions& options,
        TChroniclerPtr chronicler) = 0;

    virtual NThreading::TFuture<TSingleRowResponse> GetLeader(const TActivationStorageKey& key,
        const TActivationStorageData& data, const TActivationStorageRequestOptions& options,
        TChroniclerPtr chronicler) = 0;

    virtual NThreading::TFuture<TSingleRowResponse> ClenupLeaders(const TActivationStorageKey& key,
        const TActivationStorageData& data, const TActivationStorageRequestOptions& options,
        TChroniclerPtr chronicler) = 0;

    virtual TString GetStorageTag() const = 0;

    void SetDefaultTtlSeconds(int64_t seconds) {
        DefaultTtlSeconds = seconds;
    }

    void SetFreshnessThresholdMilliSeconds(int64_t milliseconds) {
        FreshnessManager.FreshnessDeltaMilliSeconds = milliseconds;
    }

    int64_t GetFreshnessThresholdMilliSeconds() {
        return FreshnessManager.FreshnessDeltaMilliSeconds;
    }

protected:
    int64_t DefaultTtlSeconds = 12 * 60 * 60;
    TFreshnessManager FreshnessManager;
};


class TActivationYdbStorage : public IActivationStorage {
public:
    using typename IActivationStorage::TSingleRowResponse;

public:
    TActivationYdbStorage(const NCachalot::TYdbSettings& settings);

    NThreading::TFuture<TSingleRowResponse> MakeAnnouncement(const TActivationStorageKey& key,
        const TActivationStorageData& data, const TActivationStorageRequestOptions& options,
        TChroniclerPtr chronicler) override;

    NThreading::TFuture<TSingleRowResponse> TryAcquireLeadership(const TActivationStorageKey& key,
        const TActivationStorageData& data, const TActivationStorageRequestOptions& options,
        TChroniclerPtr chronicler) override;

    NThreading::TFuture<TSingleRowResponse> GetLeader(const TActivationStorageKey& key,
        const TActivationStorageData& data, const TActivationStorageRequestOptions& options,
        TChroniclerPtr chronicler) override;

    NThreading::TFuture<TSingleRowResponse> ClenupLeaders(const TActivationStorageKey& key,
        const TActivationStorageData& data, const TActivationStorageRequestOptions& options,
        TChroniclerPtr chronicler) override;

    TString GetStorageTag() const override {
        return "activation-ydb";
    }

private:
    TYdbContext Ydb;
    TDuration OperationTimeout = TDuration::MilliSeconds(400);
};


class TActivationStorageMock : public IActivationStorage {
public:
    using typename IActivationStorage::TSingleRowResponse;
    using typename IActivationStorage::TExtraData;

public:
    TActivationStorageMock();

    NThreading::TFuture<TSingleRowResponse> MakeAnnouncement(const TActivationStorageKey& key,
        const TActivationStorageData& data, const TActivationStorageRequestOptions& options,
        TChroniclerPtr chronicler) override;

    NThreading::TFuture<TSingleRowResponse> TryAcquireLeadership(const TActivationStorageKey& key,
        const TActivationStorageData& data, const TActivationStorageRequestOptions& options,
        TChroniclerPtr chronicler) override;

    NThreading::TFuture<TSingleRowResponse> GetLeader(const TActivationStorageKey& key,
        const TActivationStorageData& data, const TActivationStorageRequestOptions& options,
        TChroniclerPtr chronicler) override;

    NThreading::TFuture<TSingleRowResponse> ClenupLeaders(const TActivationStorageKey& key,
        const TActivationStorageData& data, const TActivationStorageRequestOptions& options,
        TChroniclerPtr chronicler) override;

    TString GetStorageTag() const override {
        return "activation-mock";
    }

private:
    enum EElectionMode {
        Normal,
        IgnoreRms,
        Deaf,
        NonDeaf,
    };

    TActivationStorageMock::TSingleRowResponse FindBetterAnnouncement(const TActivationStorageKey& key,
        const TActivationStorageData& data, const TActivationStorageRequestOptions& options) const;
    TActivationStorageMock::TSingleRowResponse FindBestAnnouncement(const TActivationStorageKey& key,
        const TActivationStorageData& data, const TActivationStorageRequestOptions& options,
        bool requireValidation, EElectionMode electionMode) const;

private:
    TIntrusivePtr<IKvStorage<TActivationStorageKey, TActivationStorageData, TExtraData>> AnnouncementStorage;
    TIntrusivePtr<IKvStorage<TActivationStorageKey, TActivationStorageData, TExtraData>> LeadershipStorage;
};


TIntrusivePtr<IActivationStorage> MakeActivationStorage(const NCachalot::TYdbSettings& settings);

}   // namespace NCachalot


// For THashMap in TKvStorageMock.
template <>
struct THash<NCachalot::TActivationStorageKey> {
    size_t operator()(const NCachalot::TActivationStorageKey& object) const noexcept {
        return THash<TStringBuf>()(object.UserId);
    }
};
