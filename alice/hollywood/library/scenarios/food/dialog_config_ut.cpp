#include "dialog_config.h"
#include <alice/hollywood/library/scenarios/food/nlg/register.h>
#include <alice/nlg/library/nlg_renderer/nlg_renderer.h>
#include <alice/nlg/library/testing/testing_helpers.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NFood {

Y_UNIT_TEST_SUITE(DialogConfig) {

    Y_UNIT_TEST(FrameNames) {
        // TODO(samoylovboris) Compile grammar and search frames there.
        /*
        const TDialogConfig& config = GetDialogConfig();
        for (const auto& [groupName, frameNames] : config.FrameGroups) {
            for (const auto& frameName : frameNames) {
                UNIT_ASSERT(config.Frames.contains(frameName));
            }
        }
        */
    }

    Y_UNIT_TEST(GroupNames) {
        const TDialogConfig& config = GetDialogConfig();
        for (const auto& [responseName, responseConfig] : config.Responses) {
            for (const TString& groupName : responseConfig.ExpectedFrameGroups) {
                UNIT_ASSERT(config.FrameGroups.contains(groupName));
            }
        }
    }

    Y_UNIT_TEST(HasNlg) {
        const auto reg = &NAlice::NHollywood::NLibrary::NScenarios::NFood::NNlg::RegisterAll;
        const auto nlg = NNlg::NTesting::CreateTestingNlgRenderer(reg);
        const TDialogConfig& config = GetDialogConfig();
        for (const auto& [responseName, responseConfig] : config.Responses) {
            UNIT_ASSERT(responseConfig.Flags.HasFlags(RCF_SILENT) || nlg->HasPhrase("food", responseName, ELanguage::LANG_RUS));
        }
        for (const auto& [name, group] : config.FrameGroups) {
            UNIT_ASSERT(group.FallbackNlg.empty() || nlg->HasPhrase("food", group.FallbackNlg, ELanguage::LANG_RUS));
        }
    }

}

}  // namespace NAlice::NHollywood::NFood
