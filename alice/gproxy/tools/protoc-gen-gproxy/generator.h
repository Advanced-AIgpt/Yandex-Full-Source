#pragma once

#include <google/protobuf/compiler/code_generator.h>


namespace NGProxyTraits {

class TGenerator : public ::google::protobuf::compiler::CodeGenerator {
public:
    bool Generate(
        const ::google::protobuf::FileDescriptor* file,
        const TProtoStringType& parameter,
        ::google::protobuf::compiler::GeneratorContext* generator_context,
        TProtoStringType* error
    ) const override;
};

}  // namespace NGProxyTraits
