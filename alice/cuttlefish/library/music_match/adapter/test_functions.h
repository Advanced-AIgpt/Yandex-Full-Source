#pragma once

#include <voicetech/library/asconv/threaded_converter.h>
#include <voicetech/library/messages/message.h>
#include <voicetech/library/ws_server/error.h>
#include <voicetech/library/ws_server/server.h>
#include <voicetech/library/ws_server/ws_server.h>

#include <library/cpp/neh/asio/executor.h>
#include <library/cpp/testing/common/network.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ptr.h>

namespace NAlice::NMusicMatchAdapter::NTestLib {
    class TMusicMatchFakeProxyHandler : public NVoicetech::TWsHandler {
    public:
        enum EMessageType {
            TEXT = 1,
            BINARY = 2,
        };

    public:
        TMusicMatchFakeProxyHandler(NVoicetech::THttpServer& httpServer)
            : HttpServer_(httpServer)
        {
        }

        NVoicetech::THttpServer& GetHttpServer() {
            return HttpServer_;
        }

        void CheckNoErrors();
        TString BuildFullErrorMessage() const;
        TString GetErrorMessage(const TString& mainError, const TString& fullError) const;
        virtual void ResetSomeErrors() {
            // Override this function to clean up the errors that should occur in the test
            // The function will be called after the calling BuildFullErrorMessage
            // So if an unexpected error occurs, all errors will be in the output,
            // not just the ones left by this function
        }

    protected:
        // It is guaranteed that at the time of the call last message is
        // TextMessages_.back() or BinaryMessages_.back()
        virtual bool CheckSendClassifying(EMessageType lastMessageType) {
            Y_UNUSED(lastMessageType);
            return false;
        }
        virtual bool CheckSendNotMusic(EMessageType lastMessageType) {
            Y_UNUSED(lastMessageType);
            return false;
        }
        virtual void SendCustomAnswer(EMessageType lastMessageType) {
            Y_UNUSED(lastMessageType);
        }

        void SafeSendTextMessage(const TString& message, bool sendCloseMessage);
        void SendCloseMessageIfNotSended();

    private:
        // WebSocket handlers
        void OnUpgradeRequest(const THttpParser& httpParser, int httpCode, const TString& error) override;
        void OnTextMessage(const char* data, size_t size) override;
        void OnBinaryMessage(const void* data, size_t size) override;
        void OnCloseMessage(ui16 code, const TString& reason) override;
        void OnError(const NVoicetech::TNetworkError& error) override;
        void OnError(const NRfc6455::TWsError& error) override;
        void OnError(const NVoicetech::TTypedError& error) override;

        void OnMessage(EMessageType lastMessageType);

    protected:
        bool RequestUpgraded_ = false;
        bool HasCloseMessage_ = false;
        bool CloseMessageSended_ = false;

        TVector<TString> TextMessages_;
        TVector<TString> BinaryMessages_;

        TMaybe<TString> UpgradeRequestError_ = Nothing();
        TMaybe<TString> CloseMessageError_ = Nothing();
        TMaybe<TString> NetworkError_ = Nothing();
        TMaybe<TString> WsError_ = Nothing();
        TMaybe<TString> TypedError_ = Nothing();

        TMaybe<TString> CustomError_ = Nothing();

    private:
        NVoicetech::THttpServer& HttpServer_;
    };

    template<typename TMusicMatchProxyHandler>
    class TMusicMatchFakeProxyServer : public NVoicetech::TServer {
        static_assert(std::is_base_of<TMusicMatchFakeProxyHandler, TMusicMatchProxyHandler>::value, "TMusicMatchProxyHandler must derive from TMusicMatchFakeProxyHandler");

    public:
        TMusicMatchFakeProxyServer(const NVoicetech::TWsServerConfig& cfg)
            : TServer(cfg)
        {}

    public:
        virtual void Test() = 0;

    protected:
        bool TryWaitNoServerWsHandlerAsioClients(const TDuration& period = TDuration::Seconds(1), size_t tries = 30) {
            return TryWait(period, tries, [this]() {
                return !(ServerWsHandler_ && ServerWsHandler_->GetHttpServer().AsioClients.Val());
            });
        }

        TString GetWsPath() {
            return TStringBuilder() << "ws://localhost:" << Config.Http().Port() << "/" << DEFAULT_PATH;
        }

    protected:
        struct TRequestKeepaliver: public TThrRefBase {
            TRequestKeepaliver(NVoicetech::TIOService& svc, NVoicetech::THttpServer::TRequestRef req)
                : Timer_(svc)
                , Request_(req)
            {}

            void KeepAlive() {
                TIntrusivePtr<TRequestKeepaliver> self(this);
                Timer_.AsyncWaitExpireAt(
                    TDuration::MilliSeconds(10),
                    [self, this](const NVoicetech::TErrorCode& err, NVoicetech::IHandlingContext&) {
                        if (Request_->Canceled() || err) {
                            return;
                        }

                        KeepAlive();
                    }
                );
            }

            NVoicetech::TDeadlineTimer Timer_;
            NVoicetech::THttpServer::TRequestRef Request_;
        };

    private:
        void OnRequest(NVoicetech::THttpServer::TRequestRef req) override {
            if (req->Service() == DEFAULT_PATH) {
                ServerWsHandler_.Reset(new TMusicMatchProxyHandler(req->HttpServer));
                NVoicetech::TWsHandlerRef handler(ServerWsHandler_.Get());
                NVoicetech::NWebSocket::HandleWsRequest(*req, handler, Config.WebSocket());
            } else {
                TServer::OnRequest(req);
            }
        }

        void WaitShutdown(NVoicetech::THttpServer& /* httpServer */) override {
            Test();

            UNIT_ASSERT_C(TryWaitNoServerWsHandlerAsioClients(), "Some http server asio threads are running at the end of server shutdown");
        }

    public:
        static inline TString DEFAULT_PATH = "music_match";

        TIntrusivePtr<TMusicMatchProxyHandler> ServerWsHandler_;
    };

    class THttpClientRunner {
    public:
        THttpClientRunner(const NVoicetech::THttpClientConfig& config = {}, size_t executors = 1)
            : AsioPool_(executors)
            , HttpClientConfig_(config)
            , HttpClient_(HttpClientConfig_, AsioPool_.GetExecutor().GetIOService())
        {}

        bool TryWaitAsioClients(const TDuration& period = TDuration::Seconds(1), size_t tries = 30);

    public:
        NAsio::TExecutorsPool AsioPool_;
        NVoicetech::THttpClientConfig HttpClientConfig_;
        NVoicetech::THttpClient HttpClient_;
    };

    template<typename TMusicMatchProxyServer>
    void RunTestWithMusicMatchProxyServer() {
        const NTesting::TPortHolder port = NTesting::GetFreePort();
        NVoicetech::TWsServerConfig config;
        config.Http().SetPort(static_cast<uint16_t>(port));
        TMusicMatchProxyServer server(config);
        server.Run();
        // test executed in TMusicMatchProxyServer::WaitShutdown()
    };

    bool TryWait(const TDuration& period, size_t tries, std::function<bool()>&& breakHook);
}
