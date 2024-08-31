#include "test_users_details.h"

#include "ydb.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/ydb_helpers/queries.h>

#include <util/generic/hash.h>
#include <util/generic/variant.h>

namespace NBASS {
namespace NTestUsersDetails {
namespace {

struct TColumn {
    TString Get(NYdb::TResultSetParser& userParser) const {
        TString value{userParser.ColumnParser(Name).GetOptionalString().GetOrElse(Default)};
        if (value.empty()) {
            value = Default;
        }
        return value;
    }

    const TString Name;
    const TString Default = {};
};

NYdb::TDriver ConstructYdbDriver(TConfig::TYdbScheme config) {
    auto validate = [](TStringBuf path, TStringBuf error) {
        LOG(ERR) << "YdbStruct: " << path << " : " << error << Endl;
    };
    Y_ENSURE(config.Validate("", false, validate));

    const NYdb::TDriverConfig ydbConfig{
        NYdb::TDriverConfig{}
            .SetEndpoint(TString{*config.Endpoint()})
            .SetDatabase(TString{*config.DataBase()})
            .SetAuthToken(TString{*config.Token()})
    };
    return NYdb::TDriver{ydbConfig};
}

} // namespace

// TUserManager::TErrorResult -------------------------------------------------
TUserManager::TErrorResult::TErrorResult(HttpCodes httpCode, TStringBuf msg)
    : HttpCode{httpCode}
{
    Json.SetString(msg);
}

TUserManager::TErrorResult::TErrorResult(HttpCodes httpCode, TStringBuf type, TStringBuf msg, TStringBuf request)
    : HttpCode{httpCode}
{
    Json["type"].SetString(type);
    Json["msg"].SetString(msg);
    Json["request"].SetString(request);
}

// TUserManager ---------------------------------------------------------------
TUserManager::TUserManager(TConfig::TYdbScheme config)
    : YDbDriver{ConstructYdbDriver(config)}
    , YDbClient{YDbDriver}
{
}

TUserManager::TResult TUserManager::GetUser(const NSc::TValue& tags, ui64 now, ui64 releaseAt) {
    static const TColumn columnLogin{"login"};
    static const TColumn columnToken{"token"};
    static const TColumn columnUuid{"uuid"};
    static const TColumn columnClientIp{"client_ip", "127.0.0.1"};

    TStringStream query;

    query << "$release_at_filter = " << NYdbHelpers::EmitValue(now) << ';' << Endl
          << "$release_after = " << NYdbHelpers::EmitValue(releaseAt) << ';' << Endl;

    if (const size_t totalTags = tags.ArraySize(); totalTags > 0) {
        query << "$tags = AsList(";
        NYdbHelpers::TSepVisitor tagsVisitor(query, ", ");
        for (const NSc::TValue& tag : tags.GetArray()) {
            tagsVisitor(tag.GetString());
        }
        query << ");";

        query << R"(
            $available_logins = (SELECT COUNT(tut.tag) AS tags_count, tu.login AS login
                FROM test_users_tags AS tut
                JOIN test_users AS tu USING(login)
                WHERE tut.tag IN $tags AND (tu.release_at IS NULL OR tu.release_at < $release_at_filter)
                GROUP BY tu.login ORDER BY tags_count DESC LIMIT 1);
            $suitable_login = (
                SELECT tu.login AS login, tu.token AS token, tu.uuid AS uuid, tu.client_ip AS client_ip
                    FROM test_users AS tu JOIN $available_logins AS sl ON tu.login = sl.login WHERE sl.tags_count = ListLength($tags)
                ORDER BY login
            );)";
    }
    else {
        query << R"(
            $suitable_login = (
                SELECT tu.login AS login, tu.token AS token, tu.uuid AS uuid, tu.client_ip AS client_ip
                    FROM test_users AS tu
                    WHERE tu.release_at IS NULL OR tu.release_at < $release_at_filter
                ORDER BY login
                LIMIT 1
            );)";
    }

    query << R"(
            UPDATE test_users ON SELECT login, $release_after AS release_at FROM $suitable_login;
            SELECT * FROM $suitable_login;
            SELECT tut.tag AS tag FROM test_users_tags AS tut JOIN $suitable_login AS tt USING (login);)";

    TMaybe<TResult> result;
    auto cb = [&result, releaseAt](NYdb::NTable::TDataQueryResult& res) {
        NYdb::TResultSetParser userParser = res.GetResultSetParser(0 /* user select */);
        if (!userParser.TryNextRow()) {
            result.ConstructInPlace(TErrorResult{HTTP_OK, "no user"});
            return;
        }

        const TString login{columnLogin.Get(userParser)};
        const TString token{columnToken.Get(userParser)};
        const TString uuid{columnUuid.Get(userParser)};
        const TString clientIp{columnClientIp.Get(userParser)};

        NSc::TValue resultJson;
        resultJson["client_ip"].SetString(clientIp);
        resultJson["lock"] = releaseAt;
        resultJson["login"].SetString(login);
        resultJson["token"].SetString(token);
        resultJson["uuid"].SetString(uuid);

        NSc::TValue& tagsJson = resultJson["tags"].SetArray();

        auto tagsParser = res.GetResultSetParser(1);
        while (tagsParser.TryNextRow()) {
            static const TColumn columnTag{"tag"};
            const TString tag = columnTag.Get(tagsParser);
            if (!tag.empty()) {
                tagsJson.Push().SetString(tag);
            }
        }

        result.ConstructInPlace(std::move(resultJson));
    };
    NYdb::TStatus res = NYdbHelpers::ExecuteSelectQuery(YDbClient, query.Str(), cb, NYdbHelpers::TQueryOptions{});
    if (!res.IsSuccess()) {
        return TErrorResult{HTTP_BAD_REQUEST, "ydb_request", res.GetIssues().ToString(), query.Str()};
    }

    return result ? *result : TErrorResult{HTTP_BAD_REQUEST, "unknown error"};
}

TUserManager::TResult TUserManager::ReleaseUser(TStringBuf login) {
    const TString query = TStringBuilder{} << "UPDATE test_users SET release_at = NULL WHERE login = " << NYdbHelpers::EmitValue(login) << ';';
    const auto res = NYdbHelpers::ExecuteSelectQuery(
        YDbClient, query, [](NYdb::NTable::TDataQueryResult&) {}, NYdbHelpers::TQueryOptions{});

    if (!res.IsSuccess()) {
        return TResult{TErrorResult{HTTP_BAD_REQUEST, "ydb_request", res.GetIssues().ToString(), query }};
    }

    return TResult{NSc::Null()};
}

} // namespace NTestUsersDetails
} // namespace NBASS
