#include "test_users_details.h"

#include <alice/bass/libs/ydb_helpers/table.h>
#include <alice/bass/ut/protos/protos.pb.h>
#include <alice/bass/ut/helpers.h>

#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>

namespace {

using namespace NBASS::NTestUsersDetails;
using namespace NYdbHelpers;

struct TUser {
    TStringBuf Login;
    TMaybe<ui64> ReleaseAt;
    TVector<TStringBuf> Tags;
};

class TTestItem {
public:
    TTestItem(TStringBuf descr, TUserManager::TResult result)
        : Descr{descr}
        , Result{result}
    {
    }
    virtual ~TTestItem() = default;

    virtual bool Validate(TUserManager& um) const = 0;

    TStringBuf Descr;
    TUserManager::TResult Result;
};

class TGetUserTestItem : public TTestItem {
public:
    TGetUserTestItem(TStringBuf descr, std::initializer_list<TStringBuf> tags, ui64 now, ui64 timeout, TUserManager::TResult result)
        : TTestItem{descr, std::move(result)}
        , Now{now}
        , Timeout{timeout}
    {
        Tags.SetArray();
        std::transform(tags.begin(), tags.end(),
                       std::back_inserter(Tags.GetArrayMutable()),
                       [](TStringBuf value) { return NSc::TValue{value}; }
        );
    }

    bool Validate(TUserManager& um) const override {
        return um.GetUser(Tags, Now, Now + Timeout) == Result;
    }

private:
    ui64 Now;
    ui64 Timeout;
    NSc::TValue Tags;
};

class TReleaseUserTestItem : public TTestItem {
public:
    TReleaseUserTestItem(TStringBuf descr, TStringBuf login, TUserManager::TResult&& result)
        : TTestItem{descr, std::move(result)}
        , Login{login}
    {
    }

    bool Validate(TUserManager& um) const override {
        return um.ReleaseUser(Login) == Result;
    }

private:
    TStringBuf Login;
};

class TTestUsersDb {
public:
    TTestUsersDb(NYdb::NTable::TTableClient& client, const TTablePath& testUsers, const TTablePath& testUsersTags)
        : TestUsersWriter(client, testUsers)
        , TestUsersTagsWriter(client, testUsersTags)
    {
    }

    NYdb::TStatus Add(TStringBuf login, TMaybe<ui64> releaseAt, const TVector<TStringBuf>& tags) {
        NBassUtProtos::TTestUsers user;
        user.Setlogin(login.data(), login.size());
        if (releaseAt) {
            user.set_release_at(*releaseAt);
        }
        user.Setclient_ip("127.0.0.1");
        user.Setpassword("test.password");
        user.Settoken("123");
        user.Setuuid("456");
        TestUsersWriter.AddRow(user);
        auto status = TestUsersWriter.Flush();
        if (!status.IsSuccess()) {
            return status;
        }

        for (TStringBuf tag : tags) {
            NBassUtProtos::TTestUsersTags tut;
            tut.Setlogin(login.data(), login.size());
            tut.Settag(tag.data(), tag.size());
            TestUsersTagsWriter.AddRow(tut);
        }
        return TestUsersTagsWriter.Flush();
    }

private:
    TTableWriter<NBassUtProtos::TTestUsers> TestUsersWriter;
    TTableWriter<NBassUtProtos::TTestUsersTags> TestUsersTagsWriter;
};

class TLocalFixture : public NTestingHelpers::TBassContextFixture {
public:
    TLocalFixture()
        : TestUsersPath{TPath{LocalYdbConfig().DataBase()}, "test_users"}
        , TestUsersTagsPath{TPath{LocalYdbConfig().DataBase()}, "test_users_tags"}
        , YdbClient{LocalYdb()}
    {
        CreateTableOrFail<NBassUtProtos::TTestUsers>(YdbClient, TestUsersPath, TVector<TString>({ "login" }));
        CreateTableOrFail<NBassUtProtos::TTestUsersTags>(YdbClient, TestUsersTagsPath, TVector<TString>({ "login", "tag" }));
    }

    TUserManager CreateUserManager() {
        return TUserManager{LocalYdbConfig()};
    }

public:
    const TTablePath TestUsersPath;
    const TTablePath TestUsersTagsPath;
    NYdb::NTable::TTableClient YdbClient;
};

Y_UNIT_TEST_SUITE_F(TestUsers, TLocalFixture) {
    Y_UNIT_TEST(UserManagerGetAndReleaseUser) {
        {
            TTestUsersDb tuDb{YdbClient, TestUsersPath, TestUsersTagsPath};
            static const TUser usersToAdd[] = {
                {
                    "vi002", Nothing(), { "tag1" },
                },
                {
                    "osado", Nothing(), { "tag1", "tag2" },
                },
            };
            for (const TUser& user : usersToAdd) {
                const auto status{tuDb.Add(user.Login, user.ReleaseAt, user.Tags)};
                UNIT_ASSERT_C(status.IsSuccess(), TStringBuilder() << "unable to add user: " << user.Login << ", " << status.GetIssues().ToString());
            }
        }

        const ui64 now = 1000;
        const THolder<TTestItem> items[] = {
            THolder(new TGetUserTestItem(
                "two tags requested one is existed and one is not",
                { "not_extisted_tag", "tag1" },
                now,
                1, // timeout
                TUserManager::TErrorResult{HTTP_OK, "no user"}
            )),
            THolder(new TGetUserTestItem(
                "not existed tag requested",
                { "not_extisted_tag" },
                now,
                1, // timeout
                TUserManager::TErrorResult{HTTP_OK, "no user"}
            )),
            THolder(new TGetUserTestItem(
                "no tags requested",
                {},
                now,
                4, // timeout
                TUserManager::TSuccessResult::FromJson(R"({"client_ip":"127.0.0.1","lock":1004,"login":"osado","tags":["tag1","tag2"],"token":"123","uuid":"456"})")
            )),
            THolder(new TGetUserTestItem(
                "existed tag 'tag1' requested",
                { "tag1" },
                now,
                3, // timeout
                NSc::TValue::FromJson(R"({"client_ip":"127.0.0.1","lock":1003,"login":"vi002","tags":["tag1"],"token":"123","uuid":"456"})")
            )),
            THolder(new TGetUserTestItem(
                "existed tag 'tag1' requested but no free users available",
                { "tag1" },
                now,
                1, // timeout
                TUserManager::TErrorResult{HTTP_OK, "no user"}
            )),
            THolder(new TGetUserTestItem(
                "existed tag 'tag1' requested and move now further when one user is available",
                { "tag1" },
                now + 4,
                3, // timeout
                TUserManager::TSuccessResult::FromJson(R"({"client_ip":"127.0.0.1","lock":1007,"login":"vi002","tags":["tag1"],"token":"123","uuid":"456"})")
            )),
            THolder(new TGetUserTestItem(
                "two existed tags requested",
                { "tag2", "tag1" },
                now + 10,
                1, // timeout
                TUserManager::TSuccessResult::FromJson(R"({"client_ip":"127.0.0.1","lock":1011,"login":"osado","tags":["tag1","tag2"],"token":"123","uuid":"456"})")
            )),
            THolder(new TGetUserTestItem(
                "second try two existed tags requested",
                { "tag2", "tag1" },
                now + 10,
                1, // timeout
                TUserManager::TErrorResult{HTTP_OK, "no user"}
            )),
            THolder(new TReleaseUserTestItem(
                "free osado",
                "osado",
                TUserManager::TSuccessResult()
            )),
            THolder(new TGetUserTestItem(
                "two existed tags requested after free",
                { "tag2", "tag1" },
                now + 10,
                1, // timeout
                TUserManager::TSuccessResult::FromJson(R"({"client_ip":"127.0.0.1","lock":1011,"login":"osado","tags":["tag1","tag2"],"token":"123","uuid":"456"})")
            ))
        };

        TUserManager um = CreateUserManager();
        for (const auto& item : items) {
            UNIT_ASSERT_C(item->Validate(um), item->Descr);
        }
    }
}

} // namespace
