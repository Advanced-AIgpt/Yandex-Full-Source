#include "kinopoisk_utils.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/video_content/common.h>
#include <alice/bass/libs/ydb_helpers/table.h>

#include <util/generic/algorithm.h>
#include <util/stream/output.h>
#include <util/string/split.h>
#include <util/string/join.h>
#include <util/string/printf.h>
#include <util/system/yassert.h>

#include <algorithm>
#include <functional>

namespace NVideoCommon {
namespace {
constexpr size_t KINOPOISK_SVOD_TABLES_TO_KEEP = 2;

template <typename TIt, typename TFn>
NYdb::NTable::TDataQueryResult ForEachSVOD(NYdb::NTable::TSession& session, NYdb::NTable::TDataQuery& query,
                                           const NYdb::NTable::TTxControl& tx, TIt begin, TIt end, TFn&& fn) {
    // It's important not to pass empty list here, because YDB SDK
    // requires struct type to be specified for empty list, and it's
    // much easier to avoid empty lists therefore.
    Y_ASSERT(begin != end);

    auto params = session.GetParamsBuilder().AddParam("$kinopoiskIds", ToKinopoiskIdsList(begin, end)).Build();

    auto result = query.Execute(tx, std::move(params)).GetValueSync();
    if (!result.IsSuccess())
        return result;

    Y_ASSERT(result.GetResultSets().size() == 1);

    auto parser = result.GetResultSetParser(0);
    Y_ASSERT(parser.ColumnsCount() == 1);

    while (parser.TryNextRow())
        fn(parser.ColumnParser(0).GetString());

    return result;
}

TMaybe<TSVODError> GetKinopoiskSVOD(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& path,
                                    const NYdb::NTable::TRetryOperationSettings& settings, const TVector<TString>& in,
                                    THashSet<TString>& out) {
    constexpr size_t batchSize = 1000;

    if (in.empty())
        return Nothing();

    if (in.size() == 1) {
        const auto& kinopoiskId = in[0];

        const auto status = IsKinopoiskSVOD(client, path, kinopoiskId, settings);
        if (!status.IsSuccess())
            return TSVODError{status};
        if (status.IsSVOD())
            out.insert(kinopoiskId);
        return Nothing();
    }

    const auto query = Sprintf(R"(
                               PRAGMA TablePathPrefix("%s");
                               DECLARE $kinopoiskIds AS "List<Struct<KinopoiskId:String>>";

                               SELECT ks.KinopoiskId FROM AS_TABLE($kinopoiskIds) AS ks
                                   INNER JOIN [%s] AS ts
                                   ON ks.KinopoiskId == ts.KinopoiskId;
                               )",
                               path.Database.c_str(), path.Name.c_str());

    THashSet<TString> svod;
    auto onSVOD = [&svod](const TString& kinopoiskId) { svod.insert(kinopoiskId); };

    Y_ASSERT(!in.empty());

    const auto status = client.RetryOperationSync([&](NYdb::NTable::TSession session) -> NYdb::TStatus {
        const auto prepareResult = session.PrepareDataQuery(query).GetValueSync();
        if (!prepareResult.IsSuccess())
            return prepareResult;

        auto query = prepareResult.GetQuery();

        if (in.size() <= batchSize)
            return ForEachSVOD(session, query, NYdbHelpers::SerializableRW(), in.begin(), in.end(), onSVOD);

        auto result =
            ForEachSVOD(session, query, NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()),
                        in.begin(), in.begin() + batchSize, onSVOD);
        if (!result.IsSuccess())
            return result;

        for (size_t from = batchSize; from < in.size();) {
            const size_t to = from + Min(in.size() - from, batchSize);
            auto tx = result.GetTransaction();

            if (to == in.size()) {
                ForEachSVOD(session, query, NYdb::NTable::TTxControl::Tx(*tx).CommitTx(), in.begin() + from,
                            in.begin() + to, onSVOD);
            } else {
                ForEachSVOD(session, query, NYdb::NTable::TTxControl::Tx(*tx), in.begin() + from, in.begin() + to,
                            onSVOD);
            }

            from = to;
        }

        return result;
    }, settings);

    if (!status.IsSuccess())
        return TSVODError{status};

    out.insert(svod.begin(), svod.end());

    return {};
}

NHttpFetcher::TRequestPtr AttachKinopoiskRequest(ISourceRequestFactory& source, TStringBuf path,
                                                 const TMaybe<TStringBuf>& kinopoiskId,
                                                 NHttpFetcher::IMultiRequest::TRef multiRequest) {
    auto request = source.AttachRequest(path, multiRequest);
    Y_ASSERT(request);
    if (kinopoiskId)
        request->AddCgiParam("kpId", *kinopoiskId);
    return request;
}

TString MakeUrlWithSizes(TStringBuf inputUrl, TStringBuf width, TStringBuf height) {
    inputUrl.ChopSuffix("/orig");
    TStringBuilder url;
    url << inputUrl;
    url << '/' << width << 'x' << height << '/';
    return url;
}

TString JoinStrings(TStringBuf separator, const NSc::TArray& stringsArray) {
    TVector<TStringBuf> strings{Reserve(stringsArray.size())};
    for (const auto& v : stringsArray) {
        strings.emplace_back(v.GetString());
    }
    return JoinSeq(separator, strings);
}

// Kinopoisk tv-show episodes titles can be like
// [tv-show - season N - episode M - episode name],
// [tv-show - season N - episode M]
// or any other format
TStringBuf ExtractKinopoiskEpisodeTitle(TStringBuf title) {
    TVector<TStringBuf> parts = StringSplitter(title).SplitByString(" - ");

    // return episode name or just 'episode M' if possible
    if (parts.size() > 2) {
        return parts.back();
    }

    // in any other case return full title itself
    return title;
}

double ExtractKinopoiskRating(const NSc::TArray& ratingsArray) {
    for (const auto& r : ratingsArray) {
        if (r.TrySelect("type") == "KINOPOISK") {
            return r.TrySelect("value").GetNumber();
        }
    }

    return 0;
}

EItemType ParseItemType(const NSc::TValue& providerItem) {
    const TStringBuf itemType = providerItem["filmType"].GetString();
    if (itemType == TStringBuf("MOVIE")) {
        return EItemType::Movie;
    }
    if (itemType == TStringBuf("TV_SERIES")) {
        return EItemType::TvShow;
    }
    if (itemType == TStringBuf("EPISODE")) {
        return EItemType::TvShowEpisode;
    }

    LOG(WARNING) << "Unknown kinopoisk item type: " << itemType << Endl;
    return EItemType::Null;
}

void ParseContentItem(const NSc::TValue& elem, TVideoItemScheme& item, bool enableShowingItemsComingSoon);

void ParseTvShowSeasons(const NSc::TValue& elem, TVideoItemScheme& item, bool enableShowingItemsComingSoon) {
    const NSc::TArray& elemSeasons = elem.TrySelect("seasons").GetArray();
    auto kinopoiskInfo = item.KinopoiskInfo();

    ui64 backupNumber = 0;
    for (const auto& season : elemSeasons) {
        ++backupNumber;

        using TSeason = TSchemeHolder<NBassApi::TVideoItem<TSchemeTraits>::TSeason>;
        TSeason newSeason;
        const auto providerNumber = season.TrySelect("number").GetIntNumber();
        newSeason->Id() = ToString(providerNumber);
        newSeason->ProviderNumber() = (providerNumber > 0) ? providerNumber : backupNumber;
        const NSc::TArray& episodes = season.TrySelect("episodes").GetArray();
        bool seasonHasReleasedEpisodes = false;

        for (const auto& episode : episodes) {
            bool isReleased = episode.TrySelect("released").GetBool(true);
            if (!enableShowingItemsComingSoon && !isReleased)
                continue;

            seasonHasReleasedEpisodes |= isReleased;

            TVideoItem newEpisode;
            newEpisode->TvShowItemId() = item.ProviderItemId();
            newEpisode->Type() = ToString(EItemType::TvShowEpisode);
            ParseContentItem(episode, newEpisode.Scheme(), enableShowingItemsComingSoon);
            newSeason->Episodes().Add() = newEpisode.Scheme();
        }

        newSeason->Soon() = !seasonHasReleasedEpisodes;

        // TODO: remove the condition and the loop below after the flag is enabled (@a-sidorin, QUASARSUP-326).
        if (!newSeason->Episodes().Empty())
            kinopoiskInfo.Seasons().Add() = newSeason.Scheme();
    }

    if (enableShowingItemsComingSoon)
        return;

    // Set SeasonsCount()'s of items to the actual season count.
    const size_t seasonCount = kinopoiskInfo.Seasons().Size();
    item.SeasonsCount() = seasonCount;
    for (size_t seasonIdx = 0; seasonIdx < seasonCount; ++seasonIdx) {
        auto season = kinopoiskInfo.Seasons(seasonIdx);
        for (size_t episodeIdx = 0; episodeIdx < season.Episodes().Size(); ++episodeIdx)
            season.Episodes(episodeIdx)->SeasonsCount() = seasonCount;
    }
}

void ParseContentItem(const NSc::TValue& elem, TVideoItemScheme& item, bool enableShowingItemsComingSoon) {
    item.ProviderName() = PROVIDER_KINOPOISK;

    EItemType type;
    if (item.Type()->empty()) {
        type = ParseItemType(elem);
        item.Type() = ToString(type);
    } else {
        type = FromString<EItemType>(item.Type());
    }

    item.ProviderItemId() = elem.TrySelect("filmId").GetString(item.HumanReadableId());
    item.MiscIds().KinopoiskUuid() = item.ProviderItemId();
    if (const NSc::TValue& kinopoiskId = elem.TrySelect("kpId"); !kinopoiskId.IsNull())
        item.MiscIds().Kinopoisk() = kinopoiskId.ForceString();

    if (type == EItemType::TvShowEpisode) {
        item.Name() = ExtractKinopoiskEpisodeTitle(elem.TrySelect("title").GetString());
    } else {
        item.Name() = elem.TrySelect("title").GetString();
    }

    item.Description() = elem.TrySelect("description").GetString();
    item.Duration() = elem.TrySelect("duration").GetIntNumber();

    if ((type == EItemType::TvShow) && item.KinopoiskInfo().Seasons().Empty())
        ParseTvShowSeasons(elem, item, enableShowingItemsComingSoon);

    const TStringBuf posterUrl = elem.TrySelect("posterUrl").GetString();
    if (type == EItemType::Movie || type == EItemType::TvShow) {
        item.CoverUrl2X3() = MakeUrlWithSizes(posterUrl, TStringBuf("328"), TStringBuf("492"));
        item.ThumbnailUrl2X3Small() = MakeUrlWithSizes(posterUrl, TStringBuf("88"), TStringBuf("132"));
    }
    const TStringBuf coverUrl = elem.TrySelect("coverUrl");
    if (type == EItemType::Movie || type == EItemType::TvShow) {
        item.CoverUrl16X9() = MakeUrlWithSizes(coverUrl, TStringBuf("1920"), TStringBuf("1080"));
    } else if (type == EItemType::TvShowEpisode) {
        item.ThumbnailUrl16X9() =
            MakeUrlWithSizes(elem.TrySelect("imageUrl").GetString(), TStringBuf("504"), TStringBuf("284"));
    }

    // TODO Background16x9 ?

    item.Genre() = JoinStrings(", ", elem.TrySelect("genres").GetArray());
    item.Rating() = ExtractKinopoiskRating(elem.TrySelect("ratings").GetArray());
    item.ReleaseYear() = elem.TrySelect("years/0").GetIntNumber();

    item.Directors() = JoinStrings(", ", elem.TrySelect("directors").GetArray());
    item.Actors() = JoinStrings(", ", elem.TrySelect("actors").GetArray());

    // item.MiscIds().Imdb() ?

    item.Available() = true;
    item.Soon() = !elem.TrySelect("released").GetBool(true);

    {
        const auto minAge = elem["restrictionAge"];
        if (minAge.IsIntNumber()) {
            item.MinAge() = minAge.GetIntNumber();
            item.AgeLimit() = ToString(minAge.GetIntNumber());
        } else if (!minAge.IsNull()) {
            LOG(WARNING) << "Unexpected restrictionAge field type: " << minAge.ToJson() << Endl;
        }
    }
}

class TKinopoiskContentInfoHandler: public IVideoItemHandle {
public:
    TKinopoiskContentInfoHandler(TKinopoiskSourceRequestFactoryWrapper& source, TLightVideoItemConstScheme item,
                                 bool enableShowingItemsComingSoon, NHttpFetcher::IMultiRequest::TRef multiRequest)
        : EnableShowingItemsComingSoon(enableShowingItemsComingSoon)
    {
        TStringBuilder path;
        path << "films";

        TMaybe<TStringBuf> kinopoiskId;
        if (const auto& kinopoisk = item.MiscIds().Kinopoisk(); !kinopoisk.IsNull())
            kinopoiskId = kinopoisk;
        else
            path << "/" << item.ProviderItemId();
        auto request = AttachKinopoiskRequest(source, path, kinopoiskId, multiRequest);
        Y_ASSERT(request);
        Handle = request->Fetch();
    }

    TResult WaitAndParseResponse(TVideoItem& item) override {
        auto response = Handle->Wait();

        if (response->IsError()) {
            TStringBuilder error;
            error << "item " << TString{*item->ProviderItemId()}.Quote() << " failed to resolve: " << response->GetErrorText();
            LOG(ERR) << "Kinopoisk error: " << error << " (" << response->Data << ")" << Endl;
            return TError{response->GetErrorText(), response->Code};
        }

        if (const auto error = ParseResponse(response, item))
            return error;

        return {};
    }

private:
    TResult ParseResponse(NHttpFetcher::TResponse::TRef response, TVideoItem& item) const {
        NSc::TValue json = NSc::TValue::FromJson(response->Data);
        ParseContentItem(json, item.Scheme(), EnableShowingItemsComingSoon);
        return {};
    }

    NHttpFetcher::THandle::TRef Handle;
    bool EnableShowingItemsComingSoon;
};

class TSerialDescriptorHandle : public ISerialDescriptorHandle {
public:
    TSerialDescriptorHandle(ISourceRequestFactory& source, TVideoItemConstScheme tvShowItem,
                            bool enableShowingItemsComingSoon, NHttpFetcher::IMultiRequest::TRef multiRequest)
        : EnableShowingItemsComingSoon(enableShowingItemsComingSoon) {
        TvShowItem->Assign(tvShowItem);
        TStringBuf serialId = tvShowItem->ProviderItemId();
        if (tvShowItem->KinopoiskInfo().Seasons().Empty()) {
            TString path = TStringBuilder() << TStringBuf("films/") << serialId;

            auto request = AttachKinopoiskRequest(source, path, Nothing() /* kinopoiskId */, multiRequest);
            Y_ASSERT(request);
            Handle = request->Fetch();
        }
    }

    // ISerialDescriptorHandle overrides:
    TResult WaitAndParseResponse(TSerialDescriptor& serial) override {
        TVideoItem item{TvShowItem};
        const auto serialId = TString{*item->ProviderItemId()};

        if (Handle) {
            auto response = Handle->Wait();

            if (!response) {
                const TString error = TStringBuilder() << "item " << item->ProviderItemId() << " failed to resolve";
                LOG(ERR) << "kinopoisk error: " << error << Endl;
                return TError{error};
            }

            if (response->IsError()) {
                const TString error = TStringBuilder() << "item " << item->ProviderItemId()
                                                       << " failed to resolve: " << response->GetErrorText();
                LOG(ERR) << "kinopoisk error: " << error << Endl;
                return TError{error, response->Code};
            }

            NSc::TValue data;
            if (!NSc::TValue::FromJson(data, response->Data)) {
                const TString error = TStringBuilder() << "item " << item->ProviderItemId()
                                                       << " failed to parse response data";
                LOG(ERR) << "kinopoisk error: " << error << Endl;
                return TError{error};
            }

            ParseContentItem(data, item.Scheme(), EnableShowingItemsComingSoon);
        }

        for (ui32 i = 0; i < item->KinopoiskInfo().Seasons().Size(); ++i) {
            TSeasonDescriptor season;
            auto kinopoiskSeason = item->KinopoiskInfo().Seasons(i);

            season.SerialId = serialId;
            season.Index = i;
            season.ProviderNumber = kinopoiskSeason.ProviderNumber();
            season.Id = ToString(kinopoiskSeason.Id());
            for (const auto& episode : kinopoiskSeason.Episodes()) {
                season.EpisodeIds.push_back(TString{*episode.ProviderItemId()});
                season.EpisodeItems.push_back(TVideoItem(*episode.GetRawValue()));
            }
            season.EpisodesCount = season.EpisodeItems.size();
            season.Soon = kinopoiskSeason.Soon();
            serial.Seasons.push_back(std::move(season));
        }
        serial.Id = serialId;

        return {};
    }

private:
    TVideoItem TvShowItem;
    bool EnableShowingItemsComingSoon;
    NHttpFetcher::THandle::TRef Handle;
};
} // namespace

NYdb::TValue ToKinopoiskIdStruct(const TString& kinopoiskId) {
    NYdb::TValueBuilder builder;

    builder.BeginStruct();
    builder.AddMember("KinopoiskId").String(kinopoiskId);
    builder.EndStruct();
    return builder.Build();
}

NYdb::TValue ToIdStruct(const ui64 Id) {
    NYdb::TValueBuilder builder;

    builder.BeginStruct();
    builder.AddMember("Id").Uint64(Id);
    builder.EndStruct();
    return builder.Build();
}

NYdb::TValue ToIdsList(const TVector<ui64>& ids) {
    Y_ASSERT(!ids.empty());

    NYdb::TValueBuilder builder;
    builder.BeginList();
    for (size_t i = 0; i < ids.size(); ++i)
        builder.AddListItem(ToIdStruct(ids[i]));
    builder.EndList();

    return builder.Build();
}

TMaybe<TSVODError> RemoveIfNotKinopoiskSVOD(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& path,
                                            const NYdb::NTable::TRetryOperationSettings& settings,
                                            const TVector<TString>& in, TVector<TString>& out) {
    THashSet<TString> svod;
    if (const auto error = GetKinopoiskSVOD(client, path, settings, in, svod))
        return error;
    for (const auto& kinopoiskId : in) {
        if (svod.find(kinopoiskId) != svod.end())
            out.push_back(kinopoiskId);
    }
    return {};
}

TMaybe<TSVODError> RemoveIfNotKinopoiskSVOD(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& path,
                                            const TVector<TString>& in, TVector<TString>& out) {
    return RemoveIfNotKinopoiskSVOD(client, path, NYdbHelpers::DefaultYdbRetrySettings(), in, out);
}

TIsSVODStatus IsKinopoiskSVOD(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& path,
                              const TString& kinopoiskId, const NYdb::NTable::TRetryOperationSettings& settings) {
    const auto query = Sprintf(R"(
                               PRAGMA TablePathPrefix("%s");
                               DECLARE $kinopoiskId AS String;
                               SELECT * FROM %s WHERE KinopoiskId == $kinopoiskId;
                               )",
                               path.Database.c_str(), path.Name.c_str());

    bool isSVOD = false;
    auto status = client.RetryOperationSync(
        [&](NYdb::NTable::TSession session) -> NYdb::TStatus {
            const auto prepareResult = session.PrepareDataQuery(query).GetValueSync();
            if (!prepareResult.IsSuccess())
                return prepareResult;

            auto query = prepareResult.GetQuery();
            auto params = query.GetParamsBuilder().AddParam("$kinopoiskId").String(kinopoiskId).Build().Build();

            auto result = query.Execute(NYdbHelpers::SerializableRW(), std::move(params)).GetValueSync();
            if (!result.IsSuccess())
                return result;

            Y_ASSERT(result.GetResultSets().size() == 1);
            NYdb::TResultSetParser parser(result.GetResultSet(0));
            isSVOD = parser.TryNextRow();

            return result;
        },
        settings);
    return {std::move(status), isSVOD};
}

bool DropOldKinopoiskSVODTables(NYdb::NScheme::TSchemeClient& schemeClient, NYdb::NTable::TTableClient& tableClient,
                                const NYdbHelpers::TPath& database, TStringBuf prefix, const TString& latest) {
    return NVideoContent::DropOldTables(schemeClient, tableClient, database, prefix, latest,
                                        KINOPOISK_SVOD_TABLES_TO_KEEP);
}

// TKinopoiskSourceRequestFactoryWrapper ---------------------------------------
TKinopoiskSourceRequestFactoryWrapper::TKinopoiskSourceRequestFactoryWrapper(
    std::unique_ptr<ISourceRequestFactory> delegate, const TString& kinopoiskToken, const TString& serviceId,
    const TMaybe<TString>& authorization, const TMaybe<TString>& userAgent)
    : Delegate(std::move(delegate))
    , KinopoiskToken(kinopoiskToken)
    , ServiceId(serviceId)
    , Authorization(authorization)
    , UserAgent(userAgent) {
    Y_ENSURE(Delegate, "Delegate must be set!");
}

NHttpFetcher::TRequestPtr TKinopoiskSourceRequestFactoryWrapper::Request(TStringBuf path) {
    auto request = Delegate->Request(path);
    Y_ASSERT(request);
    FillRequest(*request);
    return request;
}

NHttpFetcher::TRequestPtr
TKinopoiskSourceRequestFactoryWrapper::AttachRequest(TStringBuf path, NHttpFetcher::IMultiRequest::TRef multiRequest) {
    auto request = Delegate->AttachRequest(path, multiRequest);
    Y_ASSERT(request);
    FillRequest(*request);
    return request;
}

void TKinopoiskSourceRequestFactoryWrapper::FillRequest(NHttpFetcher::TRequest& request) {
    request.AddCgiParam(TStringBuf("serviceId"), ServiceId);

    if (Authorization.Defined())
        request.AddHeader(TStringBuf("Authorization"), *Authorization);
    if (UserAgent.Defined())
        request.AddHeader(TStringBuf("User-Agent"), *UserAgent);

    request.AddHeader(TStringBuf("X-Service-Token"), KinopoiskToken);
}

// TKinopoiskContentInfoProvider -----------------------------------------------
TKinopoiskContentInfoProvider::TKinopoiskContentInfoProvider(
    std::unique_ptr<TKinopoiskSourceRequestFactoryWrapper> source, bool enableShowingItemsComingSoon)
    : Source(std::move(source))
    , EnableShowingItemsComingSoon(enableShowingItemsComingSoon) {
}

std::unique_ptr<ISerialDescriptorHandle>
TKinopoiskContentInfoProvider::MakeSerialDescriptorRequest(TVideoItemConstScheme tvShowItem,
                                                           NHttpFetcher::IMultiRequest::TRef multiRequest) {
    Y_ASSERT(Source);
    return std::make_unique<TSerialDescriptorHandle>(*Source, tvShowItem, EnableShowingItemsComingSoon, multiRequest);
}

std::unique_ptr<ISeasonDescriptorHandle>
TKinopoiskContentInfoProvider::MakeSeasonDescriptorRequest(const TSerialDescriptor& /* serialDescr */,
                                                           const TSeasonDescriptor& seasonDescr,
                                                           NHttpFetcher::IMultiRequest::TRef /* multiRequest */) {
    // |seasonDescr| from |serialDescr| should contain all necessary
    // info.
    return std::make_unique<TMockContentRequestHandle<TSeasonDescriptor>>(seasonDescr);
}

std::unique_ptr<IVideoItemHandle>
TKinopoiskContentInfoProvider::MakeContentInfoRequestImpl(TLightVideoItemConstScheme item,
                                                          NHttpFetcher::IMultiRequest::TRef multiRequest) {
    Y_ASSERT(Source);
    return std::make_unique<TKinopoiskContentInfoHandler>(*Source, item, EnableShowingItemsComingSoon, multiRequest);
}

} // namespace NVideoCommon

template <>
void Out<NVideoCommon::TSVODError>(IOutputStream& os, const NVideoCommon::TSVODError& error) {
    os << error.Msg << Endl;
}
