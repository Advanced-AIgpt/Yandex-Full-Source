
#include <alice/cuttlefish/tools/streaming_grpc_client/apphost_session.h>
#include <apphost/lib/grpc/client/grpc_pool.h>
#include <apphost/lib/grpc/json/service_request.h>
#include <apphost/lib/grpc/json/service_response.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <future>


NJson::TJsonValue ReadJsonArray(IInputStream& input)
{
    static TVector<char> buff;

    buff.resize(0);
    char s;

    while (input.ReadChar(s) && std::isspace(s)) // strip
        ;

    if (s != '[')
        ythrow yexception() << "invalid JSON array";

    buff.push_back(s);

    int depth = 1;
    int esc_count = 0;
    while (input.ReadChar(s) && depth > 0) {
        if (s == '\\') {
            ++esc_count;
        } else {
            if (s == '[') {
                if (esc_count % 2 == 0)
                    ++depth;
            } else if (s == ']') {
                if (esc_count % 2 == 0)
                    --depth;
            }
            esc_count = 0;
        }
        buff.push_back(s);
    }

    NJson::TJsonValue value;
    NJson::ReadJsonTree(TStringBuf(buff.data(), buff.size()), &value, /*throwOnError =*/ true);
    return value;
}

void StdinToSession(TAppHostSession& session)
{
    try {
        NAppHostProtocol::TServiceRequest req;
        req.SetPath("/");
        while (true) {
            Cerr << "Read request JSON from stdin..." << Endl;
            const NJson::TJsonValue reqJson = ReadJsonArray(Cin);

            NAppHost::LoadRequestFromJSON(reqJson, &req);


            if (!session.Write(std::move(req)).GetValueSync()) {
                Cerr << "Could not send request" << Endl;
                break;
            }
            Cerr << "Request was sent" << Endl;

        }
    } catch (const std::exception& err) {
        Cerr << "StdinToSession failed: " << err.what() << Endl;
        session.WritesDone();
    }
}



int main(int, const char**)
{

    NAppHost::NTransport::NGrpc::TGrpcPool grpcPool;
    grpcPool.SetThreads(1);

    auto session = TAppHostSession::Start(grpcPool, "localhost:10003");

    if (!session->IsReady().GetValueSync()) {
        Cerr << "could not get ready" << Endl;
        return -1;
    }
    Cerr << "Session is ready" << Endl;

    auto writeFut = std::async(std::launch::async, [&session](){
        StdinToSession(*session);
    });

    TString respData;
    while (true) {
        auto responseFut = session->Read();
        const TAppHostSession::TMaybeServiceResponse response = responseFut.GetValueSync();
        if (!response.Defined())
            break;

        NAppHost::SaveResponseToJSON(*response, true, false, &Cout);
        Cout << Endl;
    }

    writeFut.wait();
}
