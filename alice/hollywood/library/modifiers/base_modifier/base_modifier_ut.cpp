#include "base_modifier.h"

#include <alice/hollywood/library/modifiers/testing/fake_modifier.h>
#include <alice/hollywood/library/modifiers/testing/mock_modifier_context.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>

namespace {

using namespace testing;
using namespace NAlice;
using namespace NAlice::NHollywood;
using namespace NAlice::NHollywood::NModifiers;

inline constexpr TStringBuf ENABLE_KEK_MODIFIER = "mm_enable_modifier=kek-modifier";
inline constexpr TStringBuf DISABLE_KEK_MODIFIER = "mm_disable_modifier=kek-modifier";

Y_UNIT_TEST_SUITE(BaseModifier) {
    Y_UNIT_TEST(TestDefaultDisabled) {
        TFakeModifier modifier{"kek-modifier"};
        TMockModifierContext ctx;
        ON_CALL(ctx, HasExpFlag(ENABLE_KEK_MODIFIER)).WillByDefault(Return(false));
        ON_CALL(ctx, HasExpFlag(DISABLE_KEK_MODIFIER)).WillByDefault(Return(false));
        UNIT_ASSERT(!modifier.IsEnabled(ctx));
    }

    Y_UNIT_TEST(TestDefaultEnabled) {
        TFakeModifier modifier{"kek-modifier"};
        TMockModifierContext ctx;
        ON_CALL(ctx, HasExpFlag(ENABLE_KEK_MODIFIER)).WillByDefault(Return(false));
        ON_CALL(ctx, HasExpFlag(DISABLE_KEK_MODIFIER)).WillByDefault(Return(false));
        modifier.SetEnabled(true);
        UNIT_ASSERT(modifier.IsEnabled(ctx));
    }

    Y_UNIT_TEST(TestForceEnableByFlag) {
        TFakeModifier modifier{"kek-modifier"};
        TMockModifierContext ctx;
        ON_CALL(ctx, HasExpFlag(ENABLE_KEK_MODIFIER)).WillByDefault(Return(true));
        ON_CALL(ctx, HasExpFlag(DISABLE_KEK_MODIFIER)).WillByDefault(Return(false));
        modifier.SetEnabled(false);
        UNIT_ASSERT(modifier.IsEnabled(ctx));
    }

    Y_UNIT_TEST(TestForceDisableByFlag) {
        TFakeModifier modifier{"kek-modifier"};
        TMockModifierContext ctx;
        ON_CALL(ctx, HasExpFlag(ENABLE_KEK_MODIFIER)).WillByDefault(Return(false));
        ON_CALL(ctx, HasExpFlag(DISABLE_KEK_MODIFIER)).WillByDefault(Return(true));
        modifier.SetEnabled(true);
        UNIT_ASSERT(!modifier.IsEnabled(ctx));
    }

    Y_UNIT_TEST(TestForceDisableByFlagWithForceEnabled) {
        TFakeModifier modifier{"kek-modifier"};
        TMockModifierContext ctx;
        ON_CALL(ctx, HasExpFlag(ENABLE_KEK_MODIFIER)).WillByDefault(Return(true));
        ON_CALL(ctx, HasExpFlag(DISABLE_KEK_MODIFIER)).WillByDefault(Return(true));
        modifier.SetEnabled(true);
        UNIT_ASSERT(!modifier.IsEnabled(ctx));
    }
}

} // namespace
