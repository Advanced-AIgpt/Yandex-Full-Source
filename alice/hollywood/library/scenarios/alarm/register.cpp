#include <alice/hollywood/library/scenarios/alarm/handles/alarm_prepare.h>
#include <alice/hollywood/library/scenarios/alarm/handles/alarm_prepare_music_catalog.h>
#include <alice/hollywood/library/scenarios/alarm/handles/alarm_run.h>

#include <alice/hollywood/library/scenarios/alarm/nlg/register.h>

#include <alice/hollywood/library/music/music_resources.h>

#include <alice/hollywood/library/registry/registry.h>

namespace NAlice::NHollywood::NReminders {

REGISTER_SCENARIO("hollywood_alarm",
                  AddHandle<TAlarmRunHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NAlarm::NNlg::RegisterAll));

REGISTER_SCENARIO("alarm",
                  AddHandle<TAlarmPrepareHandle>()
                  .AddHandle<TAlarmPrepareMusicCatalogHandle>()
                  .AddHandle<TAlarmRunHandle>()
                  .SetResources<NMusic::TMusicResources>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NAlarm::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NReminders
