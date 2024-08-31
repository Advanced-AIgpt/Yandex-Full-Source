#pragma once

#include <kernel/factor_slices/factor_domain.h>
#include <kernel/factor_storage/factor_storage.h>

namespace NAlice::NMegamind {

TFactorDomain CreateFactorDomain();
TFactorStorage CreateFactorStorage(const TFactorDomain& domain);

} // namespace NAlice::NMegamind
