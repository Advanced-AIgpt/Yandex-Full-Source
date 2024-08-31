#include <alice/cuttlefish/library/convert/private/json_converts.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/json/json_reader.h>


using namespace NAlice::NCuttlefish::NConvert::NPrivate;



Y_UNIT_TEST_SUITE(JsonConverts) {

inline NJson::TJsonValue CreateJson(TStringBuf raw) {
    NJson::TJsonValue value;
    NJson::ReadJsonTree(raw, &value, /*throwOnError =*/ true);
    return value;

}

Y_UNIT_TEST(Basic) {
    const NJson::TJsonValue value("string value");

    UNIT_ASSERT_EQUAL(TJsonConverts<TString>::GetSafe(value), "string value");
}

}  // Y_UNIT_TEST_SUITE(ParseSynchronizeState)
