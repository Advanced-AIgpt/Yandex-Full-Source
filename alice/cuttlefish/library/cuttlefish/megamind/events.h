#pragma once
#include <alice/cuttlefish/library/evproc/events.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>
#include <apphost/api/service/cpp/service.h>
#include "subrequest.h"

namespace NAlice::NCuttlefish::NAppHostServices::NMegamindEvents {

    struct TReceiveMmRequest: NSM::TEvent<TReceiveMmRequest> {
        NSM_EVENT("recv.mm_request");
    };

    struct TContextsLoaded: NSM::TEvent<TContextsLoaded> {
        NSM_EVENT("contexts.loaded");

        TContextsLoaded(const TString& s)
            : RequestName(s)
        {
        }

        const TString RequestName; // vins.textinput | vins.voiceinput
    };

    struct TFirstPartialAsrResult: NSM::TEvent<TFirstPartialAsrResult> {
        NSM_EVENT("asr.result.partial.first");
    };

    struct TNotEmptyPartialAsrResult: NSM::TEvent<TNotEmptyPartialAsrResult> {
        NSM_EVENT("asr.result.partial");
    };

    struct TSendSubrequest: NSM::TEvent<TSendSubrequest> {
        NSM_EVENT("send.subrequest");
    };

    struct TReceiveEouAsrResult: NSM::TEvent<TReceiveEouAsrResult> {
        NSM_EVENT("asr.result.eou");

        TReceiveEouAsrResult(bool empty)
            : Empty{empty}
        {}

        const bool Empty{false};
    };

    struct TUseEouAsrResult: NSM::TEvent<TUseEouAsrResult> {
        NSM_EVENT("asr.result.eou.use");
        TUseEouAsrResult(bool postponed=false, TSubrequestPtr subrequest = {}, bool subrequestFinished=false)
            : Postponed(postponed)
            , Subrequest{subrequest}
            , SubrequestFinished(subrequestFinished)
        {}

        const bool Postponed{false};
        TSubrequestPtr Subrequest;
        const bool SubrequestFinished{false};
    };

    struct TUsefulMegamindResponse: NSM::TEvent<TUsefulMegamindResponse> {
        NSM_EVENT("mm.result");

        TUsefulMegamindResponse(TInstant start, TInstant finish, const TString& intentName = {})
            : Start{start}
            , Finish{finish}
            , IntentName(intentName)
        {}

        const TInstant Start;
        const TInstant Finish;
        const TString IntentName;
    };

    struct TSuccess: NSM::TEvent<TSuccess> {
        NSM_EVENT("success");

        TSuccess(bool needApply=false)
            : NeedApply{needApply}
        {}

        const bool NeedApply;
    };

    struct TError: NSM::TEvent<TError> {
        NSM_EVENT("error");

        TError(const TString& errorCode, const TString& errorText)
            : ErrorCode{errorCode}
            , ErrorText{errorText}
        {}

        const TString ErrorCode;
        const TString ErrorText;
    };

    struct TCancelRequest: NSM::TEvent<TCancelRequest> {
        NSM_EVENT("request.cancel");
    };

    struct TReceiveApplyRequest: NSM::TEvent<TReceiveApplyRequest> {
        NSM_EVENT("request.apply");
    };

}
