#pragma once

#include <alice/hollywood/library/base_hw_service/base_hw_service_handle.h>
#include <alice/hollywood/library/rpc_service/rpc_request.h>
#include <alice/hollywood/library/rpc_service/rpc_response.h>

#include <alice/library/client/protos/client_info.pb.h>
#include <alice/library/network/headers.h>
#include <alice/library/util/search_convert.h>
#include <alice/protos/data/search_result/tv_search_result.pb.h>

#include <util/system/hostname.h>

namespace NAlice::NHollywood::NVideo {

namespace {

inline const TString GET_TV_SEARCH_RESULT_HANDLER_NAME = "rpc_handler_get_tv_search_result_rpc";
inline const TString GET_TV_SEARCH_RESULT_HANDLER_NAME_FINALIZE = "rpc_handler_get_tv_search_result_rpc_finalize";

// TODO use realization from https://a.yandex-team.ru/arcadia/alice/hollywood/library/scenarios/video/video_search.cpp?rev=r9667198#L201
const NJson::TJsonValue& GetLastSection(const NAppHost::IServiceContext& ctx, TStringBuf type) {
    NAppHost::TContextItemRefArray responses = ctx.GetItemRefs(type);

    static_assert(
        std::is_same<decltype(responses.back()), const NJson::TJsonValue&>::value,
        "NAppHost::TContextItemRefArray::front() must return a const reference.");
    return responses.empty() ? NJson::TJsonValue::UNDEFINED : responses.back();
}

void SetupVideoSearchRequest(const NRpc::TRpcRequestWrapper<TTvSearchRequest>& rpcRequest, THwServiceContext& ctx) {
    TCgiParameters cgi;

    auto addRearr = [&cgi](const TString& rearr) {
        cgi.InsertUnescaped(TStringBuf("rearr"), rearr);
    };

    TString client = "tvandroid";
    cgi.InsertUnescaped(TStringBuf("client"), client);
    addRearr("forced_player_device=" + client);

    if (!rpcRequest.GetRequest().GetSearchText().Empty()) {
        cgi.InsertUnescaped(TStringBuf("text"), rpcRequest.GetRequest().GetSearchText());
    } else {
        cgi.InsertUnescaped(TStringBuf("text"), TStringBuf("видео"));
    }

    NJson::TJsonValue request;
    request["headers"].SetValue(NJson::JSON_ARRAY);
    auto addHeader = [&request](const TStringBuf& key, const TString& value) {
        NJson::TJsonValue header;
        header.SetValue(NJson::JSON_ARRAY);
        header.AppendValue(key);
        header.AppendValue(value);
        request["headers"].AppendValue(header);
    };
    const auto host = "yandex.ru";
    addHeader("Host", host);
    addHeader(NAlice::NNetwork::HEADER_USER_AGENT, rpcRequest.GetRequestMeta()->GetUserTicket());
    addHeader(NAlice::NNetwork::HEADER_X_YANDEX_INTERNAL_REQUEST, "1");
    request["uri"] = TStringBuilder{} << "/video/result?" << cgi.Print();
    request["type"] = "http_request";
    request["method"] = "GET";
    ctx.ApphostContext().AddItem(request, "video_search_request");
}

} // namespace

class TGetTvSearchResultHandle : public IHwServiceHandle {
public:
    void Do(THwServiceContext& ctx) const override {
        NRpc::TRpcRequestWrapper<TTvSearchRequest> request{ctx};
        SetupVideoSearchRequest(request, ctx);
    }

    const TString& Name() const override {
        return GET_TV_SEARCH_RESULT_HANDLER_NAME;
    }
};

class TGetTvSearchResultHandleFinalize : public IHwServiceHandle {
public:
    void Do(THwServiceContext& ctx) const override {
        auto httpResponse = GetLastSection(ctx.ApphostContext(), "video_search_response");
        LOG_DEBUG(ctx.Logger()) << httpResponse << Endl;

        NRpc::TRpcResponseBuilder<TTvSearchResultData> responseBuilder;
        TTvSearchResultData result;
        result.AddGalleries()->SetId("test_gallery_id");
        responseBuilder.SetResponseBody(result);
        ctx.AddProtobufItemToApphostContext(std::move(responseBuilder).Build(), "rpc_response");
    }

    const TString& Name() const override {
        return GET_TV_SEARCH_RESULT_HANDLER_NAME_FINALIZE;
    }
};


} // namespace NAlice::NHollywood::NVideo
