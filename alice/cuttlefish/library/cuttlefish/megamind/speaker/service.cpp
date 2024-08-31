#include <alice/cuttlefish/library/cuttlefish/megamind/speaker/service.h>
#include <alice/cuttlefish/library/cuttlefish/megamind/speaker/context.h>
#include <util/generic/yexception.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    const int APPROXIMATE_NUMBER_OF_GUESTS = 10;

    TSpeakerService::TSpeakerService()
        : Speakers(APPROXIMATE_NUMBER_OF_GUESTS)
        , CurrentActiveSpeaker(NOBODY)
    {
    }

    void TSpeakerService::OnDatasyncResponse(NAppHostHttp::THttpResponse response, TTicket ticket) {
        EnrichFromDatasyncResponse(*GetOrCreate(ticket), std::move(response));
    }

    void TSpeakerService::OnBlackboxResponse(NAppHostHttp::THttpResponse response, TTicket ticket) {
        EnrichFromBlackboxResponse(*GetOrCreate(ticket), std::move(response));
    }

    void TSpeakerService::OnMatch(NAliceProtocol::TMatchVoiceprintResult matchResult, TTicket ticket) {
        EnrichFromMatchResult(*GetOrCreate(ticket), std::move(matchResult));
        ChangeActiveSpeaker(ticket);
    }

    void TSpeakerService::OnNoMatch() {
        ChangeActiveSpeaker(NOBODY);
    }

    void TSpeakerService::ChangeActiveSpeaker(TTicket ticket) {
        CurrentActiveSpeaker = ticket;
    }

    TAtomicSharedPtr<TSpeakerContext> TSpeakerService::GetActiveSpeaker() const {
        return CurrentActiveSpeaker != NOBODY
            ? Speakers[CurrentActiveSpeaker]
            : nullptr;
    }

    TSpeakerService::TSpeakerContextPtr TSpeakerService::GetOrCreate(TTicket ticket) {
        // Actually, the number of unique speakers should not exceed ~10
        if (ticket >= (int)Speakers.size()) {
            ythrow yexception() << "Too big speakerId: " << ticket;
        }

        if (!Speakers[ticket]) {
            Speakers[ticket] = MakeHolder<TSpeakerContext>();
        }

        return Speakers[ticket];
    }

}
