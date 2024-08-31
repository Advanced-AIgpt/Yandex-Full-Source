#include "show_gif_resources.h"

#include <alice/hollywood/library/gif_card/gif_card.h>

#include <library/cpp/json/json_reader.h>

#include <util/stream/file.h>

namespace NAlice::NHollywood {

void TShowGifResources::LoadFromPath(const TFsPath& dirPath) {
    TFileInput in(dirPath / "gifs.json");
    NJson::TJsonValue json;
    Y_ENSURE(NJson::ReadJsonTree(&in, &json));
    for (const auto& node : json.GetArraySafe()) {
        Gifs_.push_back(GifFromJson(node));
    }
}

} // namespace NAlice::NHollywood
