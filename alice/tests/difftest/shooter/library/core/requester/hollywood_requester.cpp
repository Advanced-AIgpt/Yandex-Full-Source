#include "hollywood_requester.h"

#include <alice/joker/library/log/log.h>
#include <alice/tests/difftest/shooter/library/core/util.h>
#include <alice/tests/difftest/shooter/library/core/engine.h>

#include <apphost/lib/client/client_shoot.h>
#include <apphost/lib/common/statistics.h>
#include <apphost/lib/grpc/client/grpc_client.h>
#include <apphost/lib/grpc/json/service_request.h>
#include <apphost/lib/grpc/json/service_response.h>
#include <apphost/lib/grpc/protos/service.pb.h>

#include <library/cpp/protobuf/json/proto2json.h>

#include <util/generic/queue.h>
#include <util/string/builder.h>
#include <util/string/strip.h>

namespace NAlice::NShooter {

THollywoodRequester::THollywoodRequester(const IEngine& engine)
    : Engine_{engine}
{
}

TMaybe<TRequestResponse> THollywoodRequester::Request(const TFsPath& path) const {
    using NAppHost::NGrpc::NClient::TGrpcCommunicationSystem;
    using NAppHost::NGrpc::NClient::IServiceClient;

    // Setup client
    TGrpcCommunicationSystem sys;
    sys.SetUserAgentPrefix("grpc_client");
    TString hostPort = TStringBuilder{} << "localhost:" << (Engine_.Ports()->Get("hollywood") + 1); // target port is +1 to hollywood's port
    IServiceClient& client = sys.ProvideServiceClient(hostPort);

    // Setup request
    NAppHostProtocol::TServiceRequest request;
    TFileInput fin{path};
    NJson::TJsonValue tree = NJson::ReadJsonTree(&fin);
    TString inputData = Base64Decode(StripString(tree["Data"].GetString()));
    if (inputData.Empty() || !request.ParseFromArray(inputData.data(), inputData.size())) {
        LOG(ERROR) << "Can't parse request in " << path << Endl;
        return Nothing();
    }

    // Send request
    NAppHost::NGrpc::NClient::TServiceResponsePtr response;
    NAppHost::NGrpc::NClient::TServiceSessionPtr session;

    try {
        session = client.StartSession();
    } catch (...) {
        LOG(ERROR) << CurrentExceptionMessage() << Endl;
        return Nothing();
    }

    session->SendRequest(std::move(request));

    try {
        response = session->Response().GetValue(TDuration::Seconds(10));
    } catch (...) {
        LOG(ERROR) << CurrentExceptionMessage() << Endl;
        return Nothing();
    }

    if (!response) {
        LOG(ERROR) << "failed to get a response from " << path << Endl;
        return Nothing();
    }

    // Return output
    const auto& runSettings = Engine_.RunSettings();
    TDuration duration = session->GetDuration();
    TString data = NProtobufJson::Proto2Json(*response, NProtobufJson::TProto2JsonConfig().SetFormatOutput(true));
    TFsPath outputPath = TFsPath{runSettings.ResponsesPath} / path.GetName() / "response.json";

    return TMaybe<TRequestResponse>({std::move(data), duration, outputPath});
}

} // namespace NAlice::NShooter
