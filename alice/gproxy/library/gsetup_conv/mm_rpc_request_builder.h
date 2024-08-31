#pragma once

#include <alice/cuttlefish/library/protos/session.pb.h>

#include <alice/library/client/protos/client_info.pb.h>
#include <alice/library/json/json.h>

#include <alice/megamind/protos/common/experiments.pb.h>
#include <alice/megamind/protos/grpc_request/request.pb.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/protobuf/json/util.h>

#include <google/protobuf/struct.pb.h>

#include <util/string/cast.h>

namespace NGProxy {

class TMMRpcRequestBuilder {
public:
    TMMRpcRequestBuilder(TString method) {
        NProtobufJson::ToSnakeCase(&method);
        Request.SetHandler(method);
    }

    TMMRpcRequestBuilder& SetRequestId(const TString& requestId) {
        Request.MutableMeta()->SetRequestId(requestId);
        return *this;
    }

    TMMRpcRequestBuilder& SetRandomSeed(ui64 randomSeed) {
        Request.MutableMeta()->SetRandomSeed(randomSeed);
        return *this;
    }

    TMMRpcRequestBuilder& SetServerTimeMs(ui64 serverTimeMs) {
        Request.MutableMeta()->SetServerTimeMs(serverTimeMs);
        return *this;
    }

    TMMRpcRequestBuilder& ConstructApplication(const NAliceProtocol::TSessionContext& session) {
        auto& app = *Request.MutableMeta()->MutableApplication();
        app.SetAppId(session.GetAppId());
        app.SetDeviceId(session.GetDeviceInfo().GetDeviceId());
        app.SetLang(session.GetAppLang());
        if (session.GetDeviceInfo().HasDeviceManufacturer()) {
            app.SetDeviceManufacturer(session.GetDeviceInfo().GetDeviceManufacturer());
        }
        if (session.GetDeviceInfo().HasDeviceModel()) {
            app.SetDeviceModel(session.GetDeviceInfo().GetDeviceModel());
        }
        if (session.GetDeviceInfo().HasDeviceModification()) {
            app.SetDeviceRevision(session.GetDeviceInfo().GetDeviceModification());
        }
        if (session.GetDeviceInfo().HasPlatform()) {
            app.SetPlatform(session.GetDeviceInfo().GetPlatform());
        }
        if (session.GetUserInfo().HasUuid()) {
            app.SetUuid(session.GetUserInfo().GetUuid());
        }

        return *this;
    }

    TMMRpcRequestBuilder& SetRequest(const google::protobuf::Any& request) {
        Request.MutableRequest()->CopyFrom(request);
        return *this;
    }

    TMMRpcRequestBuilder& SetExperiments(const NAlice::TExperimentsProto& exps) {
        Request.MutableMeta()->MutableExperiments()->CopyFrom(exps);
        return *this;
    }

    TMMRpcRequestBuilder& SetSupportedFeatures(const google::protobuf::RepeatedPtrField<TProtoStringType>& features) {
        Request.MutableMeta()->MutableSupportedFeatures()->CopyFrom(features);
        return *this;
    }

    TMMRpcRequestBuilder& SetUnsupportedFeatures(const google::protobuf::RepeatedPtrField<TProtoStringType>& features) {
        Request.MutableMeta()->MutableUnsupportedFeatures()->CopyFrom(features);
        return *this;
    }

    TMMRpcRequestBuilder& SetTestIds(const NAliceProtocol::TFlagsInfo& flags) {
        for (size_t i = 0; i < flags.AllTestIdsSize(); ++i) {
            Request.MutableMeta()->AddTestIDs(FromString<ui64>(flags.GetAllTestIds(i)));
        }
        return *this;
    }

    TMMRpcRequestBuilder& SetLaasRegion(const NAppHostHttp::THttpResponse& laasHttpResponse) {
        const TString content = laasHttpResponse.GetContent();
        NJson::TJsonValue laas;
        if (NJson::ReadJsonTree(content, &laas, false)) {
            *Request.MutableMeta()->MutableLaasRegion() = NAlice::JsonToProto<google::protobuf::Struct>(laas);
        }
        return *this;
    }

    NAlice::NRpc::TRpcRequestProto Build() && {
        return std::move(Request);
    }

private:
    NAlice::NRpc::TRpcRequestProto Request;
};

}   // namespace NGProxy
