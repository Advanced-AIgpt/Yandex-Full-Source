#pragma once

#include <util/generic/string.h>

namespace NAlice::NVins::NWizard {

inline constexpr size_t CLIP_NORMALIZE_SIZE = 256;

void DoClipNormalization(TString& text);

} // namespace NAlice::NVins::NWizard
