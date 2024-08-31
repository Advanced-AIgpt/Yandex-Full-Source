#pragma once

#include <alice/megamind/library/session/session.h>
#include <alice/megamind/library/speechkit/request.h>

class TFactorStorage;

namespace NAlice {

void FillDeviceStateFactors(const TSpeechKitRequest& request, TFactorStorage& storage);
void FillSessionFactors(const ISession* session, TFactorStorage& storage);

} // namespace NAlice
