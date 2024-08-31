#include "voice_doodle_modifier.h"

#include <alice/hollywood/library/modifiers/internal/config/proto/exact_key_groups.pb.h>
#include <alice/hollywood/library/modifiers/internal/config/proto/exact_mapping_config.pb.h>
#include <alice/library/client/protos/promo_type.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/proto/proto.h>

#include <util/stream/file.h>

namespace NAlice::NHollywood::NModifiers {

namespace {

inline const TString MODIFIER_TYPE = "voice_doodle";

constexpr TStringBuf CONFIG_FILE_NAME = "mapping.pb.txt";
constexpr TStringBuf PHRASE_GROUPS_FILE_NAME = "key_groups.pb.txt";

} // namespace

TVoiceDoodleModifier::TVoiceDoodleModifier()
    : TBaseModifier(MODIFIER_TYPE)
{
}

void TVoiceDoodleModifier::LoadResourcesFromPath(const TFsPath& modifierResourcesBasePath) {
    TFileInput configInput(modifierResourcesBasePath / CONFIG_FILE_NAME);
    TFileInput phrasesInput(modifierResourcesBasePath / PHRASE_GROUPS_FILE_NAME);
    Matcher_ = std::make_unique<TExactMatcher>(ParseProtoText<TExactMappingConfig>(configInput.ReadAll()),
                                               ParseProtoText<TExactKeyGroups>(phrasesInput.ReadAll()));
}

TApplyResult TVoiceDoodleModifier::TryApply(TModifierApplyContext applyCtx) const {
    Y_ENSURE(Matcher_, "Matcher is not initialized");
    if (auto* matchingsPtr =
            Matcher_->FindPtr(NClient::EPromoType::PT_NO_TYPE, applyCtx.ModifierContext.GetFeatures().GetProductScenarioName(),
                             applyCtx.ResponseBody.GetModifierBody().GetLayout().GetOutputSpeech());
        matchingsPtr && !matchingsPtr->empty()) {
        applyCtx.ResponseBody.SetRandomPhrase(*matchingsPtr, applyCtx.ModifierContext.Rng());
        return Nothing();
    }
    return TNonApply{"No mapping found"};
}

} // namespace NAlice::NHollywood::NModifiers
