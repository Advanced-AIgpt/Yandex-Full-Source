#pragma once

#include <alice/bass/forms/computer_vision/context.h>
#include <alice/bass/forms/computer_vision/handler.h>

namespace NBASS {

    class TComputerVisionMarketHandler : public TComputerVisionMainHandler {
    public:
        TComputerVisionMarketHandler();

        static void Register(THandlersMap* handlers);

        static inline TStringBuf FormShortName() {
            return TStringBuf("image_what_is_this_market");
        }

        static inline TStringBuf FormName() {
            return TStringBuf("personal_assistant.scenarios.image_what_is_this_market");
        }
    };

}

