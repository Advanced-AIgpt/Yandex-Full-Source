#include "base_client.h"
#include <alice/uniproxy/mapper/library/logging/log.h>

#include <contrib/libs/poco/Foundation/include/Poco/Buffer.h>
#include <contrib/libs/poco/Net/include/Poco/Net/HTTPClientSession.h>
#include <contrib/libs/poco/Net/include/Poco/Net/HTTPRequest.h>
#include <contrib/libs/poco/Net/include/Poco/Net/HTTPResponse.h>
#include <contrib/libs/poco/NetSSL_OpenSSL/include/Poco/Net/Context.h>
#include <contrib/libs/poco/NetSSL_OpenSSL/include/Poco/Net/HTTPSClientSession.h>
#include <contrib/libs/poco/Net/include/Poco/Net/NetException.h>
#include <contrib/libs/poco/Net/include/Poco/Net/WebSocket.h>

#include <library/cpp/uri/uri.h>
#include <util/generic/yexception.h>

namespace NAlice::NUniproxy {
    using namespace Poco;
    using namespace Poco::Net;
    using namespace NUri;
    using namespace std;

    namespace {
        string ToStdString(TStringBuf s) {
            return string(s.data(), s.data() + s.size());
        }

        unique_ptr<HTTPClientSession> CreateSession(TUri const& uri, bool disableServerCertificateValidation) {
            switch (uri.GetScheme()) {
                case TScheme::EKind::SchemeWS:
                    return unique_ptr<HTTPClientSession>(new HTTPClientSession(ToStdString(uri.GetHost()), uri.GetPort()));
                case TScheme::EKind::SchemeWSS: {
                    Context::Params contextParams{};
                    if (disableServerCertificateValidation) {
                        contextParams.verificationMode = Context::VerificationMode::VERIFY_NONE;
                    }
                    Context::Ptr contextPtr(new Context(Context::Usage::CLIENT_USE, contextParams));
                    return unique_ptr<HTTPClientSession>(new HTTPSClientSession(ToStdString(uri.GetHost()), uri.GetPort(), contextPtr));
                }
                default:
                    ythrow yexception() << "Invalid uniproxy URI scheme. Expected WS or WSS";
            }
        }

        TBaseClient::EResultType ResultTypeFromFlags(int flags) {
            if ((flags & WebSocket::FrameFlags::FRAME_FLAG_FIN) != WebSocket::FrameFlags::FRAME_FLAG_FIN) {
                ythrow yexception() << "Partial frames are not supported";
            }
            auto const type = flags & WebSocket::FrameOpcodes::FRAME_OP_BITMASK;
            switch (type) {
                case WebSocket::FrameOpcodes::FRAME_OP_TEXT:
                    return TBaseClient::EResultType::Text;
                case WebSocket::FrameOpcodes::FRAME_OP_BINARY:
                    return TBaseClient::EResultType::Binary;
            }
            ythrow yexception() << "Unsupported frame type " << type;
        }

    }

    TBaseClient::TBaseClient(TStringBuf uniproxyUri, const TBaseClientParams& params)
        : Logger(params.Logger)
        , FlagsContainer(params.FlagsContainer)
        , HttpSession()
        , Params(params)
    {
        Y_ENSURE(!FlagsContainer || FlagsContainer->Has(DUMMY_FLAG));
        TUri uri;
        Y_ENSURE(uri.ParseAbs(uniproxyUri, TUri::FeaturesRecommended) == NUri::TState::ParsedOK, "Incorrect uniproxy URL");
        LOG_INFO("Host %s, port: %d\r\n", uri.GetHost().data(), uri.GetPort());
        HttpSession = CreateSession(uri, params.DisableServerCertificateValidation);
        HttpSession->setTimeout(Timespan(params.ConnectTimeout.MicroSeconds()), Timespan(params.SendTimeout.MicroSeconds()), Timespan(params.ReceiveTimeout.MicroSeconds()));
        HttpSession->setKeepAlive(true);
        auto const path = uri.PrintS(TField::EFlags::FlagPath | TField::EFlags::FlagQuery);
        HTTPRequest request(HTTPRequest::HTTP_GET, ToStdString(path), HTTPMessage::HTTP_1_1);
        HTTPResponse response;
        for (const auto& [header, value] : params.SessionHeaders) {
            request.add(header, value);
        }

        LOG_INFO("Connecting to %s\r\n", uniproxyUri.data());
        WebSocket.reset(new Poco::Net::WebSocket(*HttpSession, request, response));
        LOG_INFO("Response: %d %s\r\n", response.getStatus(), response.getReason().c_str());
    }

    TBaseClient::~TBaseClient() {
        try {
            WebSocket->shutdown();
        } catch (const Poco::Net::NetException& e) {
            LOG_INFO("Error while closing websocket %s:%s", e.name(), e.className());
        } catch (const std::exception& e) {
            LOG_INFO("Unknown error occured while closing websocket %s", e.what());
        }
    }

    void TBaseClient::SendText(TStringBuf text) {
        WebSocket->sendFrame(text.data(), text.size(), WebSocket::SendFlags::FRAME_TEXT);
    }

    void TBaseClient::SendBinary(TArrayRef<char> data) {
        WebSocket->sendFrame(data.data(), data.size(), WebSocket::SendFlags::FRAME_BINARY);
    }

    TBaseClient::TReceiveResult TBaseClient::Receive(Buffer<char>& buffer) {
        while (true) {
            int flags = 0;
            size_t bytes = 0;
            const auto availBytes = WebSocket->available();
            if (availBytes > 0) {
                bytes = WebSocket->receiveFrame(buffer, flags);
            } else {
                const auto poll = WebSocket->poll(Timespan(Params.ReceiveTimeout.MicroSeconds()),
                                                  Poco::Net::WebSocket::SelectMode::SELECT_READ);
                if (poll) {
                    bytes = WebSocket->receiveFrame(buffer, flags);
                } else {
                    LOG_INFO("Timeout receiving frame from socket\n");
                }
            }
            auto check = flags & Poco::Net::WebSocket::FrameOpcodes::FRAME_OP_BITMASK;
            if (check == Poco::Net::WebSocket::FrameOpcodes::FRAME_OP_PING) {
                WebSocket->sendFrame(buffer.begin(), bytes,
                                     Poco::Net::WebSocket::FRAME_OP_PONG | Poco::Net::WebSocket::FRAME_FLAG_FIN);
                continue;
            }
            if (bytes == 0) {
                return TReceiveResult{0, EResultType::None};
            }
            return TReceiveResult{bytes, ResultTypeFromFlags(flags)};
        }
    }

    TString TBaseClient::GetRemoteAddress() const {
        auto const addr = WebSocket->peerAddress().host().toString();
        return TString(addr.data(), addr.size());
    }

    ui16 TBaseClient::GetRemotePort() const {
        return WebSocket->peerAddress().port();
    }

}
