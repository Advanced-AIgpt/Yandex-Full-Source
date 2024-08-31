#include "directives.h"

#include <alice/bass/libs/analytics/analytics.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/system/yassert.h>

using namespace NBASS;

namespace {

inline TString GetDirectivePath() {
    return "arcadia_source_path/alice/bass/data/directives.json";
}

inline bool IsBlank(const TString& str) {
    return str.find_first_not_of(" \t\r\n") == str.npos;
}

Y_UNIT_TEST_SUITE(Directives) {
    Y_UNIT_TEST(CheckOnFillInDescription) {
        const auto descriptionDirectives = NResource::Find("directives");

        NJson::TJsonValue directivesJson;
        NJson::ReadJsonTree(descriptionDirectives, &directivesJson, /* throwOnError = */true);

        TSet<TString> tags;
        for (const auto& directiveJson : directivesJson.GetArray()) {
            auto tag = directiveJson["analytics_tag"].GetString();
            auto type = directiveJson["type"].GetString();
            auto directive = directiveJson["directive"].GetString();
            auto author = directiveJson["author"].GetString();
            auto ticket = directiveJson["ticket"].GetString();
            auto description = directiveJson["human_description"].GetString();

            UNIT_ASSERT_C(!IsBlank(tag), "'analytics_tag' is blank");
            UNIT_ASSERT_C(!IsBlank(type), TStringBuilder() << "'type' is blank for '" << tag << "'");
            UNIT_ASSERT_C(!IsBlank(directive), TStringBuilder() << "'directive' is blank for '" << tag << "'");
            UNIT_ASSERT_C(!IsBlank(author), TStringBuilder() << "'author' is blank for '" << tag << "'");
            UNIT_ASSERT_C(!IsBlank(ticket), TStringBuilder() << "'ticket' is blank for '" << tag << "'");
            UNIT_ASSERT_C(!IsBlank(description), TStringBuilder() << "'human_description' is blank for '" << tag << "'");

            tags.insert(tag);
        }
    }

    Y_UNIT_TEST(CheckOnHavingDescriptionInDirectives) {
        const auto descriptionDirectives = NResource::Find("directives");

        NJson::TJsonValue directivesJson;
        NJson::ReadJsonTree(descriptionDirectives, &directivesJson, /* throwOnError = */true);

        TSet<TString> tags;
        for (const auto& directiveJson : directivesJson.GetArray()) {
            if (auto tag = directiveJson["analytics_tag"].GetString()) {
                tags.insert(tag);
            }
        }

        const auto factory = TDirectiveFactory::Get();
        for (const auto& [k, v] : *factory) {
            UNIT_ASSERT_C(tags.contains(v), TStringBuilder() << "Not found description for '"
                                                             << v << "' in file " << GetDirectivePath());
        }
    }
}

} // namespace
