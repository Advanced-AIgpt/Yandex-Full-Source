#include "config.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/string/cast.h>

namespace {

static TString PYTHON_UNIPROXY_CONFIG = "/python_uniproxy_config.json";

static TString CUTTLEFISH_BETA_CONFIG = "/cuttlefish_beta_config.json";
static TString CUTTLEFISH_PROD_CONFIG = "/cuttlefish_prod_config.json";
static TString CUTTLEFISH_SPLIT_BETA_CONFIG = "/cuttlefish_split_beta_config.json";
static TString CUTTLEFISH_SPLIT_MAIN_CONFIG = "/cuttlefish_split_main_config.json";


const TVector<std::pair<TString, NAliceCuttlefishConfig::TConfig>>& GetCuttlefishConfigs() {
    static const TVector<std::pair<TString, NAliceCuttlefishConfig::TConfig>> cuttlefishConfigs = {
        {"default", NAlice::NCuttlefish::GetDefaultCuttlefishConfig()},
        {"beta", NAlice::NCuttlefish::GetCuttlefishConfigFromResource(CUTTLEFISH_BETA_CONFIG)},
        {"prod", NAlice::NCuttlefish::GetCuttlefishConfigFromResource(CUTTLEFISH_PROD_CONFIG)},
        {"split_beta", NAlice::NCuttlefish::GetCuttlefishConfigFromResource(CUTTLEFISH_SPLIT_BETA_CONFIG)},
        {"split_main", NAlice::NCuttlefish::GetCuttlefishConfigFromResource(CUTTLEFISH_SPLIT_MAIN_CONFIG)},
    };

    return cuttlefishConfigs;
}

NJson::TJsonValue ReadJson(TStringBuf raw) {
    NJson::TJsonValue val;
    NJson::ReadJsonTree(raw, &val, true);
    return val;
}

TVector<TString> ArrayToStringVector(const NJson::TJsonValue::TArray& arr) {
    TVector<TString> res;
    for (const NJson::TJsonValue& it : arr) {
        res.push_back(it.GetString());
    }
    return res;
}

THashMap<TString, TString> MapToStringMap(const NJson::TJsonValue::TMapType& map) {
    THashMap<TString, TString> res;
    for (const auto& it : map) {
        res[it.first] = it.second.GetString();
    }
    return res;
}

} // namespace

Y_UNIT_TEST_SUITE(CuttlefishConfig) {

Y_UNIT_TEST(ValidateConfigsWithPythonUniproxyConfig) {
    NJson::TJsonValue pythonUniproxyConfig = ReadJson(NResource::Find(PYTHON_UNIPROXY_CONFIG));

    TVector<TString> apiKeysWhitelist = ArrayToStringVector(pythonUniproxyConfig["apikeys"]["whitelist"].GetArray());
    TString apiKeysUrl = pythonUniproxyConfig["apikeys"]["url"].GetString();
    TString apiKeysMobileToken = pythonUniproxyConfig["apikeys"]["mobile_token"].GetString();
    TString apiKeysJsToken = pythonUniproxyConfig["apikeys"]["js_token"].GetString();

    TString messengerAnonymousGuid = pythonUniproxyConfig["messenger"]["anonymous_guid"].GetString();

    TString mdsDownloadUrl = pythonUniproxyConfig["mds"]["download"].GetString();
    TString mdsNamespace = pythonUniproxyConfig["mds"]["namespace"].GetString();
    TString mdsTtl = pythonUniproxyConfig["mds"]["ttl"].GetString();

    TString megamindDefaultRunSuffix = pythonUniproxyConfig["vins"]["default_run_suffix"].GetString();
    TString megamindDefaultApplySuffix = pythonUniproxyConfig["vins"]["default_apply_suffix"].GetString();
    double megamindTimeout = pythonUniproxyConfig["vins"]["timeout"].GetDouble();

    THashMap<TString, TString> appTypes = MapToStringMap(pythonUniproxyConfig["vins"]["app_types"].GetMap());

    for (const auto& [configType, config] : GetCuttlefishConfigs()) {
        {
            UNIT_ASSERT_EQUAL_C(config.api_keys().whitelist(), apiKeysWhitelist, "api keys whitelist is differ at " << configType << " config");
            UNIT_ASSERT_VALUES_EQUAL_C(config.api_keys().url(), apiKeysUrl, "at " << configType << " config");
            UNIT_ASSERT_VALUES_EQUAL_C(config.api_keys().mobile_token(), apiKeysMobileToken, "at " << configType << " config");
            UNIT_ASSERT_VALUES_EQUAL_C(config.api_keys().js_token(), apiKeysJsToken, "at " << configType << " config");
        }

        {
            UNIT_ASSERT_VALUES_EQUAL_C(config.messenger().anonymous_guid(), messengerAnonymousGuid, "at " << configType << " config");
        }

        {
            UNIT_ASSERT_VALUES_EQUAL_C(config.mds().download_url(), mdsDownloadUrl, "at " << configType << " config");
            UNIT_ASSERT_VALUES_EQUAL_C(config.mds().upload_namespace(), mdsNamespace, "at " << configType << " config");
            UNIT_ASSERT_VALUES_EQUAL_C(config.mds().ttl(), FromString<TDuration>(mdsTtl), "at " << configType << " config");
        }

        {
            UNIT_ASSERT_VALUES_EQUAL_C(config.megamind().default_run_suffix(), megamindDefaultRunSuffix, "at " << configType << " config");
            UNIT_ASSERT_VALUES_EQUAL_C(config.megamind().default_apply_suffix(), megamindDefaultApplySuffix, "at " << configType << " config");
            UNIT_ASSERT_DOUBLES_EQUAL_C(config.megamind().timeout().SecondsFloat(), megamindTimeout, 1e-9, "at " << configType << " config");
        }

        {
            UNIT_ASSERT_EQUAL_C(config.app_types().mapper(), appTypes, "app_types is differ at " << configType << " config");
        }
    }
}

Y_UNIT_TEST(ValidateConfigsWithDefaultCuttlefishConfig) {
    // almost everything is validated in ValidateConfigsWithPythonUniproxyConfig, here are cuttlefish only parts
    auto defaultConfig = NAlice::NCuttlefish::GetDefaultCuttlefishConfig();

    TString defaultS3Bucket = defaultConfig.tts().s3_config().default_bucket();
    TVector<TString> allowedS3Buckets = defaultConfig.tts().s3_config().allowed_buckets();

    for (const auto& [configType, config] : GetCuttlefishConfigs()) {
        {
            UNIT_ASSERT_VALUES_EQUAL_C(config.tts().s3_config().default_bucket(), defaultS3Bucket, "at" << configType);
            UNIT_ASSERT_EQUAL_C(config.tts().s3_config().allowed_buckets(), allowedS3Buckets, "allowed_backets is differ at " << configType << " config");
        }
    }
}

}
