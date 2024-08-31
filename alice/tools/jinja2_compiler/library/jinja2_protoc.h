#pragma once

#include <util/generic/noncopyable.h>

#include <contrib/libs/protoc/src/google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.pb.h>

namespace NAlice {

class TSourceTree;
class TMultiFileErrorCollector;

class TProtoCompiler : public TNonCopyable {
public:
    explicit TProtoCompiler(const TVector<TString>& includeFolders);
    ~TProtoCompiler();

    const google::protobuf::DescriptorPool* Compile(const TVector<TString>& protoRoot);

private:
    TSourceTree* SourceTree_;
    TMultiFileErrorCollector* ErrorCollector_;
    google::protobuf::compiler::Importer Importer_;
};

} // namespace NAlice
