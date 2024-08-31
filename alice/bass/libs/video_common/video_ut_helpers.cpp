#include "video_ut_helpers.h"

#include <alice/bass/libs/video_content/common.h>
#include <alice/bass/libs/video_content/protos/rows.pb.h>

using namespace NVideoCommon;
using namespace NVideoContent;
using namespace NYdbHelpers;

namespace NTestingHelpers {

TSeasonDescriptor MakeSeason(TStringBuf providerName, TStringBuf serialId, const TMaybe<TString>& id, ui32 seasonIndex,
                             ui32 episodesCount) {
    TSeasonDescriptor season;

    season.SerialId = serialId;
    season.Id = id;
    season.EpisodesCount = episodesCount;
    season.Index = seasonIndex;
    season.ProviderNumber = seasonIndex + 1;

    for (ui32 episodeIndex = 0; episodeIndex < episodesCount; ++episodeIndex) {
        TVideoItem item;
        item->ProviderName() = providerName;
        item->Type() = ToString(EItemType::TvShowEpisode);
        item->ProviderItemId() = TStringBuilder{}
                                 << serialId << "_s" << (seasonIndex + 1) << "e" << (episodeIndex + 1);

        season.EpisodeItems.push_back(item);
        season.EpisodeIds.push_back(TString{item->ProviderItemId().Get()});
    }

    return season;
}

// TDbVideoDirectory
TDbVideoDirectory::TDbVideoDirectory(NYdb::NTable::TTableClient& tableClient,
                                     NYdb::NScheme::TSchemeClient& schemeClient, TTablePath directoryPath)
    : TableClient(tableClient)
    , SchemeClient(schemeClient)
    , DirectoryPath(std::move(directoryPath))
    , Directory(TScopedDirectory::Make(SchemeClient, DirectoryPath))
    , Tables(TVideoTablesPaths::MakeDefault(DirectoryPath))
    , Indexes(TIndexTablesPaths::MakeDefault(DirectoryPath)) {
    CreateTables();
}

TDbVideoDirectory::~TDbVideoDirectory() {
    DropTables();
}

void TDbVideoDirectory::CreateTables() {
    CreateTableOrFail<TVideoItemsLatestTableTraits>(TableClient, Tables.ItemsPath());
    CreateTableOrFail<TVideoSerialsTableTraits>(TableClient, Tables.SerialsPath());
    CreateTableOrFail<TVideoSeasonsTableTraits>(TableClient, Tables.SeasonsPath());
    CreateTableOrFail<TProviderUniqueItemsTableTraitsV2>(TableClient, Tables.ProviderUniqueItemsPath());
    CreateTableOrFail<TProviderItemIdIndexTableTraits>(TableClient, Indexes.PiidsPath());
    CreateTableOrFail<THumanReadableIdIndexTableTraits>(TableClient, Indexes.HridsPath());
    CreateTableOrFail<TKinopoiskIdIndexTableTraits>(TableClient, Indexes.KpidsPath());
}

void TDbVideoDirectory::DropTables() {
    auto dropTable = [this](const TTablePath& path) { DropTable(TableClient, path); };
    Tables.ForEachPath(dropTable);
    Indexes.ForEachPath(dropTable);
}

void TDbVideoDirectory::WriteItems(const TVector<TVideoItem>& items) {
    TTableWriter<TVideoItemsLatestTableTraits::TScheme> itemsWriter{TableClient, Tables.ItemsPath()};
    TTableWriter<TProviderUniqueItemsTableTraitsV2::TScheme> puiWriter{TableClient, Tables.ProviderUniqueItemsPath()};
    TTableWriter<NProtos::TProviderItemIdIndexRow> piidsWriter{TableClient, Indexes.PiidsPath()};
    TTableWriter<NProtos::THumanReadableIdIndexRow> hridsWriter{TableClient, Indexes.HridsPath()};
    TTableWriter<NProtos::TKinopoiskIdIndexRow> kpidsWriter{TableClient, Indexes.KpidsPath()};

    for (ui64 id = 0; id < items.size(); ++id) {
        const auto& item = items[id];
        {
            TVideoItemsLatestTableTraits::TScheme row;
            Ser(item, id, false /* isVoid */, row);
            itemsWriter.AddRow(row);
        }

        {
            NProtos::TProviderItemIdIndexRow row;
            if (Ser(item, id, row))
                piidsWriter.AddRow(row);
        }

        {
            NProtos::THumanReadableIdIndexRow row;
            if (Ser(item, id, row))
                hridsWriter.AddRow(row);
        }

        {
            NProtos::TKinopoiskIdIndexRow row;
            if (Ser(item, id, row))
                kpidsWriter.AddRow(row);
        }

        {
            if (DoesProviderHaveUniqueIdsForItems(item->ProviderName())) {
                TProviderUniqueItemsTableTraitsV2::TScheme row;
                if (Ser(item, row))
                    puiWriter.AddRow(row);
            }
        }
    }
}

bool TDbVideoDirectory::WriteSerialDescriptor(const TSerialDescriptor& serialDescr, TStringBuf providerName) {
    {
        TTableWriter<NVideoContent::NProtos::TSerialDescriptorRow> writer(TableClient, Tables.SerialsPath());
        NVideoContent::NProtos::TSerialDescriptorRow row;
        if (!serialDescr.Ser(TSerialKey{providerName, serialDescr.Id}, row))
            return false;
        writer.AddRow(row);
    }

    {
        TTableWriter<NVideoContent::NProtos::TSeasonDescriptorRow> writer(TableClient, Tables.SeasonsPath());
        for (const auto& season : serialDescr.Seasons) {
            NVideoContent::NProtos::TSeasonDescriptorRow row;
            if (!season.Ser(TSeasonKey{providerName, season.SerialId, season.ProviderNumber}, row))
                return false;
            writer.AddRow(row);
        }
    }

    {
        if (DoesProviderHaveUniqueIdsForItems(providerName)) {
            TTableWriter<TProviderUniqueItemsTableTraitsV2::TScheme> writer(TableClient,
                                                                            Tables.ProviderUniqueItemsPath());
            for (const auto& season : serialDescr.Seasons) {
                for (const auto& episode : season.EpisodeItems) {
                    TProviderUniqueItemsTableTraitsV2::TScheme row;
                    if (!Ser(episode, row))
                        return false;
                    writer.AddRow(row);
                }
            }
        }
    }
    return true;
}

}
