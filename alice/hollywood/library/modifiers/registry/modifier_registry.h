#pragma once

#include <alice/hollywood/library/modifiers/base_modifier/base_modifier.h>

#include <alice/hollywood/library/config/config.pb.h>

#include <library/cpp/protobuf/util/pb_io.h>

#include <util/generic/vector.h>
#include <util/folder/path.h>

namespace NAlice::NHollywood::NModifiers {

using TModifierProducer = std::function<TBaseModifierPtr()>;
using TModifiersConfig = TConfig::THwServicesConfig::TModifiersConfig;

class TModifierRegistry {

public:
    static TModifierRegistry& Get();

    void AddModifierProducer(const TModifierProducer& producer);
    TVector<TBaseModifierPtr> CreateModifiers(const TModifiersConfig& modifiersConfig, const TFsPath& resourcesBasePath);

private:
    TVector<TModifierProducer> Producers_;
};

struct TModifierRegistrator {
    explicit TModifierRegistrator(const TModifierProducer& producer);
};

} // namespace NAlice::NHollywood::NModifiers

#define REGISTER_HOLLYWOOD_MODIFIER(modifierClassName) \
static NAlice::NHollywood::NModifiers::TModifierRegistrator \
Y_GENERATE_UNIQUE_ID(ModifierRegistrator)([]() -> std::unique_ptr<TBaseModifier> { \
return std::make_unique<modifierClassName>(); \
})
