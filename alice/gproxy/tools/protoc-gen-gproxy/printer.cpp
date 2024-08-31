#include "printer.h"

#include <google/protobuf/compiler/cpp/cpp_helpers.h>

using namespace google::protobuf;
using namespace google::protobuf::compiler;
using namespace google::protobuf::io;

namespace NGProxyTraits {


io::ZeroCopyOutputStream* TPrinter::OpenStream(GeneratorContext *ctx, const FileDescriptor *file) {
    const TProtoStringType path = cpp::StripProto(file->name()) + ".gproxy.pb.h";
    return ctx->Open(path);
}


}   // namespace NGProxyTraits
