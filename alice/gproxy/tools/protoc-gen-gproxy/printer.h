#pragma once

#include <util/generic/ptr.h>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>

#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/printer.h>


namespace NGProxyTraits {

class TPrinter {
public:
    TPrinter(::google::protobuf::compiler::GeneratorContext* ctx, const ::google::protobuf::FileDescriptor *file)
        : Stream(OpenStream(ctx, file))
        , Printer(Stream.Get(), '$')
    {
        Printer.Print("#pragma once\n\n");
    }

    ~TPrinter() {
    }

    template <typename ... TArgs>
    inline TPrinter& Print(TArgs&& ... args) {
        Printer.Print(std::forward<TArgs>(args)...);
        return *this;
    }

    inline TPrinter& NewLine() {
        Printer.Print("\n");
        return *this;
    }

    inline TPrinter& PrintInclude(const TProtoStringType& path) {
        Printer.Print(
            "#include <$PB$>\n",
            "PB", path
        );
        return *this;        
    }

    inline TPrinter& Indent() {
        Printer.Indent();
        Printer.Indent();
        return *this;
    }

    inline TPrinter& Outdent() {
        Printer.Outdent();
        Printer.Outdent();
        return *this;
    }

    inline TPrinter& OpenNamespace() {
        return OpenNamespace("NGProxyTraits");
    }

    inline TPrinter& OpenNamespace(const TString& name) {
        return Print("namespace $NS$ {\n", "NS", name);
    }

    inline TPrinter& CloseNamespace() {
        return CloseNamespace("NGProxyTraits");
    }

    inline TPrinter& CloseNamespace(const TString& name) {
        return Print("}   // namespace $NS$\n", "NS", name);
    }

private:
    ::google::protobuf::io::ZeroCopyOutputStream* OpenStream(::google::protobuf::compiler::GeneratorContext *ctx, const ::google::protobuf::FileDescriptor *file);

private:
    THolder<google::protobuf::io::ZeroCopyOutputStream> Stream; // (generator_context->Open(ProtobufTraitsHeader(file)));
    ::google::protobuf::io::Printer                     Printer;
};

}   // namespace NGProxyTraits
