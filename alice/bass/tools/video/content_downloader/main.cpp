#include "utils.h"

#include <alice/bass/libs/video_content/common.h>
#include <alice/bass/libs/video_content/protos/rows.pb.h>

#include <alice/bass/libs/video_common/amediateka_utils.h>
#include <alice/bass/libs/video_common/content_db.h>
#include <alice/bass/libs/video_common/defs.h>
#include <alice/bass/libs/video_common/ivi_genres.h>
#include <alice/bass/libs/video_common/ivi_utils.h>
#include <alice/bass/libs/video_common/keys.h>
#include <alice/bass/libs/video_common/providers.h>
#include <alice/bass/libs/video_common/rps_config.h>
#include <alice/bass/libs/video_common/tvm_cache_delegate.h>
#include <alice/bass/libs/video_common/universal_api_utils.h>

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/tvm2/ticket_cache/ticket_cache.h>

#include <alice/library/yt/util.h>
#include <alice/library/json/json.h>

#include <robot/jupiter/protos/compatibility/urldat.pb.h>

#include <mapreduce/yt/common/helpers.h>
#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/cypress.h>
#include <mapreduce/yt/interface/operation.h>
#include <mapreduce/yt/util/ypath_join.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/string_utils/scan/scan.h>

#include <util/generic/algorithm.h>
#include <util/generic/maybe.h>
#include <util/generic/queue.h>
#include <util/generic/string.h>
#include <util/string/join.h>
#include <util/system/types.h>
#include <util/system/yassert.h>

#include <cstddef>
#include <cstdlib>

using namespace NAliceYT;
using namespace NYT;
using namespace NVideoCommon;

namespace {

using TKeysScheme = NVideoContent::TVideoKeysTableTraits::TYTScheme;
using TItemsScheme = NVideoContent::TVideoItemsLatestTableTraits::TYTScheme;
using TSerialsScheme = NVideoContent::TVideoSerialsTableTraits::TYTScheme;
using TEpisodesScheme = NVideoContent::TVideoEpisodesTableTraits::TYTScheme;
using TSeasonsScheme = NVideoContent::TVideoSeasonsTableTraits::TYTScheme;
using TProviderUniqueItemsScheme = NVideoContent::TProviderUniqueItemsTableTraitsV2::TYTScheme;

using TProvidersCounts = THashMap<TString, size_t>;

const TString ENV_TVM2_BASS_ID = "TVM2_BASS_ID";
const TString ENV_TVM2_BASS_SECRET = "TVM2_BASS_SECRET";

const TString URLDAT_ROOT = "//home/jupiter/urldat";
const TString DATA_TABLE = "data";

const size_t DEFAULT_MAX_RETRIES = 3;

const auto TV_SHOW = ToString(EItemType::TvShow);
const auto TV_SHOW_EPISODE = ToString(EItemType::TvShowEpisode);

bool IsProviderDataValid(const TProvidersCounts& expectedCounts, const TProvidersCounts& actualCounts,
                         TStringBuf paramName) {
    for (const auto& [providerName, expectedCount] : expectedCounts) {
        const size_t* actualCountPtr = actualCounts.FindPtr(providerName);
        size_t actualCount = actualCountPtr ? *actualCountPtr : 0;
        if (expectedCount > actualCount) {
            LOG(ERR) << paramName << " for provider " << providerName << " failed sanity check: expected at least "
                     << expectedCount << " items, but only " << actualCount << " have been actually downloaded!"
                     << Endl;
            return false;
        }
    }
    return true;
}

void RegisterProviderItem(TProvidersCounts& counts, TStringBuf providerName) {
    auto [iter, wasAdded] = counts.insert({TString{providerName}, 0});
    ++iter->second;
}

// Maps urldat records to video keys, that may be used to retrieve
// video content info.
class TKeysMapper : public IMapper<TTableReader<NJupiter::TUrldat>, TTableWriter<TKeysScheme>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();

            const auto host = row.GetHost();
            const auto path = row.GetCanonizedPath();

            if (host == IVI_HOST)
                OnProvider(writer, PROVIDER_IVI, host, path);
            if (host == AMEDIATEKA_HOST)
                OnProvider(writer, PROVIDER_AMEDIATEKA, host, path);
        }
    }

private:
    static void OnProvider(TTableWriter<TKeysScheme>* writer, TStringBuf provider, const TString& host,
                           const TString& path) {
        const auto url = host + path;
        TVideoItem item;

        if (provider == PROVIDER_IVI) {
            if (!ParseIviItemFromUrl(url, item))
                return;
            if (item->Type() == TV_SHOW_EPISODE) {
                TVideoItem tvShowItem;
                tvShowItem->Type() = TV_SHOW;
                if (item->HasHumanReadableId())
                    tvShowItem->HumanReadableId() = item->HumanReadableId();
                item.Value().Swap(tvShowItem.Value());
            }
        } else if (provider == PROVIDER_AMEDIATEKA) {
            if (!ParseAmediatekaItemFromUrl(url, item))
                return;
        } else {
            return;
        }

        if (!NVideoCommon::IsTypeSupportedByVideoItemsTable(item->Type()))
            return;
        if (!item->HasHumanReadableId() && !item->HasProviderItemId())
            return;

        TKeysScheme row;

        row.SetProviderName(TString{provider});
        if (item->HasProviderItemId())
            row.SetProviderItemId(TString{item->ProviderItemId().Get()});
        if (item->HasHumanReadableId())
            row.SetHumanReadableId(TString{item->HumanReadableId().Get()});
        Y_ASSERT(item->HasType());
        row.SetType(TString{item->Type().Get()});

        writer->AddRow(row);
    }
};
REGISTER_MAPPER(TKeysMapper);

using TKeysReducer = TUniqueReducer<TKeysScheme>;
REGISTER_REDUCER(TKeysReducer);

class TBaseDownloader {
public:
    explicit TBaseDownloader(TContentInfoProvidersCache& providers)
        : Providers(providers) {
    }

    virtual ~TBaseDownloader() = default;

protected:
    TContentInfoProvidersCache& Providers;

    TStats Stats;
    TMaybe<TTimePoint> LastDownload;
};

void AddRow(TTableWriter<TItemsScheme>* writer, const TVideoItem& item, bool isVoid) {
    TItemsScheme row;
    NVideoCommon::Ser(item, isVoid, row);
    writer->AddRow(row);
}

bool AddRow(TTableWriter<TSerialsScheme>* writer, const TSerialKey& key, const TSerialDescriptor& serial) {
    TSerialsScheme row;
    if (!serial.Ser(key, row)) {
        LOG(ERR) << "Failed to serialize serial protobuf for key: " << key << Endl;
        return false;
    }

    writer->AddRow(row);
    return true;
}

bool AddRow(TTableWriter<TProviderUniqueItemsScheme>* writer, const TVideoItem& item) {
    TProviderUniqueItemsScheme outputRow;
    if (!Ser(item, outputRow)) {
        LOG(ERR) << "Cannot serialize provider-unique item: " << item << Endl;
        return false;
    }

    writer->AddRow(outputRow);
    return true;
}

class TItemsDownloader final : public TBaseDownloader {
public:
    using TReader = TTableReader<TKeysScheme>;
    using TWriter = TTableWriter<TItemsScheme>;

    TItemsDownloader(TContentInfoProvidersCache& providers, const TRPSConfig& rpsConfig,
                     TProvidersCounts& downloadCounts)
        : TBaseDownloader(providers)
        , Queue(providers, rpsConfig, Stats)
        , DownloadCounts(downloadCounts)
    {
    }

    void Do(TReader* reader, TWriter* writer) {
        for (; reader->IsValid(); reader->Next()) {
            TKeysScheme row = reader->GetRow();
            TStringBuf providerName = row.GetProviderName();

            TVideoItem item;
            item->ProviderName() = providerName;
            if (row.HasProviderItemId())
                item->ProviderItemId() = row.GetProviderItemId();
            if (row.HasHumanReadableId())
                item->HumanReadableId() = row.GetHumanReadableId();
            if (row.HasType())
                item->Type() = row.GetType();

            Queue.Push(providerName, item);
            ++Stats.Total;

            DownloadItems(providerName, writer);
        }

        while (DownloadAll(writer))
            ;
    }

protected:
    bool DownloadItems(TStringBuf providerName, TWriter* writer) {
        return Queue.Download(providerName, [&](const TVector<TDownloadItem<TVideoItem>>& candidates) {
            DownloadItemsImpl(writer, candidates);
        });
    }

    bool DownloadAll(TWriter* writer) {
        return Queue.DownloadAll(
            [&](const TVector<TDownloadItem<TVideoItem>>& candidates) { DownloadItemsImpl(writer, candidates); });
    }

    void DownloadItemsImpl(TWriter* writer, const TVector<TDownloadItem<TVideoItem>>& candidates) {
        TVector<TVideoItem> items(Reserve(candidates.size()));
        for (const auto& candidate : candidates)
            items.push_back(candidate.Item);

        Y_ASSERT(items.size() == candidates.size());

        NVideoCommon::DownloadItems(items, Providers,
                                    [&](size_t /* index */, const TVideoItem& item) {
                                        if (NVideoCommon::IsTypeSupportedByVideoItemsTable(item->Type())) {
                                            AddRow(writer, item, false /* isVoid */);
                                            ++Stats.Downloaded;
                                            RegisterProviderItem(DownloadCounts, item->ProviderName());
                                        } else {
                                            LOG(INFO) << "Skipping video item of type: " << item->Type() << Endl;
                                            ++Stats.Skipped;
                                        }
                                    } /* onSuccess */,
                                    [&](size_t index, const TError& error) {
                                        Y_ASSERT(index < candidates.size());

                                        LOG(ERR) << "Error: " << error.Msg << ", code: " << error.Code << Endl;
                                        if (error.Code != HttpCodes::HTTP_TOO_MANY_REQUESTS) {
                                            const auto& item = items[index];
                                            if (NVideoCommon::IsTypeSupportedByVideoItemsTable(item->Type())) {
                                                const auto& providerName = item->ProviderName();
                                                // Well, OTT handles
                                                // are quite unstable,
                                                // better to skip them
                                                // completely.
                                                if (providerName != PROVIDER_KINOPOISK)
                                                    AddRow(writer, item, true /* isVoid */);
                                            }
                                            ++Stats.Failed;
                                            return;
                                        }

                                        auto& candidate = candidates[index];
                                        if (candidate.Retries < DEFAULT_MAX_RETRIES) {
                                            ++Stats.Retries;
                                            Queue.Push(candidate.Item->ProviderName(), candidate);
                                        } else {
                                            ++Stats.Failed;
                                        }
                                    } /* onError */);
    }

private:
    TBatchedMultiQueue<TVideoItem> Queue;
    TProvidersCounts& DownloadCounts;
};

using TItemsReducer = TUniqueReducer<TItemsScheme>;
REGISTER_REDUCER(TItemsReducer);

class TSerialsDownloader final : public TBaseDownloader {
public:
    using TReader = TTableReader<TItemsScheme>;
    using TWriter = TTableWriter<TSerialsScheme>;

    TSerialsDownloader(TContentInfoProvidersCache& providers, const TRPSConfig& rpsConfig,
                       TProvidersCounts& downloadCounts)
        : TBaseDownloader(providers)
        , Queue(providers, rpsConfig, Stats)
        , DownloadCounts(downloadCounts)
    {
    }

    void Do(TReader* reader, TWriter* writer) {
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            if (row.GetType() != ToString(EItemType::TvShow) || row.GetIsVoid())
                continue;

            TVideoItem item;
            if (!NSc::TValue::FromJson(item.Value(), row.GetContent()))
                continue;

            TStringBuf providerName = item->ProviderName();
            Queue.Push(providerName, item);
            ++Stats.Total;

            DownloadItems(providerName, writer);
        }

        while (DownloadAll(writer))
            ;
    }

protected:
    bool DownloadItems(TStringBuf providerName, TWriter* writer) {
        return Queue.Download(providerName, [&](const TVector<TDownloadItem<TVideoItem>>& candidates) {
            DownloadItemsImpl(writer, candidates);
        });
    }

    bool DownloadAll(TWriter* writer) {
        return Queue.DownloadAll(
            [&](const TVector<TDownloadItem<TVideoItem>>& candidates) { DownloadItemsImpl(writer, candidates); });
    }

    void DownloadItemsImpl(TWriter* writer, const TVector<TDownloadItem<TVideoItem>>& candidates) {
        TVector<TVideoItem> items(Reserve(candidates.size()));
        for (const auto& candidate : candidates)
            items.emplace_back(candidate.Item);

        DownloadSerials(items, Providers,
                        [&](size_t /* index */, const TSerialKey& key, const TSerialDescriptor& serial) {
                            if (!AddRow(writer, key, serial)) {
                                ++Stats.Failed;
                                return;
                            }
                            ++Stats.Downloaded;
                            RegisterProviderItem(DownloadCounts, key.ProviderName);
                        } /* onSuccess */,

                        [&](size_t index, const TError& error) {
                            Y_ASSERT(index < candidates.size());
                            LOG(ERR) << "Error: " << error.Msg << ", code: " << error.Code << Endl;
                            if (error.Code != HttpCodes::HTTP_TOO_MANY_REQUESTS) {
                                ++Stats.Failed;
                                return;
                            }

                            auto& candidate = candidates[index];
                            if (candidate.Retries < DEFAULT_MAX_RETRIES) {
                                ++Stats.Retries;
                                Queue.Push(candidate.Item->ProviderName(), candidate);
                            } else {
                                ++Stats.Failed;
                            }
                        } /* onError */);
    }

private:
    TBatchedMultiQueue<TVideoItem> Queue;
    TProvidersCounts& DownloadCounts;
};

class TSeasonsDownloader : public TBaseDownloader {
public:
    using TReader = TTableReader<TSerialsScheme>;
    using TWriter = TTableWriter<TSeasonsScheme>;

    static constexpr size_t ALL_SEASONS_MAX_RPS = 30;

    TSeasonsDownloader(TContentInfoProvidersCache& providers, const TRPSConfig& rpsConfig,
                       TProvidersCounts& downloadCounts)
        : TBaseDownloader(providers)
        , AllSeasonsQueue(providers, TRPSConfig(ALL_SEASONS_MAX_RPS), Stats)
        , IndividualSeasonsQueue(providers, rpsConfig, Stats)
        , DownloadCounts(downloadCounts)
    {
    }

    void Do(TReader* reader, TWriter* writer) {
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();

            const auto providerName = row.GetProviderName();
            auto serial = MakeIntrusive<TSerialDescriptor>();
            Y_ASSERT(serial);
            {
                NVideoContent::NProtos::TSerialDescriptor s;
                TStringInput input(row.GetSerial());
                if (!s.ParseFromArcadiaStream(&input)) {
                    LOG(ERR) << "Corrupted serial descriptor protobuf" << Endl;
                    continue;
                }
                if (!serial->Des(s)) {
                    LOG(ERR) << "Failed to parse serial descriptor" << Endl;
                    continue;
                }
            }

            const auto* provider = Providers.GetProvider(providerName);
            if (!provider) {
                LOG(ERR) << "Unknown provider: " << providerName << Endl;
                continue;
            }

            Stats.Total += serial->Seasons.size();

            switch (provider->GetPreferredSeasonDownloadMode()) {
                case IContentInfoProvider::EPreferredSeasonDownloadMode::All:
                    AllSeasonsQueue.Push(providerName, TAllSeasonsDownloadItem(providerName, *serial));
                    break;
                case IContentInfoProvider::EPreferredSeasonDownloadMode::Individual:
                    for (auto& season : serial->Seasons)
                        IndividualSeasonsQueue.Push(providerName, TSeasonDownloadItem(providerName, serial, season));
                    break;
            }

            DownloadItems(providerName, writer);
        }

        while (DownloadAll(writer))
            ;
    }

private:
    bool DownloadItems(TStringBuf providerName, TWriter* writer) {
        const bool all = AllSeasonsQueue.Download(
            providerName, [&](const TVector<TDownloadItem<TAllSeasonsDownloadItem>>& candidates) {
                DownloadAllItemsImpl(writer, candidates);
            });

        const bool individual = IndividualSeasonsQueue.Download(
            providerName, [&](const TVector<TDownloadItem<TSeasonDownloadItem>>& candidates) {
                DownloadIndividualItemsImpl(writer, candidates);
            });

        return all || individual;
    }

    bool DownloadAll(TWriter* writer) {
        const bool all =
            AllSeasonsQueue.DownloadAll([&](const TVector<TDownloadItem<TAllSeasonsDownloadItem>>& candidates) {
                DownloadAllItemsImpl(writer, candidates);
            });

        const bool individual =
            IndividualSeasonsQueue.DownloadAll([&](const TVector<TDownloadItem<TSeasonDownloadItem>>& candidates) {
                DownloadIndividualItemsImpl(writer, candidates);
            });

        return all || individual;
    }

    void DownloadAllItemsImpl(TWriter* writer, const TVector<TDownloadItem<TAllSeasonsDownloadItem>>& candidates) {
        TVector<TAllSeasonsDownloadItem> items(Reserve(candidates.size()));
        for (const auto& candidate : candidates)
            items.push_back(candidate.Item);

        NVideoCommon::DownloadAllSeasons(
            items, Providers, [&](size_t /* index */, const TSeasonKey& key,
                                  const TSeasonDescriptor& season) { OnSuccess(writer, key, season); } /* onSuccess */,
            [&](size_t index, const TError& error) {
                Y_ASSERT(index < candidates.size());
                auto& candidate = candidates[index];
                OnError(AllSeasonsQueue, candidate, error);
            } /* onError */);
    }

    void DownloadIndividualItemsImpl(TWriter* writer, const TVector<TDownloadItem<TSeasonDownloadItem>>& candidates) {
        TVector<TSeasonDownloadItem> items(Reserve(candidates.size()));
        for (const auto& candidate : candidates)
            items.push_back(candidate.Item);

        NVideoCommon::DownloadSeasons(items, Providers,
                                      [&](size_t /* index */, const TSeasonKey& key, const TSeasonDescriptor& season) {
                                          OnSuccess(writer, key, season);
                                      } /* onSuccess */,
                                      [&](size_t index, const TError& error) {
                                          Y_ASSERT(index < candidates.size());
                                          auto& candidate = candidates[index];
                                          OnError(IndividualSeasonsQueue, candidate, error);
                                      } /* onError */);
    }

    void OnSuccess(TWriter* writer, const TSeasonKey& key, const TSeasonDescriptor& season) {
        TSeasonsScheme row;
        if (!season.Ser(key, row)) {
            LOG(ERR) << "Failed to serialize season protobuf for key: " << key << Endl;
            ++Stats.Failed;
            return;
        }

        writer->AddRow(row);
        ++Stats.Downloaded;
        RegisterProviderItem(DownloadCounts, key.ProviderName);
    }

    template <typename TItem>
    void OnError(TBatchedMultiQueue<TItem>& queue, const TDownloadItem<TItem>& candidate, const TError& error) {
        LOG(ERR) << "Error: " << error.Msg << ", code: " << error.Code << Endl;
        if (error.Code != HttpCodes::HTTP_TOO_MANY_REQUESTS) {
            ++Stats.Failed;
            return;
        }
        if (candidate.Retries < DEFAULT_MAX_RETRIES) {
            ++Stats.Retries;
            queue.Push(candidate.Item.Provider, candidate);
        } else {
            ++Stats.Failed;
        }
    }

private:
    TBatchedMultiQueue<TAllSeasonsDownloadItem> AllSeasonsQueue;
    TBatchedMultiQueue<TSeasonDownloadItem> IndividualSeasonsQueue;
    TProvidersCounts& DownloadCounts;
};

class TProviderUniqueItemsReducer
    : public IReducer<TTableReader<TItemsScheme>, TTableWriter<TProviderUniqueItemsScheme>> {
public:
    void Do(TTableReader<TItemsScheme>* reader, TTableWriter<TProviderUniqueItemsScheme>* writer) override {
        if (!reader->IsValid())
            return;

        const auto& row = reader->GetRow();
        if (row.GetIsVoid())
            return;

        TStringBuf providerName = row.GetProviderName();
        if (!DoesProviderHaveUniqueIdsForItems(providerName))
            return;

        if (!row.HasProviderName() || !row.HasProviderItemId())
            return;

        TVideoItem item;
        if (!NSc::TValue::FromJson(item.Value(), row.GetContent())) {
            LOG(ERR) << "Failed to read JSON item content!" << Endl;
            return;
        }

        if (!AddRow(writer, item))
            return;

        reader->Next();
        if (reader->IsValid()) {
            LOG(ERR) << "Found non-unique items " << item->ProviderItemId() << "for provider " << providerName
                     << " during item reducing!" << Endl;
        }
    }
};

REGISTER_REDUCER(TProviderUniqueItemsReducer);

class TProviderUniqueEpisodesReducer
    : public IReducer<TTableReader<TSeasonsScheme>, TTableWriter<TProviderUniqueItemsScheme>> {
public:
    void Do(TTableReader<TSeasonsScheme>* reader, TTableWriter<TProviderUniqueItemsScheme>* writer) override {
        if (!reader->IsValid())
            return;

        const auto& row = reader->GetRow();
        TStringBuf providerName = row.GetProviderName();
        if (!DoesProviderHaveUniqueIdsForItems(providerName))
            return;

        TSeasonDescriptor season;
        NVideoContent::NProtos::TSeasonDescriptor s;
        TStringInput input(row.GetSeason());
        if (!s.ParseFromArcadiaStream(&input)) {
            LOG(ERR) << "Corrupted season descriptor protobuf" << Endl;
            return;
        }

        if (!season.Des(s)) {
            LOG(ERR) << "Failed to deserialize season row!" << Endl;
            return;
        }

        for (const auto& episode : season.EpisodeItems)
            AddRow(writer, episode);

        reader->Next();
        if (reader->IsValid()) {
            LOG(ERR) << "Found non-unique seasons for tv show" << season.SerialId << "for provider " << providerName
                     << " during season reducing!" << Endl;
        }

    }
};

REGISTER_REDUCER(TProviderUniqueEpisodesReducer);

TResult BuildKeys(IClient& client, const TYPath& root, const TYPath& urls, TContentInfoProvidersCache& providers) {
    if (providers.GetProvider(PROVIDER_AMEDIATEKA) || providers.GetProvider(PROVIDER_IVI)) {
        auto spec = DefaultMapReduceOperationSpec(5 * 1024 /* memoryLimitMB */);

        ForEachInMapNode(client, root, ENodeType::NT_MAP, [&](const TString& node) {
            TRichYPath input = NYT::JoinYPaths(root, node, DATA_TABLE);
            if (providers.GetProvider(PROVIDER_IVI))
                input.AddRange(TReadRange().Exact(TReadLimit().Key(IVI_HOST)));
            if (providers.GetProvider(PROVIDER_AMEDIATEKA))
                input.AddRange(TReadRange().Exact(TReadLimit().Key(AMEDIATEKA_HOST)));
            spec.AddInput<NJupiter::TUrldat>(input);
        });

        spec.AddOutput<TKeysScheme>(TRichYPath(urls).Append(true));
        spec.ReduceBy(TSortColumns{"ProviderName", "ProviderItemId", "HumanReadableId", "Type"});

        client.MapReduce(spec, MakeIntrusive<TKeysMapper>(), MakeIntrusive<TKeysReducer>());
        client.Merge(TMergeOperationSpec().AddInput(urls).Output(urls).CombineChunks(true));
    }

    TVector<std::unique_ptr<IVideoItemListHandle>> contentListHandles;
    NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::WeakMultiRequest();

    for (IContentInfoProvider* provider : providers) {
        if (provider->IsContentListAvailable())
            contentListHandles.push_back(provider->MakeContentListRequest(multiRequest));
    }

    if (contentListHandles.empty())
        return NBASS::ResultSuccess();

    auto writer = client.CreateTableWriter<TKeysScheme>(TRichYPath(urls).Append(true));

    for (auto& handle : contentListHandles) {
        TVideoItemList contentList;
        if (const auto error = handle->WaitAndParseResponse(contentList)) {
            LOG(ERR) << "Cannot fetch root content list: " << error << Endl;
            return error;
        }

        for (const TVideoItem& item : contentList.Items) {
            TKeysScheme row;
            row.SetProviderName(TString{*item->ProviderName()});
            row.SetProviderItemId(TString{*item->ProviderItemId()});
            row.SetType(TString{*item->Type()});
            writer->AddRow(row);
        }
    }

    return NBASS::ResultSuccess();
}

bool BuildItems(IClient& client, const TYPath& keys, const TYPath& items, TContentInfoProvidersCache& providers,
                const TRPSConfig& rpsConfig, const TProvidersCounts& itemsExpected) {
    {
        auto reader = client.CreateTableReader<TKeysScheme>(keys);
        auto writer = client.CreateTableWriter<TItemsScheme>(TRichYPath(items).Append(true));

        TProvidersCounts actualCounts;
        TItemsDownloader downloader(providers, rpsConfig, actualCounts);
        downloader.Do(reader.Get(), writer.Get());
        if (!IsProviderDataValid(itemsExpected, actualCounts, "Items"))
            return false;
    }

    const auto columns = TSortColumns{"ProviderName", "ProviderItemId", "HumanReadableId", "KinopoiskId", "Type"};

    client.Sort(TSortOperationSpec{}.AddInput(items).Output(items).SortBy(columns));
    {
        TReduceOperationSpec spec;
        spec.AddInput<TItemsScheme>(items).AddOutput<TItemsScheme>(items).ReduceBy(columns);
        client.Reduce(spec, MakeIntrusive<TItemsReducer>());
    }
    return true;
}

class TContentGroupMapper
        : public IMapper<TTableReader<TNode>, TTableWriter<TNode>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            TNode result = TNode::CreateMap();
            result["ContentGroupID"] = row["ContentGroupID"];
            if (row.HasKey("ContentTypeID")) {
                if (IsIn({6, 7, 20, 21}, row["ContentTypeID"].AsInt64())) {
                    result["ProviderItemId"] = row["UUID"];
                    result["onto_id"] = "";
                    writer->AddRow(result);
                }
            } else if (row.HasKey("ResourceName") && row["ResourceName"].AsString() == "onto_id") {
                result["onto_id"] = row["Value"];
                result["ProviderItemId"] = "";
                writer->AddRow(result);
            }

        }
    }
};

REGISTER_MAPPER(TContentGroupMapper);

class TContentGroupReducer
        : public IReducer<TTableReader<TNode>, TTableWriter<TNode>> {
public:
    void Do(TTableReader<TNode>* reader, TTableWriter<TNode>* writer) override {
        if (!reader->IsValid())
            return;

        TNode result = TNode::CreateMap();

        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            if (row.HasKey("ProviderItemId") && !row["ProviderItemId"].AsString().Empty()) {
                result["ProviderItemId"] = row["ProviderItemId"].AsString();
            }
            if (row.HasKey("onto_id") && !row["onto_id"].AsString().Empty()) {
                result["onto_id"] = row["onto_id"];
            }
        }
        if (result.HasKey("ProviderItemId") && result.HasKey("onto_id")) {
            result["ProviderName"] = "kinopoisk";
            writer->AddRow(result);
        }
    }
};

REGISTER_REDUCER(TContentGroupReducer);

class TVideoItemsOntoidsReducer
        : public IReducer<TTableReader<TNode>, TTableWriter<TItemsScheme>> {
public:
    void Do(TTableReader<TNode>* reader, TTableWriter<TItemsScheme>* writer) override {
        if (!reader->IsValid())
            return;

        TVideoItem item;
        TString ontoid;

        for ( ; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            if (reader->GetTableIndex() == 0) {
                if (!NSc::TValue::FromJson(item.Value(), row["Content"].AsString())) {
                    LOG(ERR) << "Cannot parse video item content for item: " << row["ProviderItemId"].AsString() << Endl;
                    return;
                }
            } else {
                ontoid = row["onto_id"].AsString();
            }
        }
        if (!item.Value().IsNull()) {
            if (!ontoid.empty()) {
                item->MiscIds().OntoId() = ontoid;
            }
            AddRow(writer, item, false);
        }
    }
};

REGISTER_REDUCER(TVideoItemsOntoidsReducer);

class TSerialEpisodesMapper
        : public IMapper<TTableReader<TSeasonsScheme>, TTableWriter<TEpisodesScheme>> {
public:
    void Do(TTableReader<TSeasonsScheme>* reader, TTableWriter<TEpisodesScheme>* writer) override {
        if (!reader->IsValid())
            return;

        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();

            TString season = row.GetSeason();
            NVideoContent::NProtos::TSeasonDescriptor seasonProto;
            Y_PROTOBUF_SUPPRESS_NODISCARD seasonProto.ParseFromString(season);
            for (auto item: seasonProto.GetEpisodeItems()) {
                NVideoContent::NProtos::TEpisodeDescriptorRow ansRow;
                NJson::TJsonValue jsonItem = NAlice::JsonFromString(item);

                ansRow.SetProviderItemId(jsonItem["provider_item_id"].GetString());
                ansRow.SetSeasonNumber(jsonItem["season"].GetUInteger());
                ansRow.SetEpisodeNumber(jsonItem["episode"].GetUInteger());
                writer->AddRow(ansRow);
            }

        }
    }
};
REGISTER_MAPPER(TSerialEpisodesMapper);

void UpdateVideoItemsWihtOntoids(IClient& client, const TYPath& items, const TYPath& ontoids) {
    TMapReduceOperationSpec mapReduceSpec;
    mapReduceSpec.AddInput<NYT::TNode>("//home/video-hosting/base/ContentGroup").AddInput<NYT::TNode>("//home/video-hosting/base/ContentResource").AddOutput<TNode>(ontoids).ReduceBy("ContentGroupID");

    client.Create(ontoids, NYT::ENodeType::NT_TABLE, NYT::TCreateOptions().Force(true));
    client.MapReduce(mapReduceSpec, MakeIntrusive<TContentGroupMapper>(), MakeIntrusive<TContentGroupReducer>());

    const auto columns = TSortColumns{"ProviderName", "ProviderItemId"};

    client.Sort(TSortOperationSpec{}.AddInput(ontoids).Output(ontoids).SortBy(columns));
    TReduceOperationSpec spec;
    spec.AddInput<NYT::TNode>(items).AddInput<NYT::TNode>(ontoids).AddOutput<TItemsScheme>(items).ReduceBy(columns);
    client.Reduce(spec, MakeIntrusive<TVideoItemsOntoidsReducer>());
    const auto sortColumns = TSortColumns{"ProviderName", "ProviderItemId", "HumanReadableId", "KinopoiskId", "Type"};
    client.Sort(TSortOperationSpec{}.AddInput(items).Output(items).SortBy(sortColumns));
}

void BuildEpisodes(IClient& ytClient, const TYPath& seasons, const TYPath& episodes) {
    TMapOperationSpec spec;
    spec.AddInput<TSeasonsScheme>(seasons).AddOutput<TEpisodesScheme>(episodes);
    ytClient.Map(spec, MakeIntrusive<TSerialEpisodesMapper>());
    ytClient.Sort(TSortOperationSpec{}.AddInput(episodes).Output(episodes).SortBy(NVideoContent::TVideoEpisodesTableTraits::PRIMARY_KEYS));
}

bool BuildSerials(IClient& client, const TYPath& items, const TYPath& serials, TContentInfoProvidersCache& providers,
                  const TRPSConfig& rpsConfig, const TProvidersCounts& serialsExpected) {
    auto reader = client.CreateTableReader<TItemsScheme>(items);
    auto writer = client.CreateTableWriter<TSerialsScheme>(TRichYPath(serials).Append(true));

    TProvidersCounts actualCounts;
    TSerialsDownloader downloader(providers, rpsConfig, actualCounts);
    downloader.Do(reader.Get(), writer.Get());
    return IsProviderDataValid(serialsExpected, actualCounts, "Serials");
}

bool BuildSeasons(IClient& client, const TYPath& serials, const TYPath& seasons, TContentInfoProvidersCache& providers,
                  const TRPSConfig& rpsConfig, const TProvidersCounts& seasonsExpected) {
    {
        auto reader = client.CreateTableReader<TSerialsScheme>(serials);
        auto writer = client.CreateTableWriter<TSeasonsScheme>(TRichYPath(seasons).Append(true));

        TProvidersCounts actualCounts;
        TSeasonsDownloader downloader(providers, rpsConfig, actualCounts);
        downloader.Do(reader.Get(), writer.Get());

        if (!IsProviderDataValid(seasonsExpected, actualCounts, "Seasons"))
            return false;
    }

    const auto columns = TSortColumns{"ProviderName", "SerialId", "ProviderNumber"};
    client.Sort(TSortOperationSpec{}.AddInput(seasons).Output(seasons).SortBy(columns));
    return true;
}

void BuildProviderUniqueItems(IClient& client, const TYPath& items, const TYPath& seasons,
                              const TYPath& providerUniqueItems) {
    {
        const auto columns = TSortColumns{"ProviderName", "ProviderItemId", "HumanReadableId", "KinopoiskId", "Type"};
        TReduceOperationSpec spec;
        spec.AddInput<TItemsScheme>(items)
            .AddOutput<TProviderUniqueItemsScheme>(providerUniqueItems)
            .ReduceBy(columns);
        client.Reduce(spec, MakeIntrusive<TProviderUniqueItemsReducer>());
    }

    {
        const auto columns = TSortColumns{"ProviderName", "SerialId", "ProviderNumber"};
        TReduceOperationSpec spec;
        spec.AddInput<TSeasonsScheme>(seasons)
            .AddOutput<TProviderUniqueItemsScheme>(TRichYPath(providerUniqueItems).Append(true))
            .ReduceBy(columns);
        client.Reduce(spec, MakeIntrusive<TProviderUniqueEpisodesReducer>());
    }
}

size_t TryParseIntNumber(TStringBuf paramName, TStringBuf providerName, TStringBuf userArg) {
    size_t parsedValue = 0;
    if (!TryFromString<size_t>(userArg, parsedValue))
        ythrow yexception() << paramName << " for provider " << providerName << " must be an unsigned integer number!";
    return parsedValue;
}

TProvidersCounts TryParseProviderVerificationCounts(TStringBuf paramName, TStringBuf counts) {
    TProvidersCounts countsMap;
    auto addCheckEntry = [&countsMap, paramName](const TStringBuf providerName, const TStringBuf val) {
        size_t parsedValue = TryParseIntNumber(paramName, providerName, val);
        countsMap[providerName] = parsedValue;
    };
    ScanKeyValue<true, ',', '='>(counts, addCheckEntry);
    return countsMap;
}

int Main(int argc, const char* argv[]) {
    Initialize(argc, argv);

    TString ytRoot;

    bool updateKeys;
    bool updateItems;
    bool updateSerials;
    bool updateSeasons;
    bool updateProviderUniqueItems;

    TString providerList;
    TString rpsList;

    TString itemsCountList;
    TString serialsCountList;
    TString seasonsCountList;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();

    const TString availableProviders =
        " Available providers: " + Join(", ", PROVIDER_AMEDIATEKA, PROVIDER_IVI, PROVIDER_KINOPOISK, PROVIDER_OKKO);

    opts.AddLongOption("yt-root", "YT video root directory.").StoreResult(&ytRoot).RequiredArgument("YT_ROOT");
    opts.AddLongOption("update-keys", "Do we need to update keys?").StoreResult(&updateKeys).DefaultValue("yes");
    opts.AddLongOption("update-items", "Do we need to update items?").StoreResult(&updateItems).DefaultValue("yes");
    opts.AddLongOption("update-serials", "Do we need to update serials?")
        .StoreResult(&updateSerials)
        .DefaultValue("yes");
    opts.AddLongOption("update-seasons", "Do we need to update seasons?")
        .StoreResult(&updateSeasons)
        .DefaultValue("yes");
    opts.AddLongOption("update-provider-unique-items", "Do we need to update provider-unique items?")
        .StoreResult(&updateProviderUniqueItems)
        .DefaultValue("yes");

    opts.AddLongOption("enabled-providers",
                       "Comma-separated list of providers to download content from." + availableProviders)
        .StoreResult(&providerList)
        .DefaultValue(Join(",", PROVIDER_AMEDIATEKA, PROVIDER_IVI, PROVIDER_KINOPOISK));
    opts.AddLongOption("rps-config",
                       "Comma-separated list with provider name and its RPS as key-values," + availableProviders)
        .StoreResult(&rpsList)
        .DefaultValue("");

    opts.AddLongOption("expected-items-count", "Comma-separated list with provider name and its expected item count")
        .StoreResult(&itemsCountList)
        .DefaultValue("");
    opts.AddLongOption("expected-serials-count",
                       "Comma-separated list with provider name and its expected serials count")
        .StoreResult(&serialsCountList)
        .DefaultValue("");
    opts.AddLongOption("expected-seasons-count",
                       "Comma-separated list with provider name and its expected seasons count")
        .StoreResult(&seasonsCountList)
        .DefaultValue("");

    NLastGetopt::TOptsParseResult result(&opts, argc, argv);

    TString tvm2BassId = GetEnvOrThrow(ENV_TVM2_BASS_ID);
    TString tvm2BassSecret = GetEnvOrThrow(ENV_TVM2_BASS_SECRET);
    const auto ottTicket = GetSingleTvm2Ticket(tvm2BassId, tvm2BassSecret, KINOPOISK_TVM_ID);

    TVector<TStringBuf> providers;
    Split(TStringBuf(providerList), ",", providers);

    TRPSConfig rpsConfig;
    auto addRPSConfigEntry = [&rpsConfig](const TStringBuf providerName, const TStringBuf val) {
        size_t maxRPS = TryParseIntNumber("RPS", providerName, val);
        rpsConfig.AddRPSLimit(providerName, maxRPS);
    };
    ScanKeyValue<true, ',', '='>(rpsList, addRPSConfigEntry);

    auto itemsExpected = TryParseProviderVerificationCounts("Items count", itemsCountList);
    auto serialsExpected = TryParseProviderVerificationCounts("Serials count", serialsCountList);
    auto seasonsExpected = TryParseProviderVerificationCounts("Seasons count", seasonsCountList);

    TUAPIDownloaderProviderFactory uapiFactory(ottTicket);

    TIviGenresDelegate genresDelegate;
    TAutoUpdateIviGenres genres{genresDelegate};

    // Episode RPS is set for UAPI providers only.
    TContentInfoProvidersCache providersCache{uapiFactory, genres, providers, rpsConfig};

    auto ytClient = CreateClient(TString{NVideoContent::YT_PROXY});
    Y_ASSERT(ytClient);

    // Finds latest batch of urldat tables. Batches are named
    // according to dates, e.g. typical URLDAT_ROOT directory looks
    // like:
    //
    // * 20180917-031231
    // * 20180918-085023
    // * 20180920-005624
    //
    // and we need to find the latest one.
    TMaybe<TString> latest;
    ForEachInMapNode(*ytClient, URLDAT_ROOT, ENodeType::NT_MAP, [&latest](const TString& name) {
        if (!latest || *latest < name)
            latest = name;
    });

    if (!latest) {
        LOG(ERR) << "Failed to find latest map node at " << URLDAT_ROOT << Endl;
        return EXIT_FAILURE;
    }

    const auto keys = NYT::JoinYPaths(ytRoot, NVideoContent::TVideoKeysTableTraits::YT_NAME);
    const auto items = NYT::JoinYPaths(ytRoot, NVideoContent::TVideoItemsLatestTableTraits::YT_NAME);
    const auto serials = NYT::JoinYPaths(ytRoot, NVideoContent::TVideoSerialsTableTraits::YT_NAME);
    const auto seasons = NYT::JoinYPaths(ytRoot, NVideoContent::TVideoSeasonsTableTraits::YT_NAME);
    const auto providerUniqueItems =
        NYT::JoinYPaths(ytRoot, NVideoContent::TProviderUniqueItemsTableTraitsV2::YT_NAME);
    const auto episodes = NYT::JoinYPaths(ytRoot, NVideoContent::TVideoEpisodesTableTraits::YT_NAME);
    const auto ontoids = NYT::JoinYPaths(ytRoot, "ontoids");

    if (updateKeys) {
        LOG(INFO) << "Filling " << keys << "..." << Endl;
        CreateTable<TKeysScheme>(*ytClient, keys);
        if (const auto error = BuildKeys(*ytClient, URLDAT_ROOT + "/" + (*latest), keys, providersCache))
            return EXIT_FAILURE;
    }

    if (updateItems) {
        LOG(INFO) << "Filling " << items << "..." << Endl;
        CreateTable<TItemsScheme>(*ytClient, items);
        if (!BuildItems(*ytClient, keys, items, providersCache, rpsConfig, itemsExpected))
            return EXIT_FAILURE;

        LOG(INFO) << "Updating items with ontoids ..." << Endl;
        UpdateVideoItemsWihtOntoids(*ytClient, items, ontoids);
    }

    if (updateSerials) {
        LOG(INFO) << "Filling " << serials << "..." << Endl;
        CreateTable<TSerialsScheme>(*ytClient, serials);
        if (!BuildSerials(*ytClient, items, serials, providersCache, rpsConfig, serialsExpected))
            return EXIT_FAILURE;
    }

    if (updateSeasons) {
        LOG(INFO) << "Filling " << seasons << "..." << Endl;
        CreateTable<TSeasonsScheme>(*ytClient, seasons);
        if (!BuildSeasons(*ytClient, serials, seasons, providersCache, rpsConfig, seasonsExpected))
            return EXIT_FAILURE;
        BuildEpisodes(*ytClient, seasons, episodes);
    }

    if (updateProviderUniqueItems) {
        LOG(INFO) << "Filling " << providerUniqueItems << "..." << Endl;
        CreateTable<TProviderUniqueItemsScheme>(*ytClient, providerUniqueItems);
        BuildProviderUniqueItems(*ytClient, items, seasons, providerUniqueItems);
    }
    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, const char* argv[])
try {
    return Main(argc, argv);
} catch (const yexception& e) {
    LOG(ERR) << e << Endl;
    return EXIT_FAILURE;
}
