#include "state_machine.h"

#include "megamind.h"

using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NMegamindEvents;
using namespace NMegamindStates;

void TMegamindRunServantStateMachine::OnPostFinal() {
    NAliceProtocol::TRequestDebugInfo di;
    Ctx_.Timings.SetFinish(TInstant::Now().MilliSeconds());
    *di.MutableMegamindRun() = Ctx_.Timings;
    MegamindServant_.AddRequestDebugInfo(std::move(di));
}

void TMegamindApplyServantStateMachine::OnPostFinal() {
    NAliceProtocol::TRequestDebugInfo di;
    Ctx_.Timings.SetFinish(TInstant::Now().MilliSeconds());
    *di.MutableMegamindApply() = Ctx_.Timings;
    MegamindServant_.AddRequestDebugInfo(std::move(di));
}
