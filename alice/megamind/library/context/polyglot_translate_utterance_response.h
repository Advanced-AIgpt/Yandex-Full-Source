#pragma once

#include <util/generic/string.h>

namespace NAlice {

    class TPolyglotTranslateUtteranceResponse {
    public:
        TPolyglotTranslateUtteranceResponse() = default;

        explicit TPolyglotTranslateUtteranceResponse(TString translatedUtterance)
            : TranslatedUtterance_(std::move(translatedUtterance))
        {
        }

        const TString& GetTranslatedUtterance() const {
            return TranslatedUtterance_;
        }
    private:
        TString TranslatedUtterance_;
    };

}
