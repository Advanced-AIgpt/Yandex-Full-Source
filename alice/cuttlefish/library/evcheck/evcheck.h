#pragma once
#include "parser.h"

namespace NVoice {

TParser ConstructSynchronizeStateParser();

THolder<TParser> ConstructSynchronizeStateParserInHeap() noexcept;

} // namespace NVoice
