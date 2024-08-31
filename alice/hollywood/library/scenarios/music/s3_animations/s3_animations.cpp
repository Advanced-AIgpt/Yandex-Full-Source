#include "s3_animations.h"

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>

namespace NAlice::NHollywood::NMusic {

namespace {

THashMap<TMusicArguments_EPlayerCommand, TString> S3_PATHS_MAP = {
    {TMusicArguments_EPlayerCommand_NextTrack, "animations/player_next"},
    {TMusicArguments_EPlayerCommand_PrevTrack, "animations/player_prev"},
    {TMusicArguments_EPlayerCommand_Continue, "animations/player_resume"},
    {TMusicArguments_EPlayerCommand_Like, "animations/player_like"},
    {TMusicArguments_EPlayerCommand_Dislike, "animations/player_dislike"},
};

} // namespace

TMaybe<TString> TryGetS3AnimationPath(const TMusicArguments_EPlayerCommand playerCommand) {
    if (const auto* s3Path = S3_PATHS_MAP.FindPtr(playerCommand); s3Path != nullptr) {
        return *s3Path;
    }
    return Nothing();
}

} // NAlice::NHollywood::NMusic
