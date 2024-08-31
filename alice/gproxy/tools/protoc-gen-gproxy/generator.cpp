#include "generator.h"

#include <util/generic/string.h>
#include <util/generic/set.h>
#include <util/generic/list.h>
#include <util/generic/vector.h>

#include <util/string/join.h>

#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/printer.h>
#include <util/generic/yexception.h>


#include <alice/gproxy/library/protos/annotations/graph.pb.h>

#include "printer.h"
#include "service.h"
#include "file.h"


using namespace google::protobuf;
using namespace google::protobuf::compiler;


namespace NGProxyTraits {


bool TGenerator::Generate(
    const FileDescriptor* file,
    const TProtoStringType& /*parameter*/,
    GeneratorContext* ctx,
    TProtoStringType* /*error*/
) const {
    TFile File(file);

    TPrinter outp(ctx, file);

    for (int i = 0; i < file->service_count(); ++i) {
        TService Service(file->service(i));

        for (auto method : Service) {
            Service.AddMethod(method);

            File.AddMessage(method.InputType())
                .AddMessage(method.OutputType())
            ;
        }

        File.AddService(std::move(Service));
    }

    outp.PrintInclude("alice/gproxy/library/traits/traits.h").NewLine();
    for (auto header : File.MessagePbs()) {
        outp.PrintInclude(header);
    }
    for (auto header : File.ServicePbs()) {
        outp.PrintInclude(header);
    }

    outp.NewLine().NewLine();


    outp.OpenNamespace();

    outp.NewLine();

    for (auto service : File.Services()) {
        // TODO: refactor it
        // service.GenerateTraits(outp, File);
        // service.GenerateImplementation(outp, File);
        // service.GenerateRunner(outp, File);

        //
        //  Generate Service Traits
        //
        outp.NewLine()
            .Print("template <>\n")
            .Print("class TGProxyService<$SERVICE$> {\n", "SERVICE", service.FullName())
            .Print("public:\n")
            .Indent()
            .Print("static constexpr const char *ServiceName  = \"$NAME$\";\n", "NAME", service.FullName())
            .NewLine()
        ;

        TVector<TString> classNames;
        for (auto method : service.Methods()) {
            const TString metaclass = "TServiceMethod" + method.Name();

            classNames.push_back(metaclass);

            const std::vector<TString> flagsList = method.GetApphostFlags();
            const TString flags = flagsList.empty() ? "" : ("\"" + JoinSeq("\", \"", flagsList) + "\"")/*("\"" + JoinSeq("\", \"", flags) + "\"")*/;

            outp.Print("class $CLS$ {\n", "CLS", metaclass)
                .Print("public:\n")
                .Indent()
                .Print("using InputType = $TYPE$;\n", "TYPE", method.InputType().FullName())
                .Print("using OutputType = $TYPE$;\n", "TYPE", method.OutputType().FullName())
                .Print("using ResponseWriter = grpc::ServerAsyncResponseWriter<OutputType>;\n")
                .NewLine()
                .Print("static constexpr const char *InputTypeName          = \"$NAME$\";\n", "NAME", method.InputType().FullName())
                .Print("static constexpr const char *OutputTypeName         = \"$NAME$\";\n", "NAME", method.OutputType().FullName())
                .Print("static constexpr const char *MethodName             = \"$NAME$\";\n", "NAME", method.Name())
                .Print("static constexpr const char *MethodNameLower        = \"$NAME$\";\n", "NAME", method.NameLower())
                .Print("static constexpr const char *ApphostRequestName     = \"$NAME$\";\n", "NAME", method.ApphostRequestName())
                .Print("static constexpr const char *ApphostResponseName    = \"$NAME$\";\n", "NAME", method.ApphostResponseName())
                .Print("static constexpr const char *SemanticFrameName      = \"$NAME$\"; // semantic_frame_name\n", "NAME", method.SemanticFrameName())
                .Print("static constexpr const char *HttpPath              = \"$NAME$\"; // http_path for graph\n", "NAME", method.HttpPathName())
                .Print("static constexpr int64_t     GraphTimeout           = $VAL$; // milliseconds\n", "VAL", ToString(method.GraphTimeout()))
                .Print("static constexpr int64_t     GraphRetries           = $VAL$; // retries count\n", "VAL", ToString(method.GraphRetries()))
                .Print("static constexpr const char *GraphName              = \"$VAL$\"; // graph name\n", "VAL", ToString(method.GraphName()))
                .Print("static constexpr bool        UseRawRequestResponse  = $VAL$; // user raw grpc request/response\n", "VAL", method.UseRawRequestResponse() ? "true" : "false")
                .Print("static constexpr std::initializer_list<TStringBuf> ApphostFlags  = {$VAL$}; // graph flags\n", "VAL", flags)
                .NewLine()

                .Print("template <class AsyncService, class AsyncCall>\n")
                .Print("static inline bool Request(AsyncService *service, grpc::ServerContext *ctx, InputType *request, ResponseWriter *writer, grpc::ServerCompletionQueue *queue, AsyncCall *call) {\n")
                .Indent()
                    .Print("if (!service) return false;\n")
                    .Print("service->Request$METHOD$(ctx, request, writer, queue, queue, reinterpret_cast<void*>(call));\n", "METHOD", method.Name())
                    .Print("return true;\n")
                .Outdent()
                .Print("}\n")

                .Outdent()
                .Print("};  // class $CLS$\n", "CLS", metaclass)
                .NewLine()
            ;
        }

        if (!classNames.empty()) {
            outp.Print("using TMethodList = TTypeList<\n")
                .Indent()
            ;

            for (size_t i = 0; i < classNames.size() - 1; ++i) {
                outp.Print("$CLS$,\n", "CLS", classNames.at(i));
            }

            outp.Print("$CLS$\n", "CLS", classNames.back())
                .Outdent()
                .Print(">;\n")
                .NewLine()
            ;
        } else {
            outp.Print("using TMethodList = TTypeList<>;\n")
                .NewLine();
        }

        outp.Outdent()
            .Print("};  // class TGProxyService<$SERVICE$>\n", "SERVICE", service.FullName())
            .NewLine()
        ;
    }


    outp.CloseNamespace();

    return true;
}   // TGenerator::Generate

}   // namespace NGProxyTraits
