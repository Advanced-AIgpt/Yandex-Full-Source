#pragma once

#include <alice/hollywood/library/request/request.h>

#include <alice/library/client/client_info.h>
#include <alice/library/data_sync/data_sync.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>

namespace NAlice::NHollywood {

TString ToPersonalDataKey(const TClientInfo& clientInfo, NAlice::NDataSync::EKey key, TMaybe<TString> persId = Nothing());

TMaybe<TString> GetOwnerNameFromDataSync(const TScenarioBaseRequestWrapper& request);

TMaybe<TString> GetKolonkishUidFromDataSync(const TScenarioBaseRequestWrapper& request);

TMaybe<TString> GetGenderFromDataSync(const TScenarioBaseRequestWrapper& request);

}  // namespace NAlice::NHollywood
