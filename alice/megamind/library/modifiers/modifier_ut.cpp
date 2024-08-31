#include "modifier.h"

#include <alice/megamind/library/testing/fake_modifier.h>
#include <alice/megamind/library/testing/modifier_fixture.h>

namespace {

using namespace NAlice::NMegamind;

// Common data -----------------------------------------------------------------
constexpr auto SKR_NO_EXPERIMENTS = TStringBuf(R"(
{
    "application": { "timestamp": "1337" },
    "request": {
        "experiments": {},
        "voice_session": true,
        "event": {
            "type": "text_input"
        }
    }
}
)");

// Fake modifier data ----------------------------------------------------------
constexpr auto SKR_FAKE_MODIFIER = TStringBuf(R"(
{
    "application": { "timestamp": "1337" },
    "request": {
        "experiments": {
            "debug_response_modifiers": "1"
        },
        "voice_session": true,
        "event": {
            "type": "text_input"
        }
    }
}
)");

constexpr auto EXPECTED_FAKE_MODIFIER_MM = TStringBuf(R"(
{
    "response": {
        "cards": [{
            "type": "simple_text",
            "text": "Таков путь."
        }, {
            "type": "simple_text",
            "text": "Я всё сказала."
        }]
    },
    "voice_response": {
        "output_speech": {
            "type": "simple",
            "text": "Таков путь. Я всё сказала."
        },
        "should_listen": "false"
    }
}
)");

using TInputBuilder = TModifierTestFixture::TInputBuilder;

Y_UNIT_TEST_SUITE(Modifiers) {
    Y_UNIT_TEST_F(Fake, TModifierTestFixture) {
        TestExpectedResponse(
            CreateFakeModifier,
            TInputBuilder()
                .SetSkrJson(SKR_FAKE_MODIFIER)
                .Build(),
            EXPECTED_FAKE_MODIFIER_MM
        );
    }

    Y_UNIT_TEST_F(FakeNoFlag, TModifierTestFixture) {
        TestExpectedNonApply(
            CreateFakeModifier,
            TInputBuilder()
                .SetSkrJson(SKR_NO_EXPERIMENTS)
                .Build(),
            TNonApply::EType::DisabledByFlag
        );
    }
}

} // namespace
