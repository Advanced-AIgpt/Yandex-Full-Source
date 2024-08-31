#include <alice/bass/forms/computer_vision/ocr_handler.h>

#include <alice/bass/libs/metrics/metrics.h>

using namespace NBASS;

TComputerVisionOcrHandler::TComputerVisionOcrHandler()
        : TComputerVisionMainHandler(ECaptureMode::OCR, false)
{
}

void TComputerVisionOcrHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionOcrHandler>();
    };
    handlers->emplace(TComputerVisionOcrHandler::FormName(), handler);
}
