#pragma once

#include <alice/hollywood/library/request/request.h>
#include <alice/library/logger/fwd.h>

#include <util/generic/noncopyable.h>

#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

namespace NAlice::NHollywood::NMusic {

// multiroom token generation and retrieving
class TMultiroomTokenWrapper : private NNonCopyable::TNonCopyable {
public:
    TMultiroomTokenWrapper(TScenarioState& scenarioState);

    void GenerateNewToken();
    void DropToken();
    TStringBuf GetToken() const;

private:
    TScenarioState& ScenarioState_;
};

// callbacks to call while observing multiroom status
struct TMultiroomCallbacks {
    virtual ~TMultiroomCallbacks() = default;
    virtual void OnNeedDropMultiroomToken() {}
    virtual void OnNeedStartMultiroom(NScenarios::TLocationInfo /* locationInfo */) {}
    virtual void OnNeedStopMultiroom(TStringBuf /* sessionId */) {}
    virtual void OnMultiroomNotSupported() {}
    virtual void OnMultiroomRoomsNotSupported() {}
};

// XXX(sparkle): do we also need RunRequest?
void ProcessMultiroom(const NHollywood::TScenarioApplyRequestWrapper& request,
                      TMultiroomCallbacks& callbacks);

bool WillPlayInMultiroomSession(const NHollywood::TScenarioApplyRequestWrapper& request);

// This is a multiroom request, but it can't be processed in thin music
bool HasForbiddenMultiroom(NAlice::TRTLogger& logger,
                           const TScenarioRunRequestWrapper& request,
                           const TFrame& musicPlayFrame);

} // namespace NAlice::NHollywood::NMusic
