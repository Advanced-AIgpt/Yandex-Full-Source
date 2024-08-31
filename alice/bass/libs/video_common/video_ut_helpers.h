#pragma once

#include <alice/bass/libs/video_common/content_db.h>
#include <alice/bass/libs/video_common/defs.h>
#include <alice/bass/libs/video_common/ivi_genres.h>

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/video_content/common.h>
#include <alice/bass/libs/ydb_helpers/path.h>
#include <alice/bass/libs/ydb_helpers/table.h>

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

namespace NTestingHelpers {

struct TIviGenresDelegate : public NVideoCommon::TIviGenres::TDelegate {
    // NVideoCommon::TIviGenres::TDelegate overrides:
    THolder<NHttpFetcher::TRequest> MakeRequest(TStringBuf /* path */) override {
        return {};
    }
};

NVideoCommon::TSeasonDescriptor MakeSeason(TStringBuf providerName, TStringBuf serialId, const TMaybe<TString>& id,
                                           ui32 seasonIndex, ui32 episodesCount);

struct TDbVideoDirectory final {
    TDbVideoDirectory(NYdb::NTable::TTableClient& tableClient, NYdb::NScheme::TSchemeClient& schemeClient,
                      NYdbHelpers::TTablePath directoryPath);
    ~TDbVideoDirectory();

    void CreateTables();
    void DropTables();
    void WriteItems(const TVector<NVideoCommon::TVideoItem>& items);
    bool WriteSerialDescriptor(const NVideoCommon::TSerialDescriptor& serialDescr, TStringBuf providerName);

    NYdb::NTable::TTableClient& TableClient;
    NYdb::NScheme::TSchemeClient& SchemeClient;
    NYdbHelpers::TTablePath DirectoryPath;
    NYdbHelpers::TScopedDirectory Directory;

    NVideoCommon::TVideoTablesPaths Tables;
    NVideoCommon::TIndexTablesPaths Indexes;
};

} // namespace NTestingHelpers
