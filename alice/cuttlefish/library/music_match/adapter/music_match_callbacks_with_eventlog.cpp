#include "music_match_callbacks_with_eventlog.h"

using namespace NAlice::NMusicMatch;
using namespace NAlice::NMusicMatchAdapter;

TMusicMatchCallbacksWithEventlog::TMusicMatchCallbacksWithEventlog(NAppHost::TServiceContextPtr& ahContext, TAtomicBase requestNumber)
    : NMusicMatch::TCallbacksHandler(ahContext)
    , LogFrame_(NCuttlefish::SpawnLogFrame())
    , RequestNumber_(requestNumber)
{
    LogFrame_->LogEvent(NEvClass::MusicMatchCallbacksFrame(RequestNumber_));
}

void TMusicMatchCallbacksWithEventlog::OnAnyError(const TString& error) {
    LogFrame_->LogEvent(NEvClass::ErrorMessage(error));

    NMusicMatch::TCallbacksHandler::OnAnyError(error);
}

void TMusicMatchCallbacksWithEventlog::AddInitResponseAndFlush(const NMusicMatch::NProtobuf::TInitResponse& initResponse, bool isFinalResponse) {
    LogFrame_->LogEvent(NEvClass::SendToAppHostMusicMatchInitResponse(initResponse.ShortUtf8DebugString()));

    NMusicMatch::TCallbacksHandler::AddInitResponseAndFlush(initResponse, isFinalResponse);
}

void TMusicMatchCallbacksWithEventlog::AddStreamResponseAndFlush(const NMusicMatch::NProtobuf::TStreamResponse& streamResponse, bool isFinalResponse) {
    LogFrame_->LogEvent(NEvClass::SendToAppHostMusicMatchStreamResponse(streamResponse.ShortUtf8DebugString()));

    NMusicMatch::TCallbacksHandler::AddStreamResponseAndFlush(streamResponse, isFinalResponse);
}

void TMusicMatchCallbacksWithEventlog::FlushAppHostContext(bool isFinalFlush) {
    NMusicMatch::TCallbacksHandler::FlushAppHostContext(isFinalFlush);

    if (isFinalFlush) {
        LogFrame_->LogEvent(NEvClass::FlushAppHostContext());
    }
}
