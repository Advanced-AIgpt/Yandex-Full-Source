#pragma once

#include <alice/library/video_common/defs.h>
#include <alice/protos/data/video/video.pb.h>

#include <library/cpp/json/writer/json_value.h>
#include <util/generic/maybe.h>

namespace NAlice::NHollywood::NTvChannelsEfir {

TMaybe<NAlice::TVideoItem> ParseChannelJson(const NJson::TJsonValue& channel);

}
