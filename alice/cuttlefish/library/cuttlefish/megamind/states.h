#pragma once

#include <alice/cuttlefish/library/evproc/states.h>

namespace NAlice::NCuttlefish::NAppHostServices::NMegamindStates {

    /*
    *  @brief waiting receive mm_request
    */
    struct TWaitingForMmRequestState: NSM::TStateBase<TWaitingForMmRequestState> {
        NSM_STATE("WaitingForRequest");
    };

    /*
     *  @brief save potential queries to Megamind to queue, wait for contexts to be loaded
     */
    struct TWaitingForContextsState: NSM::TStateBase<TWaitingForContextsState> {
        NSM_STATE("WaitingForContexts");
    };

    /*
     *  @brief (MEGAMIND_APPLY only) got context, but yet not make any requests to MM
     */
    struct TWaitingForApplyRequestState: NSM::TStateBase<TWaitingForApplyRequestState> {
        NSM_STATE("WaitingForApplyRequest");
    };

    /*
     *  @brief (MEGAMIND_RUN only) send a query to Megamind for each partial from ASR until we get EOU from ASR
     */
    struct TWaitingForEouState: NSM::TStateBase<TWaitingForEouState> {
        NSM_STATE("WaitingForEou");
    };

    /*
     *  @brief wait for response from the Music backend
     */
    struct TWaitingForMusicState: NSM::TStateBase<TWaitingForMusicState> {
        NSM_STATE("WaitingForMusic");
    };

    /*
     *  @brief save potential queries to Megamind to queue, wait for response from the Spotter Validation
     */
    struct TWaitingForSpotterState: NSM::TStateBase<TWaitingForSpotterState> {
        NSM_STATE("WaitingForSpotter");
    };

    /*
     *  @brief wait for final response from Megamind
     */
    struct TWaitingForFinalSubrequestResponseState: NSM::TStateBase<TWaitingForFinalSubrequestResponseState> {
        NSM_STATE("WaitingForFinalSubrequestResponse");
    };

    /*
     *  @brief process megamind response (or error)
     */
    struct TProcessFinalSubrequestResponseState: NSM::TStateBase<TProcessFinalSubrequestResponseState> {
        NSM_STATE("ProcessFinalSubrequestResponse");
    };

    /*
     *  @brief good final state, do nothing
     */
    struct TFinalState: NSM::TStateBase<TFinalState> {
        NSM_STATE("Final");
    };

    /*
     *  @brief bad final state (when got reject/cancel from Spotter Validation), do nothing
     */
    struct TRejectedState: NSM::TStateBase<TRejectedState> {
        NSM_STATE("Rejected");
    };

} // namespace NAlice::NCuttlefish::NAppHostServices::NMegamindStates
