#include <alice/cuttlefish/library/proto_converters/converter_handlers.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <util/generic/maybe.h>


using namespace NAlice::NCuttlefish::NProtoConverters;


namespace {

struct TFakeWrter {
    TMaybe<TString> Val = Nothing();

    void Value(const TString& value) {
        Val = value;
    }
};

}  // anonymous namespace


Y_UNIT_TEST_SUITE(AuthTokenHandler) {

void Check(TStringBuf src, TStringBuf token, NAliceProtocol::TUserInfo::ETokenType type) {
    NAliceProtocol::TUserInfo dst;
    TAuthTokenHandler::Parse(src, dst);
    UNIT_ASSERT_EQUAL(dst.GetAuthToken(), token);
    UNIT_ASSERT_EQUAL(dst.GetAuthTokenType(), type);
}

void CheckSerialization(const TMaybe<TString>& token, TMaybe<NAliceProtocol::TUserInfo::ETokenType> type, TMaybe<TString> expectedStr) {
    NAliceProtocol::TUserInfo msg;
    TFakeWrter writer;

    if (token.Defined())
        msg.SetAuthToken(*token);
    if (type.Defined())
        msg.SetAuthTokenType(*type);

    if (TAuthTokenHandler::IsSerializationNeeded(msg))
        TAuthTokenHandler::Serialize(writer, msg);

    UNIT_ASSERT_EQUAL(writer.Val, expectedStr);
}


Y_UNIT_TEST(EmptyValue)
{
    NAliceProtocol::TUserInfo dst;

    TAuthTokenHandler::Parse("", dst);
    UNIT_ASSERT(!dst.HasAuthToken());
    UNIT_ASSERT(!dst.HasAuthTokenType());
}

Y_UNIT_TEST(OAuthToken)
{
    Check("oauth1234567890",        "1234567890", NAliceProtocol::TUserInfo::OAUTH);
    Check("OAuth 1234567890",       "1234567890", NAliceProtocol::TUserInfo::OAUTH);
    Check("OAUTH\t1234567890\t",    "1234567890", NAliceProtocol::TUserInfo::OAUTH);
    Check("1234567890",             "1234567890", NAliceProtocol::TUserInfo::OAUTH);
}

Y_UNIT_TEST(YambAuthToken)
{
    Check("yambauth1234567890",     "1234567890", NAliceProtocol::TUserInfo::YAMB_AUTH);
    Check("YambAuth 1234567890",    "1234567890", NAliceProtocol::TUserInfo::YAMB_AUTH);
    Check("YAMBAUTH\t1234567890\t", "1234567890", NAliceProtocol::TUserInfo::YAMB_AUTH);
}

Y_UNIT_TEST(OAuthTeamToken)
{
    Check("oauthteam1234567890",        "1234567890", NAliceProtocol::TUserInfo::OAUTH_TEAM);
    Check("OAuthTeam 1234567890",       "1234567890", NAliceProtocol::TUserInfo::OAUTH_TEAM);
    Check("OAUTHTEAM\t1234567890\t",    "1234567890", NAliceProtocol::TUserInfo::OAUTH_TEAM);
}

Y_UNIT_TEST(Serializing)
{
    CheckSerialization(Nothing(), Nothing(), Nothing());
    CheckSerialization("1234567890", NAliceProtocol::TUserInfo::OAUTH, "OAuth 1234567890");
    CheckSerialization("1234567890", NAliceProtocol::TUserInfo::OAUTH_TEAM, "OAuthTeam 1234567890");
    CheckSerialization("1234567890", NAliceProtocol::TUserInfo::YAMB_AUTH, "YambAuth 1234567890");

    // creepy cases
    CheckSerialization(Nothing(), NAliceProtocol::TUserInfo::OAUTH, Nothing());
    CheckSerialization("1234567890", Nothing(), Nothing());
}

}
