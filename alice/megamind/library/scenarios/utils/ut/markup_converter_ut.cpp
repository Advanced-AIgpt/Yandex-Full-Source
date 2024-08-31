#include <alice/megamind/library/scenarios/utils/markup_converter.h>
#include <alice/library/unittest/message_diff.h>

#include <alice/library/json/json.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NScenarios;

namespace {

constexpr auto PROTOCOL_BEGEMOT_MARKUP = TStringBuf(R"(
{
    "Delimiters": [
        {},
        {
            "EndChar": 5,
            "BeginChar": 2,
            "Text": " + "
        },
        {}
    ],
    "ProcessedRequest": "10 + 10",
    "OriginalRequest": "10 + 10",
    "Morph": [
        {
            "Lemmas": [
                {
                    "Text": "00000000010"
                }
            ],
            "Tokens": {
                "End": 1,
                "Begin": 0
            }
        },
        {
            "Lemmas": [
                {
                    "Text": "00000000010"
                }
            ],
            "Tokens": {
                "End": 2,
                "Begin": 1
            }
        }
    ],
    "Tokens": [
        {
            "EndChar": 2,
            "BeginChar": 0,
            "Text": "10"
        },
        {
            "EndChar": 7,
            "BeginChar": 5,
            "Text": "10"
        }
    ],
    "DirtyLang": {
        "DirtyLangClass": "DIRTY"
    }
}
)");

constexpr auto BEGEMOT_MARKUP_RESPONSE = TStringBuf(R"(
{
    "Delimiters": [
        {},
        {
            "EndByte": 5,
            "BeginByte": 2,
            "EndChar": 5,
            "BeginChar": 2,
            "Text": " + "
        },
        {}
    ],
    "ProcessedRequest": "10 + 10",
    "OriginalRequest": "10 + 10",
    "Morph": [
        {
            "Lemmas": [
                {
                    "Text": "00000000010"
                }
            ],
            "Tokens": {
                "End": 1,
                "Begin": 0
            }
        },
        {
            "Lemmas": [
                {
                    "Text": "00000000010"
                }
            ],
            "Tokens": {
                "End": 2,
                "Begin": 1
            }
        }
    ],
    "Tokens": [
        {
            "EndByte": 2,
            "BeginByte": 0,
            "EndChar": 2,
            "BeginChar": 0,
            "Text": "10"
        },
        {
            "EndByte": 7,
            "BeginByte": 5,
            "EndChar": 7,
            "BeginChar": 5,
            "Text": "10"
        }
    ],
    "DirtyLang": {
        "Class": "DIRTY"
    }
}
)");

Y_UNIT_TEST_SUITE(MarkupConverter) {
    Y_UNIT_TEST(ConvertResponse) {
        const auto& response = JsonToProto<NBg::NProto::TExternalMarkupProto>(NJson::ReadJsonFastTree(BEGEMOT_MARKUP_RESPONSE));
        TBegemotExternalMarkup protocolMarkup;
        ConvertExternalMarkup(response, protocolMarkup);

        const auto& expected = JsonToProto<TBegemotExternalMarkup>(NJson::ReadJsonFastTree(PROTOCOL_BEGEMOT_MARKUP));
        UNIT_ASSERT_MESSAGES_EQUAL(expected, protocolMarkup);
    }
}

} // namespace
