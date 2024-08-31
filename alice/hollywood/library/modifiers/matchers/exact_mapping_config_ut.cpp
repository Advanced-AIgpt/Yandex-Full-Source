#include "exact_matcher.h"

#include <alice/hollywood/library/modifiers/internal/config/proto/exact_key_groups.pb.h>
#include <alice/hollywood/library/modifiers/internal/config/proto/exact_mapping_config.pb.h>
#include <alice/library/client/protos/promo_type.pb.h>

#include <alice/library/proto/proto.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/utf8.h>
#include <util/generic/string.h>
#include <util/stream/file.h>

namespace {

constexpr TStringBuf MAPPING_FILE_NAME = "mapping.pb.txt";

constexpr TStringBuf KEY_GROUP_FILE_NAME = "key_groups.pb.txt";

using namespace testing;
using namespace NAlice;
using namespace NAlice::NHollywood;
using namespace NAlice::NHollywood::NModifiers;

class TFixture : public NUnitTest::TBaseFixture {
public:
    TFixture() {
        ConfigPathList.push_back("alice/hollywood/library/modifiers/modifiers/colored_speaker_modifier/config");
        ConfigPathList.push_back("alice/hollywood/library/modifiers/modifiers/voice_doodle_modifier/config");
    }

    const TVector<TString> GetConfigPathList() {
        return ConfigPathList;
    }

private:
    TVector<TString> ConfigPathList;
};

std::pair<TExactMappingConfig, TExactKeyGroups> GetConfigs(const TString& configDir) {
    const auto path = TFsPath(ArcadiaSourceRoot()) / configDir;
    const auto mapping = ParseProtoText<TExactMappingConfig>(TFileInput{path / MAPPING_FILE_NAME}.ReadAll());
    const auto groups = ParseProtoText<TExactKeyGroups>(TFileInput{path / KEY_GROUP_FILE_NAME}.ReadAll());
    return std::make_pair(mapping, groups);
}

void TestPhraseGroupNamesDistinct(const TExactKeyGroups& groupsConfig) {
    THashSet<TString> groupNames;
    for (const auto& group : groupsConfig.GetGroups()) {
        UNIT_ASSERT_C(!groupNames.contains(group.GetGroupName()),
                      "All group names should be distinct got " << group.GetGroupName() << " at least twice");
        groupNames.insert(group.GetGroupName());
    }
}

void TestConfigKeysDistinct(const TExactMappingConfig& config, const TExactKeyGroups& groupsConfig) {
    THashMap<TString, TVector<TString>> groups;
    for (const auto& group : groupsConfig.GetGroups()) {
        groups[group.GetGroupName()] = TVector<TString>(group.GetPhrases().begin(), group.GetPhrases().end());
    }

    THashSet<TExactMatcher::TExactKey> keys;
    for (const auto& rule : config.GetMappings()) {
        auto* group = groups.FindPtr(rule.GetOldTtsGroupName());
        UNIT_ASSERT_C(group, "Not found group name: " << rule.GetOldTtsGroupName());
        for (const auto& phrase : *group) {
            const auto key = TExactMatcher::TExactKey{rule.GetDeviceColor(), rule.GetProductScenarioName(), phrase};
            UNIT_ASSERT_C(!keys.contains(key), "All keys should be distinct got <"
                                                   << NClient::EPromoType_Name(get<0>(key)) << ", " << get<1>(key)
                                                   << ", " << get<2>(key) << "> at least twice");
            keys.insert(key);
        }
    }
}

void TestConfigValuesNotEmpty(const TExactMappingConfig& config) {
    for (const auto& rule : config.GetMappings()) {
        UNIT_ASSERT_C(!rule.GetNewTtsTextList().empty(), "NewTextList should not be empty");
    }
}

void TestPhrasesInGroupsDistinct(const TExactKeyGroups& groupsConfig) {
    THashSet<TString> phrases;
    for (const auto& group : groupsConfig.GetGroups()) {
        for (auto phrase : group.GetPhrases()) {
            phrase = ToLowerUTF8(phrase);
            UNIT_ASSERT_C(!phrases.contains(phrase),
                          "All phrases should be distinct got" << phrase << " at least twice");
            phrases.insert(phrase);
        }
    }
}

Y_UNIT_TEST_SUITE_F(ColoredSpeakerModifier, TFixture) {
    Y_UNIT_TEST(TestPhraseGroupNamesDistinct) {
        for (const auto& path : GetConfigPathList()) {
            TestPhraseGroupNamesDistinct(GetConfigs(path).second);
        }
    }

    Y_UNIT_TEST(TestPhrasesInGroupsDistinct) {
        for (const auto& path : GetConfigPathList()) {
            TestPhrasesInGroupsDistinct(GetConfigs(path).second);
        }
    }

    Y_UNIT_TEST(TestConfigKeysDistinct) {
        for (const auto& path : GetConfigPathList()) {
            const auto& configPath = GetConfigs(path);
            TestConfigKeysDistinct(configPath.first, configPath.second);
        }
    }

    Y_UNIT_TEST(TestConfigValuesNotEmpty) {
        for (const auto& path : GetConfigPathList()) {
            TestConfigValuesNotEmpty(GetConfigs(path).first);
        }
    }
}

} // namespace
