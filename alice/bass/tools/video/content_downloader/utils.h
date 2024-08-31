#pragma once

#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/video_common/defs.h>
#include <alice/bass/libs/video_common/rps_config.h>
#include <alice/bass/libs/video_common/utils.h>

#include <alice/library/yt/util.h>

#include <mapreduce/yt/common/helpers.h>
#include <mapreduce/yt/interface/client_method_options.h>
#include <mapreduce/yt/interface/common.h>
#include <mapreduce/yt/interface/cypress.h>
#include <mapreduce/yt/interface/fwd.h>
#include <mapreduce/yt/library/table_schema/protobuf.h>
#include <library/cpp/yson/node/node.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <utility>

// // This value *MUST* be consistent with the URL below.
inline constexpr TStringBuf KINOPOISK_TVM_ID = "2009799";

// Creates an empty table with strict schema. If something already
// exists at the |path|, it will be overwritten.
template <typename TSchema>
NYT::TNodeId CreateTable(NYT::ICypressClient& client, const NYT::TYPath& path) {
    NYT::TCreateOptions options;
    const auto schema = NYT::CreateTableSchema<TSchema>().Strict(true);
    options.Attributes(NYT::TNode()("optimize_for", "scan")("schema", NYT::NodeFromTableSchema(schema)));
    options.Force(true);
    return client.Create(path, NYT::ENodeType::NT_TABLE, options);
}

template <typename TItem>
struct TDownloadItem {
    explicit TDownloadItem(const TItem& item)
        : Item(item) {
    }

    TItem Item;
    ui32 Retries = 0;
};

template <typename TItem>
using TDownloadQueue = TQueue<TDownloadItem<TItem>>;

struct TStats {
    void Reset() {
        Downloaded = 0;
        Skipped = 0;
        Failed = 0;
        Total = 0;
        Retries = 0;
    }

    void Log() const {
        LOG(INFO) << "Downloaded / Skipped / Failed / Total / Retries: " << Downloaded << " / " << Skipped << " / "
                  << Failed << " / " << Total << " / " << Retries << Endl;
    }

    ui64 Downloaded = 0;
    ui64 Skipped = 0;
    ui64 Failed = 0;
    ui64 Total = 0;
    ui64 Retries = 0;
};

template <typename TItem>
class TBatchedQueue {
public:
    TBatchedQueue(size_t maxRPS, const TStats& stats)
        : MaxRPS(maxRPS)
        , Stats(stats) {
    }

    void Clear() {
        Queue.clear();
    }

    void Push(const TItem& item) {
        Push(TDownloadItem<TItem>(item));
    }

    void Push(const TDownloadItem<TItem>& item) {
        Queue.push(item);
    }

    bool Empty() const {
        return Queue.empty();
    }

    template <typename TFn>
    bool Download(bool force, TFn&& fn) {
        if (Queue.empty())
            return false;
        if (!force && Queue.size() < MaxRPS)
            return false;

        NVideoCommon::WaitForNextDownload(LastDownload);

        TVector<TDownloadItem<TItem>> candidates;
        for (size_t i = 0; i < MaxRPS && !Queue.empty(); ++i) {
            candidates.push_back(Queue.front());
            Queue.pop();
        }

        fn(candidates);

        Stats.Log();
        LOG(INFO) << "Queue size: " << Queue.size() << Endl;

        return true;
    }

private:
    size_t MaxRPS;
    TMaybe<NVideoCommon::TTimePoint> LastDownload;
    const TStats& Stats;

    TDownloadQueue<TItem> Queue;
};

template <typename TItem>
class TBatchedMultiQueue {
public:
    TBatchedMultiQueue(NVideoCommon::IContentInfoProvidersCache& providers, const NVideoCommon::TRPSConfig& rpsConfig,
                       const TStats& stats)
        : Stats(stats) {
        for (auto iter = providers.begin(), e = providers.end(); iter != e; ++iter) {
            TString providerName = iter.GetProviderName();
            size_t maxRPS = rpsConfig.GetRPSLimit(providerName);
            Queues.emplace(std::piecewise_construct, std::forward_as_tuple(providerName),
                           std::forward_as_tuple(maxRPS, Stats));
        }
    }

    void Push(TStringBuf providerName, const TDownloadItem<TItem>& item) {
        if (auto* queue = Queues.FindPtr(providerName))
            queue->Push(item);
    }

    void Push(TStringBuf providerName, const TItem& item) {
        if (auto* queue = Queues.FindPtr(providerName))
            queue->Push(item);
    }

    template <typename TFn>
    bool Download(TStringBuf providerName, TFn&& fn) {
        auto* queue = Queues.FindPtr(providerName);
        return queue ? queue->Download(false/* force */, std::forward<TFn>(fn)) : false;
    }

    template <typename TFn>
    bool DownloadAll(TFn&& fn) {
        bool hasDownloadedItems = false;
        for (auto& [providerName, queue] : Queues) {
            if (queue.Download(true/* force */, fn))
                hasDownloadedItems = true;
        }
        return hasDownloadedItems;
    }

private:
    THashMap<TString, TBatchedQueue<TItem>> Queues;
    const TStats& Stats;
};
