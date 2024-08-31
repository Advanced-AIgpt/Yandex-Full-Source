#pragma once
#include "base.h"

namespace NAlice::NHollywood::NSearch {

const TString OPEN_APP_FRAME = "alice.search_open_app";

class TSearchAppNavScenario final : public TSearchScenario {
public:
    using TSearchScenario::TSearchScenario;

    bool FixlistAnswers(const TSearchResult& response);
    bool FrameFixlistAnswers();

    static bool HandleAppConfirmationButton(TSearchContext& ctx);

private:
    NSc::TValue GetFrameFixlistAnswer();
    bool ProcessFixlistAnswer(const NSc::TValue& fixlistAnswer, bool confirmed);
    void AddOpenAppConfirmButton(const TString& appData);
};

} // namespace NAlice::NHollywood
