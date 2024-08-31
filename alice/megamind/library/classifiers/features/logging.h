#pragma once

#include <alice/megamind/library/context/context.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>

#include <kernel/factor_storage/factor_storage.h>

namespace NAlice::NMegamind::NFeatures {

void LogFeatures(const IContext& ctx, const TFactorStorage& factorStorage, TQualityStorage& qualityStorage);

} // namespace NAlice::NMegamind::NFeatures
