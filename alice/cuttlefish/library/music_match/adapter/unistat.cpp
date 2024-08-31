#include "unistat.h"

using namespace NAlice::NMusicMatchAdapter;

namespace {

    ::NUnistat::IHolePtr CreateCounter(const TString& name, const TString& suffix = "summ") {
        return TUnistat::Instance().DrillFloatHole(
            name,
            suffix,
            ::NUnistat::TPriority(0),
            ::NUnistat::TStartValue(0));
    }

}  // namespace


TUnistatN::TUnistatN() {
    CreateMusicMatchClient_ = CreateCounter("music_match_client_create");
    MusicMatchClientCounter_ = CreateCounter("music_match_client_counter", "ammx");

    CreateMusicMatchAudioConverter_ = CreateCounter("music_match_audio_converter_create");
    MusicMatchAudioConverterCounter_ = CreateCounter("music_match_audio_converter_counter", "ammx");
    MusicMatchAudioConverterError_ = CreateCounter("music_match_audio_converter_error");

    MusicMatchReceivedAudioDataSize_ = CreateCounter("music_match_received_audio_data_size");
    MusicMatchConvertedAudioDataSize_ = CreateCounter("music_match_converted_audio_data_size");
    MusicMatchSendedAudioDataSize_ = CreateCounter("music_match_sended_audio_data_size");

    MusicMatchRequestSuccess_ = CreateCounter("music_match_request_success");
    MusicMatchRequestError_ = CreateCounter("music_match_request_error");
    MusicMatchRequestCancel_ = CreateCounter("music_match_request_cancel");

    MusicMatchFinalResultAll_ = CreateCounter("music_match_final_result_all");
    MusicMatchClassifyingNotMusicResult_ = CreateCounter("music_match_classifying_not_music_result");
    MusicMatchClassifyingMusicResult_ = CreateCounter("music_match_classifying_music_result");
    MusicMatchResultAll_ = CreateCounter("music_match_result_all");

    MusicMatchWebsocketAnswerParseError_ = CreateCounter("music_match_websocket_answer_parse_error");
    MusicMatchWebsocketUpgradeResponseError_ = CreateCounter("music_match_websocket_upgrade_response_error");
    MusicMatchWebsocketCloseError_ = CreateCounter("music_match_websocket_close_error");
    MusicMatchWebsocketNetworkError_ = CreateCounter("music_match_websocket_network_error");
    MusicMatchWebsocketWsError_ = CreateCounter("music_match_websocket_ws_error");
    MusicMatchWebsocketTypedError_ = CreateCounter("music_match_websocket_typed_error");
    MusicMatchWebsocketErrorAll_ = CreateCounter("music_match_websocket_error_all");
}

TUnistatCounterGuard TUnistatN::MusicMatchClientCounter() {
    return TUnistatCounterGuard(*MusicMatchClientCounter_);
}
void TUnistatN::OnCreateMusicMatchClient() {
    CreateMusicMatchClient_->PushSignal(1.);
}

TUnistatCounterGuard TUnistatN::MusicMatchAudioConverterCounter() {
    return TUnistatCounterGuard(*MusicMatchAudioConverterCounter_);
}
void TUnistatN::OnMusicMatchCreateAudioConverter() {
    CreateMusicMatchAudioConverter_->PushSignal(1.);
}
void TUnistatN::OnMusicMatchAudioConverterError() {
    MusicMatchAudioConverterError_->PushSignal(1.);
}

void TUnistatN::OnMusicMatchReceivedAudioData(size_t size) {
    MusicMatchReceivedAudioDataSize_->PushSignal(size);
}
void TUnistatN::OnMusicMatchConvertedAudioData(size_t size) {
    MusicMatchConvertedAudioDataSize_->PushSignal(size);
}
void TUnistatN::OnMusicMatchSendedAudioData(size_t size) {
    MusicMatchSendedAudioDataSize_->PushSignal(size);
}

void TUnistatN::OnMusicMatchRequestSuccess() {
    MusicMatchRequestSuccess_->PushSignal(1.);
}

void TUnistatN::OnMusicMatchRequestError() {
    MusicMatchRequestError_->PushSignal(1.);
}

void TUnistatN::OnMusicMatchRequestCancel() {
    MusicMatchRequestCancel_->PushSignal(1.);
}

void TUnistatN::OnMusicMatchFinalResult() {
    MusicMatchFinalResultAll_->PushSignal(1.);
    MusicMatchResultAll_->PushSignal(1.);
}
void TUnistatN::OnMusicMatchClassifyingNotMusicResult() {
    MusicMatchClassifyingNotMusicResult_->PushSignal(1.);
    MusicMatchFinalResultAll_->PushSignal(1.);
    MusicMatchResultAll_->PushSignal(1.);
}
void TUnistatN::OnMusicMatchClassifyingMusicResult() {
    MusicMatchClassifyingMusicResult_->PushSignal(1.);
    MusicMatchResultAll_->PushSignal(1.);
}

void TUnistatN::OnMusicMatchWebsocketAnswerParseError() {
    MusicMatchWebsocketAnswerParseError_->PushSignal(1.);
    MusicMatchWebsocketErrorAll_->PushSignal(1.);
}
void TUnistatN::OnMusicMatchWebsocketUpgradeResponseError() {
    MusicMatchWebsocketUpgradeResponseError_->PushSignal(1.);
    MusicMatchWebsocketErrorAll_->PushSignal(1.);
}
void TUnistatN::OnMusicMatchWebsocketCloseError() {
    MusicMatchWebsocketCloseError_->PushSignal(1.);
    MusicMatchWebsocketErrorAll_->PushSignal(1.);
}
void TUnistatN::OnMusicMatchWebsocketNetworkError() {
    MusicMatchWebsocketNetworkError_->PushSignal(1.);
    MusicMatchWebsocketErrorAll_->PushSignal(1.);
}
void TUnistatN::OnMusicMatchWebsocketWsError() {
    MusicMatchWebsocketWsError_->PushSignal(1.);
    MusicMatchWebsocketErrorAll_->PushSignal(1.);
}
void TUnistatN::OnMusicMatchWebsocketTypedError() {
    MusicMatchWebsocketTypedError_->PushSignal(1.);
    MusicMatchWebsocketErrorAll_->PushSignal(1.);
}
