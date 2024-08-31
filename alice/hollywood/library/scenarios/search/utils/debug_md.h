#pragma once

#include <alice/library/logger/logger.h>

#include <alice/protos/data/scenario/search/richcard.pb.h>

namespace NAlice::NHollywoodFw::NSearch {

extern void DumpAsMd(TRTLogger& logger, const NData::TSearchRichCardData& richCard, bool asProto = false);

} // namespace NAlice::NHollywoodFw::NSearch
