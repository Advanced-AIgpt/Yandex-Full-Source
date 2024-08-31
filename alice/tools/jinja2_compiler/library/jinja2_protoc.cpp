#include "jinja2_protoc.h"

#include <contrib/libs/protobuf/src/google/protobuf/descriptor.h>
#include <contrib/libs/protobuf/src/google/protobuf/io/zero_copy_stream_impl.h>

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/message.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/io/tokenizer.h>

#include <util/generic/vector.h>
#include <util/system/execpath.h>
#include <util/stream/output.h>
#include <util/system/file.h>

namespace NAlice {

// Error and warning output
class TMultiFileErrorCollector : public google::protobuf::compiler::MultiFileErrorCollector {
    virtual void AddError(const TProtoStringType& filename, int line, int column, const TProtoStringType& message) {
        Cout << "Error: " << filename << "(" << line << "#" << column << "): " << message << Endl;
    }
    virtual void AddWarning(const TProtoStringType& filename, int line, int column, const TProtoStringType& message) {
        Cout << "Warning: " << filename << "(" << line << "#" << column << "): " << message << Endl;
    }
};

// Source tree processor
class TSourceTree : public google::protobuf::compiler::SourceTree {
public:
    TSourceTree() {
        TString exePath = GetExecPath();

        // Add first include path (current folder, i.e. filename without path)
        IncludePathList_.push_back("");

        // Add arcadia root and contrib/libs/protobuf/src/
        size_t pos = exePath.find("arcadia");
        if (pos != TString::npos) {
            TString rootPath = exePath.substr(0, pos);
            IncludePathList_.push_back(rootPath);
            IncludePathList_.push_back(rootPath + "contrib/libs/protobuf/src/");
        }
    }
    void AddIncludeDir(const TString& dir) {
        IncludePathList_.push_back(dir);
    }

    virtual google::protobuf::io::ZeroCopyInputStream* Open(const TProtoStringType& filename) {
        for (const auto& it: IncludePathList_) {
            TString fullPath = it;
            fullPath += filename;
            TFileHandle* inputProtoStream = new TFileHandle(fullPath, OpenExisting | RdOnly);
            if (inputProtoStream->IsOpen()) {
                return new google::protobuf::io::FileInputStream(static_cast<int>(*inputProtoStream));
            }
        }
        return nullptr;
    }
private:
    TVector<TString> IncludePathList_;
};

//
// Ctor
// Create all needed objects and parse --include directives
//
TProtoCompiler::TProtoCompiler(const TVector<TString>& includeFolders)
: SourceTree_(new TSourceTree)
, ErrorCollector_(new TMultiFileErrorCollector)
, Importer_(SourceTree_, ErrorCollector_) {
    for (const auto& it : includeFolders) {
        SourceTree_->AddIncludeDir(it);
    }
}

//
// Dtor
//
TProtoCompiler::~TProtoCompiler() {
    delete SourceTree_;
    delete ErrorCollector_;
}

//
// Compile all proto files added in command line 
//
const google::protobuf::DescriptorPool* TProtoCompiler::Compile(const TVector<TString>& protoRoot) {
    for (const auto& it : protoRoot) {
        if (Importer_.Import(it) == nullptr) {
            // Error in compilation
            Cerr << "Failed compilation: " << it << Endl;            
            return nullptr;
        }
    }
    return Importer_.pool();
}

} // namespace NAlice

