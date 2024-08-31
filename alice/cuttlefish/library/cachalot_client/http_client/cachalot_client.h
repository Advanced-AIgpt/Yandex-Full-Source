#pragma once

#include <alice/cachalot/api/protos/cachalot.pb.h>

#include <apphost/lib/transport/transport_neh_backend.h>

#include <library/cpp/neh/http2.h>
#include <library/cpp/neh/http_common.h>

#include <util/generic/string.h>

struct THttpClientAnswer {
    bool Ok;
    TString ErrorText;
    int NumDeleted = 0;
};

class THttpCachalotClient {
public:
    THttpCachalotClient(TString url, int retries=3)
        : Url(url)
        , NumRetries(retries)
    {
    }

private:
    NAppHost::NTransport::TResponsePtr Request(const TVector<TString>& headers, TString body, TString contentType, NNeh::NHttp::ERequestType method) {
        TString headersStr;
        for (const auto& header : headers) {
            headersStr += header + "\r\n";
        }
        int attempt = 1;
        NAppHost::NTransport::TResponsePtr resp;
        while (attempt <= NumRetries) {
            NNeh::TMessage msg { Url, {} };
            NNeh::NHttp::MakeFullRequest(msg, headersStr, body, contentType, method);
            NAppHost::NTransport::TResponseHandle fut = Communicator.SendRequest(msg)->GetFuture();
            resp = fut.GetValueSync();
            if (resp->GetErrorCode() / 100 != 5 || attempt == NumRetries) {
                Cerr << "Ok: " << resp->GetErrorCode() << " " << resp->GetErrorMessage() << Endl;
                return resp;
            }
            Cerr << "retry: " << resp->GetErrorCode() << " " << resp->GetErrorMessage() << Endl;
            attempt += 1;
        }
        return resp;
    }

public:
    THttpClientAnswer SendYabioContextDeleteRequest(TString groupId) {
        NCachalotProtocol::TRequest request;
        auto a = request.MutableYabioContextReq();
        auto b = a->MutableDelete();
        auto key = b->MutableKey();
        key->SetGroupId(groupId);
        // want to delete all user's rows
        key->SetDevModel("");
        key->SetDevManuf("");

        TVector<TString> headers;
        TString body;
        Y_PROTOBUF_SUPPRESS_NODISCARD request.SerializeToString(&body);
        TString contentType = "application/protobuf";
        NNeh::NHttp::ERequestType method = NNeh::NHttp::ERequestType::Post;

        auto response = Request(headers, body, contentType, method);
        if (!response) {
            return THttpClientAnswer{0, "EMPTY"};
        }

        if ((response->GetErrorCode()) != 0 && (response->GetErrorCode() != 200)) {
            return THttpClientAnswer{0, "Request to YabioContext failed: " + ToString(response->GetErrorCode()) + " " + response->GetErrorMessage()};
        }
        NCachalotProtocol::TResponse resp;
        Y_PROTOBUF_SUPPRESS_NODISCARD resp.ParseFromString(ToString(response->GetData()));
        if (resp.HasYabioContextResp() && resp.GetYabioContextResp().HasSuccess()) {
            Cerr << "Success: " << resp.GetYabioContextResp().GetSuccess().GetOk() << Endl;
            return THttpClientAnswer{resp.GetYabioContextResp().GetSuccess().GetOk(), ""};
        } else if (resp.HasYabioContextResp() && resp.GetYabioContextResp().HasError()) {
            return THttpClientAnswer{0, resp.GetYabioContextResp().GetError().GetText()};
        }

        return THttpClientAnswer{0, "UNKNOWN ERROR"};
    }

    THttpClientAnswer SendVinsContextDeleteRequest(TString puid) {
        NCachalotProtocol::TRequest request;
        auto a = request.MutableVinsContextReq();
        auto b = a->MutableDelete();
        auto key = b->MutableKey();
        key->SetPuid(puid);

        TVector<TString> headers;
        TString body;
        Y_PROTOBUF_SUPPRESS_NODISCARD request.SerializeToString(&body);
        TString contentType = "application/protobuf";
        NNeh::NHttp::ERequestType method = NNeh::NHttp::ERequestType::Post;

        auto response = Request(headers, body, contentType, method);
        if (!response) {
            return THttpClientAnswer{0, "EMPTY"};
        }

        if ((response->GetErrorCode()) != 0 && (response->GetErrorCode() != 200)) {
            return THttpClientAnswer{0, "Request to VinsContext failed: " + ToString(response->GetErrorCode()) + " " + response->GetErrorMessage()};
        }
        NCachalotProtocol::TResponse resp;
        Y_PROTOBUF_SUPPRESS_NODISCARD resp.ParseFromString(ToString(response->GetData()));
        if (resp.HasVinsContextResp() && resp.GetVinsContextResp().HasSuccess()) {
            Cerr << "Success: " << resp.GetVinsContextResp().GetSuccess().GetOk() << Endl;
            return THttpClientAnswer{resp.GetVinsContextResp().GetSuccess().GetOk(), "", resp.GetVinsContextResp().GetSuccess().GetNumDeleted()};
        } else if (resp.HasVinsContextResp() && resp.GetVinsContextResp().HasError()) {
            return THttpClientAnswer{0, resp.GetVinsContextResp().GetError().GetText()};
        }

        return THttpClientAnswer{0, "UNKNOWN ERROR"};
    }

private:
    NAppHost::NTransport::TNehCommunicationSystem Communicator;
    TString Url;
    int NumRetries = 3;
};
