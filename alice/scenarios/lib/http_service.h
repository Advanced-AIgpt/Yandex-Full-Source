#pragma once

#include <alice/library/logger/logger.h>
#include <alice/library/logger/proto/config.pb.h>
#include <alice/library/network/common.h>

#include <kernel/server/protos/serverconf.pb.h>
#include <kernel/server/server.h>

#include <library/cpp/http/misc/httpcodes.h>

#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/protobuf/json/proto2json.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/string/ascii.h>

#include <functional>

namespace NAlice {

class TProtoBasedHttpRequest final : public NServer::TRequest {
public:
    using THandler = std::function<bool(TBlob&, const TString&, THttpResponse&, TRTLogger& logger)>;

    TProtoBasedHttpRequest(NServer::TServer& server, const THashMap<TString, THandler>& handlers, TRTLogClient& logClient)
        : NServer::TRequest{server}
        , Handlers(handlers)
        , LogClient(logClient)
    {
    }

    bool DoReply(const TString& path, THttpResponse& response) override {
        if (const auto* handler = Handlers.get().FindPtr(path)) {
            auto logger = LogClient.get().CreateLogger(GetRTLogToken(), false /* session */);
            return (*handler)(Buf, GetContentType(), response, logger);
        }

        return false;
    }

private:
    TString GetRTLogToken() const {
        return GetHeader("X-RTLog-Token");
    }

    TString GetContentType() const {
        return GetHeader("Content-Type");
    }

    TString GetHeader(const TString& headerName) const {
        for (const auto& h: ParsedHeaders) {
            if (AsciiEqualsIgnoreCase(h.first, TStringBuf(headerName))) {
                return h.second;
            }
        }
        return "";
    }

private:
    std::reference_wrapper<const THashMap<TString, THandler>> Handlers;
    std::reference_wrapper<TRTLogClient> LogClient;
};


class TProtoBasedHttpService : public NServer::TServer {
public:
    TProtoBasedHttpService(const NServer::THttpServerConfig& serverConfig, TRTLogClient& logClient)
        : NServer::TServer{serverConfig}
        , LogClient{logClient}
    {
    }

    template <typename TRequestProto, typename TResponseProto>
    void RegisterHandler(const TString& path, std::function<TResponseProto(const TRequestProto&, TRTLogger&)> handler) {
        Handlers[path] = [this, handler=std::move(handler)](TBlob& buf, const TString& contentType, THttpResponse& response, TRTLogger& logger){
            return InvokeHandler<TRequestProto, TResponseProto>(handler, buf, contentType, response, logger);
        };
    }

    TClientRequest* CreateClient() override {
        return new TProtoBasedHttpRequest(*this, Handlers, LogClient);
    }

private:
    inline const static TString SUPPORTED_TYPES_MSG = TStringBuilder() << "Supported types for requests are: "
                                                                       << NContentTypes::APPLICATION_PROTOBUF
                                                                       << ", "
                                                                       << NContentTypes::APPLICATION_JSON
                                                                       << ".";

    template <typename TRequestProto, typename TResponseProto>
    bool InvokeHandler(std::function<TResponseProto(const TRequestProto&, TRTLogger&)> handler, TBlob& buf, const TString& contentType, THttpResponse& response, TRTLogger& logger) const {
        TMaybe<TRequestProto> requestProto = ParseRequest<TRequestProto>(buf, contentType, response);
        if (!requestProto.Defined()) {
            return true;
        }

        try {
            const TResponseProto result = handler(*requestProto, logger);
            SerializeResponse(response, contentType, result);
        } catch (yexception ex) {
            response.SetContent(TStringBuilder() << "Exception occured: " << ex.what());
            response.SetHttpCode(HttpCodes::HTTP_INTERNAL_SERVER_ERROR);
        }

        return true;
    }

    template <typename TProto>
    TMaybe<TProto> ParseRequest(const TBlob& buf, const TString& contentType, THttpResponse& response) const {
        TMaybe<TProto> maybeRequestProto;
        if (contentType == NContentTypes::APPLICATION_PROTOBUF) {
            maybeRequestProto = ParseProto<TProto>(buf);
        } else if (contentType == NContentTypes::APPLICATION_JSON) {
            maybeRequestProto = ParseJson<TProto>(buf);
        } else {
            response.SetContent(TStringBuilder() << "Unsupported data type. " << SUPPORTED_TYPES_MSG);
            response.SetHttpCode(HttpCodes::HTTP_UNSUPPORTED_MEDIA_TYPE);
            return Nothing();
        }
        if (!maybeRequestProto.Defined()) {
            response.SetContent("Could not parse request. Format invalid or required fields missing or unknown fields present");
            response.SetHttpCode(HttpCodes::HTTP_BAD_REQUEST);
            return Nothing();
        }
        return maybeRequestProto;
    }

    template <typename TProto>
    TMaybe<TProto> ParseProto(const TBlob& buf) const {
        TProto proto;
        if (!proto.ParseFromArray(buf.Data(), buf.Size())) {
            return Nothing();
        }
        int sizeBeforeDiscardUnknown = proto.ByteSize();
        proto.DiscardUnknownFields();
        if (proto.ByteSize() != sizeBeforeDiscardUnknown) {
            return Nothing();
        }
        return proto;
    }

    template <typename T>
    TMaybe<T> ParseJson(const TBlob& buf) const {
        try {
            return NProtobufJson::Json2Proto<T>(TString(buf.AsCharPtr(), buf.Size()), {.AllowUnknownFields = false});
        } catch (const yexception& e) {
            return Nothing();
        }
    }

    template <typename TResponseProto>
    void SerializeResponse(THttpResponse& response, const TString& contentType, const TResponseProto& result) const {
        if (contentType == NContentTypes::APPLICATION_PROTOBUF) {
            response.SetContent(result.SerializeAsString());
        } else if (contentType == NContentTypes::APPLICATION_JSON) {
            response.SetContent(NProtobufJson::Proto2Json(result));
        } else {
            response.SetContent(TStringBuilder() << "Unsupported accepted data type. " << SUPPORTED_TYPES_MSG);
            response.SetHttpCode(HttpCodes::HTTP_UNSUPPORTED_MEDIA_TYPE);
            return;
        }
        response.SetContentType(contentType);
    }

private:
    THashMap<TString, TProtoBasedHttpRequest::THandler> Handlers;
    TRTLogClient& LogClient;
};

} // namespace NAlice
