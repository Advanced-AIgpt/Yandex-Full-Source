#pragma once

#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>

#include <util/generic/vector.h>

namespace NAlice::NWonderlogs {

namespace NTestSuiteUniproxy {

struct TTestCaseDisjunctionDoNotUseUserLogsFalseFirst;
struct TTestCaseDisjunctionDoNotUseUserLogsTrueFirst;

} // namespace NTestSuiteUniproxy

class TUniproxyPreparedBuilder : public NNonCopyable::TMoveOnly {
public:
    using TErrors = TVector<TUniproxyPrepared::TError>;

    TUniproxyPreparedBuilder();

    TErrors AddMegamindRequest(const TUniproxyPrepared::TMegamindRequest& megamindRequest);
    TErrors AddMegamindResponse(const TUniproxyPrepared::TMegamindResponse& megamindResponse);
    TErrors AddRequestStat(const TUniproxyPrepared::TRequestStatWrapper& requestStat);
    TErrors AddClientRetry(const TUniproxyPrepared::TClientRetry& clientRetry);
    TErrors AddSpotterValidation(const TUniproxyPrepared::TSpotterValidation& spotterValidation);
    TErrors AddStream(const TUniproxyPrepared::TStream& stream);
    TErrors AddSpotterStream(const TUniproxyPrepared::TSpotterStream& spotterStream);
    TErrors AddLogSpotterWithStreams(const TUniproxyPrepared::TLogSpotterWithStreams& logSpotterWithStreams);
    TErrors
    AddMessageIdToConnectSessionId(const TUniproxyPrepared::TMessageIdToConnectSessionId& messageIdToConnectSessionId);
    TErrors AddMessageIdToEnvironment(const TUniproxyPrepared::TMessageIdToEnvironment& messageIdToEnvironment);
    TErrors AddMessageIdToClientIp(const TUniproxyPrepared::TMessageIdToClientIp& messageIdToClientIp);
    TErrors AddVoiceInput(const TUniproxyPrepared::TVoiceInput& voiceInput);
    TErrors AddAsrRecognize(const TUniproxyPrepared::TAsrRecognize& asrRecognize);
    TErrors AddAsrResult(const TUniproxyPrepared::TAsrResult& asrResult);
    TErrors AddAsrDebug(const TUniproxyPrepared::TAsrDebug& asrDebug);
    TErrors AddSynchronizeStateWithMessageId(
        const TUniproxyPrepared::TSynchronizeStateWithMessageId& synchronizeStateWithMessageId);
    TErrors AddMegamindTimings(const TUniproxyPrepared::TMegamindTimings& megamindTimings);
    TErrors AddTtsTimings(const TUniproxyPrepared::TTtsTimings& ttsTimings);
    TErrors AddTestIds(const TUniproxyPrepared::TTestIds& testIds);
    TErrors AddTtsGenerate(const TUniproxyPrepared::TTtsGenerate& ttsGenerate);
    TErrors
    AddMessageIdToDoNotUseUserLogs(const TUniproxyPrepared::TMessageIdToDoNotUseUserLogs& messageIdToDoNotUseUserLogs);

    TUniproxyPrepared Build() &&;

private:
    friend struct NAlice::NWonderlogs::NTestSuiteUniproxy::TTestCaseDisjunctionDoNotUseUserLogsFalseFirst;
    friend struct NAlice::NWonderlogs::NTestSuiteUniproxy::TTestCaseDisjunctionDoNotUseUserLogsTrueFirst;

    TUniproxyPrepared::TError GenerateError(TUniproxyPrepared::TError::EReason reason, const TString& message);

    TErrors SetUuid(const TString& uuid);
    TErrors SetMessageId(const TString& messageId);
    TErrors SetMegamindRequestId(const TString& messageId);
    TErrors SetConnectSessionId(const TString& connectSessionId);
    TErrors SetEnvironment(const TUniproxyPrepared::TEnvironment& environment);
    TErrors SetClientIp(const TString& clientIp);
    TErrors SetDoNotUseUserLogs(const bool doNotUseUserLogs);

    template <typename T>
    void SetTimestampLogMs(const T& value) {
        if (!UniproxyPrepared.HasTimestampLogMs() && value.HasTimestampLogMs()) {
            UniproxyPrepared.SetTimestampLogMs(value.GetTimestampLogMs());
        }
    }

    void Append(TErrors& errorsTo, TErrors&& errorsFrom);

    template <typename T>
    TErrors InitializeCommonFields(const T& value) {
        TErrors errors;
        Append(errors, SetUuid(value.GetUuid()));
        Append(errors, SetMessageId(value.GetMessageId()));
        SetTimestampLogMs(value);
        return errors;
    }

    TUniproxyPrepared UniproxyPrepared;
};

} // namespace NAlice::NWonderlogs
