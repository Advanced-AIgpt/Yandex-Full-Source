#include <alice/bass/forms/computer_vision/clothes_handler.h>

#include <alice/bass/libs/metrics/metrics.h>

using namespace NBASS;

TComputerVisionClothesHandler::TComputerVisionClothesHandler()
        : TComputerVisionMainHandler(ECaptureMode::CLOTHES, false)
{
}

void TComputerVisionClothesHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionClothesHandler>();
    };
    handlers->emplace(TComputerVisionClothesHandler::FormName(), handler);
}
