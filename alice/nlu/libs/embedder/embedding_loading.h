#pragma once

#include "embedder.h"

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/stream/fwd.h>

namespace NAlice {

    THashMap<TString, TEmbedding> LoadEmbeddingsFromJson(IInputStream* input);

} // namespace NAlice
