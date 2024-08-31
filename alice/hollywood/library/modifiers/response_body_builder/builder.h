#pragma once

#include <alice/hollywood/library/modifiers/util/tts_and_text.h>

#include <alice/library/util/rng.h>
#include <alice/megamind/protos/modifiers/modifier_body.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywood::NModifiers {

using TModifierBody = NMegamind::TModifierBody;

class TResponseBodyBuilder : public NNonCopyable::TMoveOnly {
public:
    explicit TResponseBodyBuilder(TModifierBody&& proto);
    explicit TResponseBodyBuilder(const TModifierBody& proto);

    const TModifierBody& GetModifierBody() const;
    TModifierBody& MutableModifierBody();

    void SetCard(NScenarios::TLayout::TCard&& card, const size_t index);

    void SetVoiceAndText(const TString& tts, const TString& text);

    void SetText(const TString& text);

    void SetVoice(const TString& tts);

    void SetRandomPhrase(const TVector<TTtsAndText>& phrases, IRng& rng);

    void AppendVoice(const TString& tts);

    void PrependVoice(const TString& tts);

    void AddDirective(NScenarios::TDirective&& directive);

    void AddDirectiveToFront(NScenarios::TDirective&& directive);
    void AddDirectivesToFront(const TVector<NScenarios::TDirective>& directives);

    TModifierBody MoveProto() &&;

private:
    TModifierBody Proto;
};

} // namespace NAlice::NHollywood::NModifiers
