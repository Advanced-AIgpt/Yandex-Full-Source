#pragma once

#include <alice/megamind/protos/scenarios/features/music.pb.h>

#include <library/cpp/scheme/scheme.h>

namespace NAlice::NHollywood {

ui32 FillMusicFeaturesProto(const TStringBuf searchText, const NJson::TJsonValue& searchResult, bool isPlayerCommand,
                            NScenarios::TMusicFeatures& features);

} // namespace NAlice::NHollywood
