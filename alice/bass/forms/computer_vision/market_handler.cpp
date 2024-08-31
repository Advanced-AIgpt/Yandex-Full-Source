#include <alice/bass/forms/computer_vision/market_handler.h>

#include <alice/bass/libs/metrics/metrics.h>

using namespace NBASS;

TComputerVisionMarketHandler::TComputerVisionMarketHandler()
    : TComputerVisionMainHandler(ECaptureMode::MARKET, false)
{
}

void TComputerVisionMarketHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionMarketHandler>();
    };
    handlers->emplace(TComputerVisionMarketHandler::FormName(), handler);
}
