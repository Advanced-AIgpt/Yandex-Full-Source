#include "content_db_check_utils.h"

#include <alice/bass/libs/video_common/content_db.h>
#include <alice/bass/libs/video_content/common.h>
#include <alice/bass/libs/ydb_helpers/table.h>

#include <util/stream/file.h>

using namespace NVideoCommon;
using namespace NYdbHelpers;

namespace NVideoCommon {

namespace {

TString MakeDbErrorMessage(const NYdb::TStatus& status) {
    Y_ASSERT(!status.IsSuccess());
    return TStringBuilder{} << "Query failed with errors: " << status.GetIssues().ToString();
}

TString MakeDbErrorMessage(const TYdbContentDb::TStatus& status) {
    Y_ASSERT(!status.IsSuccess());
    if (status.YdbStatus.GetIssues())
        return MakeDbErrorMessage(status.YdbStatus);
    return TStringBuilder{} << "Database query status: "<< status.Status;
}

template <typename TParamSetter>
TResult RunCheckForCountDiff(const TString& query, TParamSetter&& paramSetter, TDbAmountCheckConstScheme check,
                             TDbChecker& dbChecker, TStringBuf tableName) {
    auto extractCount = [](size_t& result) {
        return [&result](NYdb::TResultSetParser& parser) {
            bool hasFirstRow = parser.TryNextRow();
            Y_ASSERT(hasFirstRow);
            Y_ASSERT(parser.ColumnsCount() == 1);
            result = parser.ColumnParser(0).GetUint64();
            Y_ASSERT(!parser.TryNextRow()); // Only a single row can be extracted by COUNT().
        };
    };

    // NOTE: YDB doesn't support specifying table names as query parameters. We're doing it by manual substitution.
    TString candidateQuery = query;
    TString referenceQuery = query;
    SubstGlobal(candidateQuery, "$tableName", Join(dbChecker.GetCandidateDir(), tableName));
    SubstGlobal(referenceQuery, "$tableName", Join(dbChecker.GetReferenceDir(), tableName));

    size_t candidateItemsCount = 0;
    if (NYdb::TStatus status = ExecuteSingleSelectDataQuery(dbChecker.GetTableClient(), candidateQuery, paramSetter,
                                                            extractCount(candidateItemsCount));
        !status.IsSuccess())
    {
        return MakeError("Candidate query for ", tableName, " failed with an error: ", MakeDbErrorMessage(status));
    }

    if (check->HasExpectedCount() && check->ExpectedCount() > candidateItemsCount) {
        return MakeError("Count mismatch for ", tableName, " table: expected at least ", check->ExpectedCount(),
                         " items, found ", candidateItemsCount);
    }

    if (!check->HasPercentageDiff())
        return Nothing();

    size_t referenceItemsCount = 0;
    if (NYdb::TStatus status = ExecuteSingleSelectDataQuery(dbChecker.GetTableClient(), referenceQuery, paramSetter,
                                                            extractCount(referenceItemsCount));
        !status.IsSuccess())
    {
        return MakeError("Reference query for ", tableName, " failed with an error: ", MakeDbErrorMessage(status));
    }

    double expectedCount = referenceItemsCount * (1. - check->PercentageDiff() / 100.);
    if (candidateItemsCount < expectedCount) {
        return MakeError("Percentage mismatch for ", tableName, " table: expected at least ", expectedCount, "/",
                         referenceItemsCount, " items, found ", candidateItemsCount);
    }

    return Nothing();
}

TResult CheckIndividualItem(TVideoItemConstScheme referenceItem, TVideoItemConstScheme candidateItem) {
    if (FromString<EItemType>(referenceItem->Type()) != FromString<EItemType>(candidateItem->Type()))
        return MakeError("Item type mismatch: expected ", referenceItem->Type(), ", got ", candidateItem->Type());

    if (referenceItem->HasSeasonsCount() && referenceItem->SeasonsCount() > candidateItem->SeasonsCount()) {
        return MakeError("Individual item ", candidateItem->ProviderItemId(), " has failed seasons count: expected ",
                         referenceItem->SeasonsCount(), " seasons, found ", candidateItem->SeasonsCount());
    }

    return Nothing();
}

TResult CheckSeasonDescriptor(TYdbContentDb& contentDb, TStringBuf providerName, TStringBuf serialId,
                              TSeasonDescriptorCheckConstScheme refSeason) {
    TSeasonDescriptor candidateSeason;
    TSeasonKey key{providerName, serialId, refSeason->ProviderNumber()};
    if (const auto status = contentDb.FindSeasonDescriptor(key, candidateSeason); !status.IsSuccess())
        return MakeError("Cannot find season descriptor for ", key, ": ", MakeDbErrorMessage(status));

    if (refSeason->HasEpisodesCount() && refSeason->EpisodesCount() > candidateSeason.EpisodesCount) {
        return MakeError("TV show ", serialId, " has failed episode count check for season #",
                         refSeason->ProviderNumber(), ": expected ", refSeason->EpisodesCount(), ", found ",
                         candidateSeason.EpisodesCount);
    }

    if (refSeason->HasSoon() && candidateSeason.Soon != refSeason->Soon()) {
        return MakeError("TV show ", serialId, " has failed 'soon' flag check for season #",
                         refSeason->ProviderNumber(), ": expected: ", refSeason->Soon(),
                         ", actual: ", candidateSeason.Soon);
    }

    // TODO: Check individual season items?
    return Nothing();
}

TResult CheckSerialDescriptor(TYdbContentDb& contentDb, TStringBuf providerName, TStringBuf serialId,
                              TSerialDescriptorCheckConstScheme refSerial, const TSerialDescriptor& candidateSerial) {
    if (refSerial->HasSeasonsCount() && refSerial->SeasonsCount() > candidateSerial.Seasons.size()) {
        return MakeError("Serial ", serialId, " has failed seasons count check: expected ", refSerial->SeasonsCount(),
                         " seasons, found ", candidateSerial.Seasons.size());
    }

    if (!refSerial->HasSeasons())
        return Nothing();

    for (const auto& refSeason : refSerial->Seasons()) {
        if (const auto error = CheckSeasonDescriptor(contentDb, providerName, serialId, refSeason))
            return error;
    }

    return Nothing();
}

} // namespace

TDbCheckList ParseCheckListFromJsonOrThrow(TStringBuf json) {
    NSc::TValue jsonVal = NSc::TValue::FromJsonThrow(json);
    TDbCheckList result(jsonVal);

    TStringBuilder errText;
    auto validateCb = [&errText](TStringBuf key, TStringBuf errMsg) { errText << key << ": " << errMsg << "; "; };
    if (!result->Validate("", false, validateCb))
        ythrow yexception() << "Failed to validate check list file. Errors:  " << errText;
    return result;
}

TDbCheckList LoadCheckListFromFileOrThrow(const TString &jsonPath) {
    TString fileContent = TFileInput{jsonPath}.ReadAll();
    return ParseCheckListFromJsonOrThrow(fileContent);
}

TResult TDbChecker::RunCheckForTotalCountDiff(TDbAmountCheckConstScheme check, TStringBuf tableName, bool isForItems) {
    TString selectCountQuery = "SELECT COUNT(*) FROM [$tableName]";
    if (isForItems)
        selectCountQuery += " WHERE IsVoid=false";
    auto dummyQueryBuilder = [](NYdb::NTable::TSession& session) { return session.GetParamsBuilder().Build(); };
    return RunCheckForCountDiff(selectCountQuery, dummyQueryBuilder, check, *this, tableName);
}

TResult TDbChecker::RunCheckForProviderCountDiff(TDbAmountCheckConstScheme check, TStringBuf tableName,
                                              TStringBuf providerName, bool isForItems) {
    TString selectQuery = R"(DECLARE $providerName AS String;
                             SELECT COUNT(*) FROM [$tableName] WHERE ProviderName=$providerName)";
    if (isForItems)
        selectQuery += " AND IsVoid=false";
    auto setProvider = [providerName](NYdb::NTable::TSession& session) {
        return session.GetParamsBuilder().AddParam("$providerName").String(TString{providerName}).Build().Build();
    };

    return RunCheckForCountDiff(selectQuery, setProvider, check, *this, tableName);
}

TResult TDbChecker::RunChecksForTotalCountDiff(TVideoItemDbAmountCheckConstScheme check) {
    if (check->HasItems()) {
        if (const auto error = RunCheckForTotalCountDiff(
                check->Items(), NVideoContent::TVideoItemsLatestTableTraits::NAME, true /* isForItems */)) {
            return MakeError("Check for total amount of items has failed. ", error->Msg);
        }
    }

    if (check->HasSerials()) {
        if (const auto error = RunCheckForTotalCountDiff(
                check->Serials(), NVideoContent::TVideoSerialsTableTraits::NAME, false /* isForItems */)) {
            return MakeError("Check for total amount of serials has failed. ", error->Msg);
        }
    }

    if (check->HasSeasons()) {
        if (const auto error = RunCheckForTotalCountDiff(
                check->Seasons(), NVideoContent::TVideoSeasonsTableTraits::NAME, false /* isForItems */)) {
            return MakeError("Check for total amount of seasons has failed. ", error->Msg);
        }
    }

    return Nothing();
}

TResult TDbChecker::RunChecksForProviderCountDiff(TVideoItemDbAmountCheckConstScheme check, TStringBuf providerName) {
    if (check->HasItems()) {
        if (const auto error =
                RunCheckForProviderCountDiff(check->Items(), NVideoContent::TVideoItemsLatestTableTraits::NAME,
                                             providerName, true /* isForItems */)) {
            return MakeError("Check for total amount of items for provider ", providerName, " has failed. ",
                             error->Msg);
        }
    }

    if (check->HasSerials()) {
        if (const auto error =
                RunCheckForProviderCountDiff(check->Serials(), NVideoContent::TVideoSerialsTableTraits::NAME,
                                             providerName, false /* isForItems */)) {
            return MakeError("Check for total amount of serials for provider ", providerName, " has failed. ",
                             error->Msg);
        }
    }

    if (check->HasSeasons()) {
        if (const auto error =
                RunCheckForProviderCountDiff(check->Seasons(), NVideoContent::TVideoSeasonsTableTraits::NAME,
                                             providerName, false /* isForItems */)) {
            return MakeError("Check for total amount of seasons for provider ", providerName, " has failed. ",
                             error->Msg);
        }
    }

    return Nothing();
}

TResult TDbChecker::RunChecksForIndividualItem(TSingleItemCheckConstScheme check) {
    TStringBuf providerName = check->Item()->ProviderName();
    TStringBuf serialId = check->Item()->ProviderItemId();
    TTablePath candidatePath{Database, CandidateDir};
    TYdbContentDb contentDb{TableClient, TVideoTablesPaths::MakeDefault(candidatePath),
                            TIndexTablesPaths::MakeDefault(candidatePath)};
    const auto key = TVideoKey::TryFromProviderItemId(providerName, check->Item());
    if (!key)
        return MakeError("Check for individual item: ", *check->GetRawValue(), " has failed: cannot build key");

    TVideoItem foundItem;
    if (const auto status = contentDb.FindVideoItem(*key, foundItem); !status.IsSuccess())
        return MakeError("Cannot find video item ", key, ": ", MakeDbErrorMessage(status));

    if (const auto error = CheckIndividualItem(check->Item(), foundItem.Scheme()))
        return error;

    if (!check->HasSerialDescriptor())
        return Nothing();

    TSerialDescriptor foundSerial;
    if (const auto status = contentDb.FindSerialDescriptor(TSerialKey{providerName, serialId}, foundSerial);
        !status.IsSuccess())
    {
        return MakeError(MakeDbErrorMessage(status));
    }

    if (const auto error =
            CheckSerialDescriptor(contentDb, providerName, serialId, check->SerialDescriptor(), foundSerial))
        return error;

    return Nothing();
}

TResult TDbChecker::CheckContentDb(const TDbCheckList& checks) {
    if (checks->HasTotalAmounts()) {
        if (const auto error = RunChecksForTotalCountDiff(checks->TotalAmounts()))
            return error;
    }

    if (checks->HasProviderAmounts()) {
        for (const auto& columnIt : checks->ProviderAmounts()) {
            if (const auto error = RunChecksForProviderCountDiff(columnIt.Value(), columnIt.Key()))
                return error;
        }
    }

    if (checks->HasSingleItems()) {
        for (const auto& check : checks->SingleItems()) {
            if (const auto error = RunChecksForIndividualItem(check))
                return error;
        }
    }

    return Nothing();
}

} // namespace NVideoCommon
