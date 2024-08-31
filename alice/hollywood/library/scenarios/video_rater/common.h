#pragma once

#include <util/generic/strbuf.h>

namespace NAlice::NHollywood::NVideoRater {

constexpr TStringBuf DATASYNC_KV_PATH =
    "/v1/personality/profile/alisa/kv/video_rater"
;
constexpr TStringBuf INIT_FRAME = "alice.video_rater.init";
constexpr TStringBuf RATE_FRAME = "alice.video_rater.rate";
constexpr TStringBuf QUIT_FRAME = "alice.video_rater.quit";
constexpr TStringBuf IRRELEVANT_FRAME = "alice.video_rater.irrelevant";

} // namespace NAlice::NHollywood::NVideoRater
