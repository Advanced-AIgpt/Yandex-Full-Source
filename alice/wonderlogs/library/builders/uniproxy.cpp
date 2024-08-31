#include "uniproxy.h"

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/protos/request_stat.pb.h>

#include <alice/megamind/protos/speechkit/request.pb.h>

#include <alice/library/unittest/message_diff.h>

#include <util/generic/maybe.h>
#include <util/string/builder.h>

namespace {

const TString DEFAULT_STREAM_TYPE = "other";

} // namespace

namespace NAlice::NWonderlogs {

TUniproxyPreparedBuilder::TUniproxyPreparedBuilder() {
    UniproxyPrepared.SetSuccessfulClientRetry(false);
    UniproxyPrepared.MutablePresence()->SetMegamindRequest(false);
    UniproxyPrepared.MutablePresence()->SetMegamindResponse(false);
    UniproxyPrepared.MutablePresence()->SetSpotterValidation(false);
    UniproxyPrepared.MutablePresence()->SetRequestStat(false);
    UniproxyPrepared.MutablePresence()->SetSpotterStream(false);
    UniproxyPrepared.MutablePresence()->SetStream(false);
    UniproxyPrepared.MutablePresence()->SetLogSpotter(false);
    UniproxyPrepared.MutablePresence()->SetVoiceInput(false);
    UniproxyPrepared.MutablePresence()->SetAsrResult(false);
    UniproxyPrepared.MutablePresence()->SetAsrRecognize(false);
    UniproxyPrepared.MutablePresence()->SetSynchronizeState(false);
    UniproxyPrepared.MutablePresence()->SetMegamindTimings(false);
    UniproxyPrepared.MutablePresence()->SetTtsTimings(false);
    UniproxyPrepared.MutablePresence()->SetTtsGenerate(false);
    UniproxyPrepared.MutablePresence()->SetAsrDebug(false);
    UniproxyPrepared.SetRealMessageId(true);
}

TUniproxyPreparedBuilder::TErrors TUniproxyPreparedBuilder::SetUuid(const TString& uuid) {
    TErrors errors;
    if (!UniproxyPrepared.HasUuid()) {
        UniproxyPrepared.SetUuid(uuid);
    } else if (UniproxyPrepared.GetUuid() != uuid) {
        errors.emplace_back(
            GenerateError(TUniproxyPrepared::TError::R_DIFFERENT_VALUES,
                          TStringBuilder{} << "Got different uuid: " << UniproxyPrepared.GetUuid() << " " << uuid));
    }
    return errors;
}

TUniproxyPreparedBuilder::TErrors TUniproxyPreparedBuilder::SetMessageId(const TString& messageId) {
    TErrors errors;
    if (!UniproxyPrepared.HasMessageId()) {
        UniproxyPrepared.SetMessageId(messageId);
    } else if (UniproxyPrepared.GetMessageId() != messageId) {
        errors.emplace_back(GenerateError(
            TUniproxyPrepared::TError::R_DIFFERENT_VALUES,
            TStringBuilder{} << "Got different message_id: " << UniproxyPrepared.GetMessageId() << " " << messageId));
    }
    return errors;
}

TUniproxyPreparedBuilder::TErrors TUniproxyPreparedBuilder::SetMegamindRequestId(const TString& megamindRequestId) {
    TErrors errors;
    if (!UniproxyPrepared.HasMegamindRequestId()) {
        UniproxyPrepared.SetMegamindRequestId(megamindRequestId);
    } else if (UniproxyPrepared.GetMegamindRequestId() != megamindRequestId) {
        // https://st.yandex-team.ru/VOICESERV-3341
        errors.emplace_back(GenerateError(TUniproxyPrepared::TError::R_DIFFERENT_VALUES,
                                          TStringBuilder{} << "Got different megamind_request_id: "
                                                           << UniproxyPrepared.GetMegamindRequestId() << " "
                                                           << megamindRequestId));
    }
    return errors;
}

TUniproxyPreparedBuilder::TErrors TUniproxyPreparedBuilder::SetConnectSessionId(const TString& connectSessionId) {
    TErrors errors;
    if (!UniproxyPrepared.HasConnectSessionId()) {
        UniproxyPrepared.SetConnectSessionId(connectSessionId);
    } else if (UniproxyPrepared.GetConnectSessionId() != connectSessionId) {
        // https://st.yandex-team.ru/MEGAMIND-1674
        errors.emplace_back(GenerateError(TUniproxyPrepared::TError::R_DIFFERENT_VALUES,
                                          TStringBuilder{} << "Got different connect_session_id:"
                                                           << UniproxyPrepared.GetConnectSessionId() << " "
                                                           << connectSessionId));
    }
    return errors;
}

TUniproxyPreparedBuilder::TErrors
TUniproxyPreparedBuilder::SetEnvironment(const TUniproxyPrepared::TEnvironment& environment) {
    TErrors errors;
    if (!UniproxyPrepared.HasEnvironment()) {
        *UniproxyPrepared.MutableEnvironment() = environment;
    } else if (!google::protobuf::util::MessageDifferencer::Equals(UniproxyPrepared.GetEnvironment(), environment)) {
        errors.emplace_back(GenerateError(TUniproxyPrepared::TError::R_DIFFERENT_VALUES,
                                          TStringBuilder{} << "Got different environment: "
                                                           << UniproxyPrepared.GetEnvironment().AsJSON() << " "
                                                           << environment.AsJSON()));
    }
    return errors;
}

TUniproxyPreparedBuilder::TErrors TUniproxyPreparedBuilder::SetClientIp(const TString& clientIp) {
    TErrors errors;
    if (!UniproxyPrepared.HasClientIp()) {
        UniproxyPrepared.SetClientIp(clientIp);
    } else if (UniproxyPrepared.GetClientIp() != clientIp) {
        errors.emplace_back(GenerateError(
            TUniproxyPrepared::TError::R_DIFFERENT_VALUES,
            TStringBuilder{} << "Got different client_ip: " << UniproxyPrepared.GetClientIp() << " " << clientIp));
    }
    return errors;
}

TUniproxyPreparedBuilder::TErrors TUniproxyPreparedBuilder::SetDoNotUseUserLogs(const bool doNotUseUserLogs) {
    TErrors errors;
    if (UniproxyPrepared.HasDoNotUseUserLogs() && UniproxyPrepared.GetDoNotUseUserLogs() != doNotUseUserLogs) {
        errors.emplace_back(GenerateError(TUniproxyPrepared::TError::R_DIFFERENT_VALUES,
                                          TStringBuilder{} << "Got different do_not_use_user_logs: "
                                                           << UniproxyPrepared.GetDoNotUseUserLogs() << " "
                                                           << doNotUseUserLogs));
    }
    UniproxyPrepared.SetDoNotUseUserLogs(UniproxyPrepared.GetDoNotUseUserLogs() || doNotUseUserLogs);
    return errors;
}

TUniproxyPrepared::TError TUniproxyPreparedBuilder::GenerateError(TUniproxyPrepared::TError::EReason reason,
                                                                  const TString& message) {
    TUniproxyPrepared::TError error;
    error.SetProcess(TUniproxyPrepared::TError::P_UNIPROXY_PREPARED_REDUCER);
    error.SetReason(reason);
    error.SetMessage(message);
    if (UniproxyPrepared.HasUuid()) {
        error.SetUuid(UniproxyPrepared.GetUuid());
    }
    if (UniproxyPrepared.HasMessageId() && UniproxyPrepared.GetRealMessageId()) {
        error.SetMessageId(UniproxyPrepared.GetMessageId());
    }
    if (auto setraceUrl = TryGenerateSetraceUrl({error.HasMessageId() ? error.GetMessageId() : TMaybe<TString>{},
                                                 error.HasUuid() ? error.GetUuid() : TMaybe<TString>{}})) {
        error.SetSetraceUrl(*setraceUrl);
    }
    return error;
}

void TUniproxyPreparedBuilder::Append(TUniproxyPreparedBuilder::TErrors& errorsTo,
                                      TUniproxyPreparedBuilder::TErrors&& errorsFrom) {
    for (auto& error : errorsFrom) {
        errorsTo.emplace_back(std::move(error));
    }
}

TUniproxyPreparedBuilder::TErrors
TUniproxyPreparedBuilder::AddMegamindRequest(const TUniproxyPrepared::TMegamindRequest& megamindRequest) {
    TErrors errors;
    Append(errors, InitializeCommonFields(megamindRequest));
    Append(errors, SetMegamindRequestId(megamindRequest.GetMegamindRequestId()));
    UniproxyPrepared.MutablePresence()->SetMegamindRequest(true);
    return errors;
}

TUniproxyPreparedBuilder::TErrors
TUniproxyPreparedBuilder::AddMegamindResponse(const TUniproxyPrepared::TMegamindResponse& megamindResponse) {
    TErrors errors;
    Append(errors, InitializeCommonFields(megamindResponse));
    Append(errors, SetMegamindRequestId(megamindResponse.GetMegamindRequestId()));
    UniproxyPrepared.SetMegamindResponseId(megamindResponse.GetMegamindResponseId());
    UniproxyPrepared.MutablePresence()->SetMegamindResponse(true);
    return errors;
}

TUniproxyPreparedBuilder::TErrors
TUniproxyPreparedBuilder::AddRequestStat(const TUniproxyPrepared::TRequestStatWrapper& requestStat) {
    TErrors errors;
    *UniproxyPrepared.MutableRequestStat() = requestStat.GetRequestStat();
    Append(errors, InitializeCommonFields(requestStat));
    UniproxyPrepared.MutablePresence()->SetRequestStat(true);
    return errors;
}

TUniproxyPreparedBuilder::TErrors
TUniproxyPreparedBuilder::AddClientRetry(const TUniproxyPrepared::TClientRetry& clientRetry) {
    TErrors errors;
    Append(errors, InitializeCommonFields(clientRetry));
    UniproxyPrepared.SetSuccessfulClientRetry(clientRetry.GetSuccessful());
    return errors;
}

TUniproxyPreparedBuilder::TErrors
TUniproxyPreparedBuilder::AddSpotterValidation(const TUniproxyPrepared::TSpotterValidation& spotterValidation) {
    TErrors errors;
    Append(errors, InitializeCommonFields(spotterValidation));
    *UniproxyPrepared.MutableSpotterValidation() = spotterValidation.GetValue();
    UniproxyPrepared.MutablePresence()->SetSpotterValidation(true);
    return errors;
}

TUniproxyPreparedBuilder::TErrors
TUniproxyPreparedBuilder::AddSpotterStream(const TUniproxyPrepared::TSpotterStream& spotterStream) {
    TErrors errors;
    if (!spotterStream.GetRealMessageId()) {
        UniproxyPrepared.SetRealMessageId(false);
    }
    Append(errors, InitializeCommonFields(spotterStream));
    *UniproxyPrepared.MutableSpotterStream() = spotterStream.GetValue();
    UniproxyPrepared.MutablePresence()->SetSpotterStream(true);
    return errors;
}

TUniproxyPreparedBuilder::TErrors TUniproxyPreparedBuilder::AddStream(const TUniproxyPrepared::TStream& stream) {
    TErrors errors;
    if (!stream.GetRealMessageId()) {
        UniproxyPrepared.SetRealMessageId(false);
    }
    Append(errors, InitializeCommonFields(stream));
    *UniproxyPrepared.MutableAsrStream() = stream.GetValue();
    UniproxyPrepared.MutablePresence()->SetStream(true);
    return errors;
}

TUniproxyPreparedBuilder::TErrors TUniproxyPreparedBuilder::AddLogSpotterWithStreams(
    const TUniproxyPrepared::TLogSpotterWithStreams& logSpotterWithStreams) {
    TErrors errors;
    if (!logSpotterWithStreams.GetRealMessageId()) {
        UniproxyPrepared.SetRealMessageId(false);
    }
    Append(errors, InitializeCommonFields(logSpotterWithStreams));
    if (logSpotterWithStreams.HasLogSpotter()) {
        *UniproxyPrepared.MutableLogSpotter() = logSpotterWithStreams.GetLogSpotter();
        UniproxyPrepared.MutablePresence()->SetLogSpotter(true);
    }
    TString streamType = DEFAULT_STREAM_TYPE;
    if (logSpotterWithStreams.GetLogSpotter().GetSpotterActivationInfo().HasStreamType()) {
        streamType = logSpotterWithStreams.GetLogSpotter().GetSpotterActivationInfo().GetStreamType();
    }
    for (const auto& spotterStream : logSpotterWithStreams.GetStreams()) {
        *(*UniproxyPrepared.MutableSpotterStreams())[streamType].MutableStreams()->Add() = spotterStream;
    }
    return errors;
}

TUniproxyPreparedBuilder::TErrors TUniproxyPreparedBuilder::AddMessageIdToConnectSessionId(
    const TUniproxyPrepared::TMessageIdToConnectSessionId& messageIdToConnectSessionId) {
    TErrors errors;
    Append(errors, InitializeCommonFields(messageIdToConnectSessionId));
    Append(errors, SetConnectSessionId(messageIdToConnectSessionId.GetConnectSessionId()));
    return errors;
}

TUniproxyPreparedBuilder::TErrors TUniproxyPreparedBuilder::AddMessageIdToEnvironment(
    const TUniproxyPrepared::TMessageIdToEnvironment& messageIdToEnvironment) {
    TErrors errors;
    Append(errors, InitializeCommonFields(messageIdToEnvironment));
    Append(errors, SetEnvironment(messageIdToEnvironment.GetEnvironment()));
    return errors;
}

TUniproxyPreparedBuilder::TErrors
TUniproxyPreparedBuilder::AddMessageIdToClientIp(const TUniproxyPrepared::TMessageIdToClientIp& messageIdToClientIp) {
    TErrors errors;
    Append(errors, InitializeCommonFields(messageIdToClientIp));
    Append(errors, SetClientIp(messageIdToClientIp.GetClientIp()));
    return errors;
}

TUniproxyPreparedBuilder::TErrors
TUniproxyPreparedBuilder::AddVoiceInput(const TUniproxyPrepared::TVoiceInput& voiceInput) {
    TErrors errors;
    Append(errors, InitializeCommonFields(voiceInput));
    if (voiceInput.GetValue().GetSpeechKitRequest().GetHeader().HasRequestId()) {
        SetMegamindRequestId(voiceInput.GetValue().GetSpeechKitRequest().GetHeader().GetRequestId());
    }
    *UniproxyPrepared.MutableVoiceInput() = voiceInput.GetValue();
    UniproxyPrepared.MutablePresence()->SetVoiceInput(true);
    return errors;
}

TUniproxyPreparedBuilder::TErrors
TUniproxyPreparedBuilder::AddAsrRecognize(const TUniproxyPrepared::TAsrRecognize& asrRecognize) {
    TErrors errors;
    Append(errors, InitializeCommonFields(asrRecognize));
    *UniproxyPrepared.MutableAsrRecognize() = asrRecognize.GetValue();
    UniproxyPrepared.MutablePresence()->SetAsrRecognize(true);
    return errors;
}

TUniproxyPreparedBuilder::TErrors
TUniproxyPreparedBuilder::AddAsrResult(const TUniproxyPrepared::TAsrResult& asrResult) {
    TErrors errors;
    Append(errors, InitializeCommonFields(asrResult));
    *UniproxyPrepared.MutableAsrResult() = asrResult.GetValue();
    UniproxyPrepared.MutablePresence()->SetAsrResult(true);
    return errors;
}

TUniproxyPreparedBuilder::TErrors TUniproxyPreparedBuilder::AddAsrDebug(const TUniproxyPrepared::TAsrDebug& asrDebug) {
    TErrors errors;
    Append(errors, InitializeCommonFields(asrDebug));
    *UniproxyPrepared.MutableAsrDebug() = asrDebug.GetValue();
    UniproxyPrepared.MutablePresence()->SetAsrDebug(true);
    return errors;
}

TUniproxyPreparedBuilder::TErrors TUniproxyPreparedBuilder::AddSynchronizeStateWithMessageId(
    const TUniproxyPrepared::TSynchronizeStateWithMessageId& synchronizeStateWithMessageId) {
    TErrors errors;
    Append(errors, InitializeCommonFields(synchronizeStateWithMessageId));
    *UniproxyPrepared.MutableSynchronizeState() = synchronizeStateWithMessageId.GetValue();
    UniproxyPrepared.MutablePresence()->SetSynchronizeState(true);
    return errors;
}

TUniproxyPreparedBuilder::TErrors
TUniproxyPreparedBuilder::AddMegamindTimings(const TUniproxyPrepared::TMegamindTimings& megamindTimings) {
    TErrors errors;
    Append(errors, InitializeCommonFields(megamindTimings));
    *UniproxyPrepared.MutableMegamindTimings() = megamindTimings.GetValue();
    UniproxyPrepared.MutablePresence()->SetMegamindTimings(true);
    return errors;
}

TUniproxyPreparedBuilder::TErrors
TUniproxyPreparedBuilder::AddTtsTimings(const TUniproxyPrepared::TTtsTimings& ttsTimings) {
    TErrors errors;
    Append(errors, InitializeCommonFields(ttsTimings));
    *UniproxyPrepared.MutableTtsTimings() = ttsTimings.GetValue();
    UniproxyPrepared.MutablePresence()->SetTtsTimings(true);
    return errors;
}

TUniproxyPreparedBuilder::TErrors TUniproxyPreparedBuilder::AddTestIds(const TUniproxyPrepared::TTestIds& testIds) {
    TErrors errors;
    Append(errors, InitializeCommonFields(testIds));
    for (const auto testId : testIds.GetTestIds()) {
        UniproxyPrepared.MutableTestIds()->Add(testId);
    }
    UniproxyPrepared.MutablePresence()->SetTestIds(true);
    return errors;
}

TUniproxyPreparedBuilder::TErrors
TUniproxyPreparedBuilder::AddTtsGenerate(const TUniproxyPrepared::TTtsGenerate& ttsGenerate) {
    TErrors errors;
    Append(errors, InitializeCommonFields(ttsGenerate));
    *UniproxyPrepared.MutableTtsGenerate() = ttsGenerate.GetValue();
    UniproxyPrepared.MutablePresence()->SetTtsGenerate(true);
    return errors;
}

TUniproxyPreparedBuilder::TErrors TUniproxyPreparedBuilder::AddMessageIdToDoNotUseUserLogs(
    const TUniproxyPrepared::TMessageIdToDoNotUseUserLogs& messageIdToDoNotUseUserLogs) {
    TErrors errors;
    Append(errors, InitializeCommonFields(messageIdToDoNotUseUserLogs));
    Append(errors, SetDoNotUseUserLogs(messageIdToDoNotUseUserLogs.GetDoNotUseUserLogs()));
    return errors;
}

TUniproxyPrepared TUniproxyPreparedBuilder::Build() && {
    return std::move(UniproxyPrepared);
}

} // namespace NAlice::NWonderlogs
