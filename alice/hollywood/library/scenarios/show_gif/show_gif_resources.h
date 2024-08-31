#pragma once

#include <alice/hollywood/library/gif_card/proto/gif.pb.h>
#include <alice/hollywood/library/resources/resources.h>

namespace NAlice::NHollywood {

class TShowGifResources final : public IResourceContainer {
public:
    void LoadFromPath(const TFsPath& dirPath) override;

    const TVector<TGif>& Gifs() const {
        return Gifs_;
    }

private:
    TVector<TGif> Gifs_;
};

} // namespace NAlice::NHollywood
