#pragma once

#include <alice/personal_cards/config/config.pb.h>

#include <library/cpp/tvmauth/client/facade.h>

namespace NPersonalCards {

using TTvmClientPtr = TAtomicSharedPtr<NTvmAuth::TTvmClient>;

TTvmClientPtr CreateTvmClient(const TTvmConfig& tvmConfig, const bool withRetries);

} // namespace NPersonalCards
