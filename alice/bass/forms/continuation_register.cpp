#include "continuation_register.h"

#include <alice/bass/forms/music/music.h>
#include <alice/bass/forms/search/direct_gallery.h>
#include <alice/bass/forms/video/video.h>

namespace NBASS {

void RegisterContinuations(TContinuationParserRegistry& registry) {
    NMusic::RegisterMusicContinuations(registry);
    RegisterVideoContinuations(registry);
    NDirectGallery::RegisterDirectGalleryContinuation(registry);
}

} // namespace NBASS
