#include "music_match_client.h"
#include "test_functions.h"

using namespace NAlice::NMusicMatchAdapter;

class TMockMusicMatchClient : public TMusicMatchClient {
public:
    // TODO(chegoryu) add better default body for callbacks
    void OnInitResponse(const NAlice::NMusicMatch::NProtobuf::TInitResponse& /* initResponse */) override {
    }
    void OnStreamResponse(const NAlice::NMusicMatch::NProtobuf::TStreamResponse& /* streamResponse */) override {
    }
    void OnClosed() override {
    }
    void OnAnyError(const TString& /* error */) override {
    }

    bool IsRequestUpgraded() {
        return RequestUpgraded_;
    }
};


Y_UNIT_TEST_SUITE(MusicMatchClientSuite) {

// TODO(chegoryu): Write normal tests
// Now only test test_functions
Y_UNIT_TEST(TestSimpleNotMusic) {
    constexpr size_t ITER = 5;

    class TTestMusicMatchClient : public TMockMusicMatchClient {
    public:
        void OnInitResponse(const NAlice::NMusicMatch::NProtobuf::TInitResponse& initResponse) override {
            // TODO(chegoryu) check that answer is unique
            InitResponse_ = initResponse;
        }
        void OnStreamResponse(const NAlice::NMusicMatch::NProtobuf::TStreamResponse& streamResponse) override {
            // TODO(chegoryu) check that answer is unique
            StreamResponse_ = streamResponse;
        }

    public:
        TMaybe<NAlice::NMusicMatch::NProtobuf::TInitResponse> InitResponse_ = Nothing();
        TMaybe<NAlice::NMusicMatch::NProtobuf::TStreamResponse> StreamResponse_ = Nothing();
    };

    class TTestMusicMatchProxyHandler : public NTestLib::TMusicMatchFakeProxyHandler {
    public:
        using NTestLib::TMusicMatchFakeProxyHandler::TMusicMatchFakeProxyHandler;

        bool CheckSendNotMusic(EMessageType lastMessageType) override {
            Y_UNUSED(lastMessageType);
            return BinaryMessages_.size() >= ITER;
        }
    };

    class TTestMusicMatchProxyServer : public NTestLib::TMusicMatchFakeProxyServer<TTestMusicMatchProxyHandler> {
    public:
        using NTestLib::TMusicMatchFakeProxyServer<TTestMusicMatchProxyHandler>::TMusicMatchFakeProxyServer;

        void Test() override {
            NTestLib::THttpClientRunner clientRunner;

            TIntrusivePtr<TTestMusicMatchClient> musicMatchClient(new TTestMusicMatchClient());
            clientRunner.HttpClient_.RequestWebSocket(GetWsPath(), NVoicetech::TWsHandlerRef(musicMatchClient.Get()));

            UNIT_ASSERT(
                NTestLib::TryWait(TDuration::Seconds(1), 30, [&musicMatchClient]() {
                    return musicMatchClient->IsRequestUpgraded();
                })
            );

            for (size_t i = 0; i < ITER; ++i) {
                musicMatchClient->SendBinaryMessage("data");
            }

            UNIT_ASSERT(clientRunner.TryWaitAsioClients());
            UNIT_ASSERT(TryWaitNoServerWsHandlerAsioClients());

            UNIT_ASSERT_C(musicMatchClient->InitResponse_.Defined(), "InitResponse not defined");
            UNIT_ASSERT_C(musicMatchClient->InitResponse_->GetIsOk(), musicMatchClient->InitResponse_->GetErrorMessage());

            UNIT_ASSERT_C(musicMatchClient->StreamResponse_.Defined(), "StreamResponse not defined");
            UNIT_ASSERT_C(musicMatchClient->StreamResponse_->GetMusicResult().GetIsOk(), musicMatchClient->StreamResponse_->GetMusicResult().GetErrorMessage());
            UNIT_ASSERT_VALUES_EQUAL(musicMatchClient->StreamResponse_->GetMusicResult().GetRawMusicResultJson(), R"({"result":"not-music"})");
            UNIT_ASSERT_C(musicMatchClient->StreamResponse_->GetMusicResult().GetIsFinish(), "IsFinish is false");

            // TODO(chegoryu) add more asserts to music match proxy handler
            ServerWsHandler_->CheckNoErrors();
        }
    };

    NTestLib::RunTestWithMusicMatchProxyServer<TTestMusicMatchProxyServer>();
}

}
