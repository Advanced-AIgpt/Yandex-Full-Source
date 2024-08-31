#include "jinja2_compiler.h"
#include "jinja2_proto2json.h"
#include "jinja2_protoc.h"
#include "jinja2_render.h"

#include <util/system/file.h>
#include <util/stream/output.h>
#include <util/system/user.h>

#include <ctime>

namespace NAlice {

namespace {

constexpr TStringBuf OPTION_PROTO = "--proto=";
constexpr TStringBuf OPTION_INPUT = "--input=";
constexpr TStringBuf OPTION_OUTPUT = "--output=";
constexpr TStringBuf OPTION_MESSAGE = "--message=";
constexpr TStringBuf OPTION_INCLUDE = "--include=";
constexpr TStringBuf OPTION_DUMP = "--dump=";
constexpr TStringBuf OPTION_HELP = "--help";

}

//
// Parse command line options
//
bool TJinja2Compiler::InitOptions(int argc, const char** argv) {
    TVector<TString> inputFiles;
    TVector<TString> outputFiles;

    for (int i=1; i<argc; i++) {
        const TString arg = argv[i];

        // Examine possible parameters
        if (arg.StartsWith(OPTION_PROTO.Data(), OPTION_PROTO.Size())) {
            Options_.ProtoRoots.push_back(arg.c_str() + OPTION_PROTO.Size());
            continue;
        }
        if (arg.StartsWith(OPTION_INPUT.Data(), OPTION_INPUT.Size())) {
            inputFiles.push_back(arg.c_str() + OPTION_INPUT.Size());
            continue;
        }
        if (arg.StartsWith(OPTION_OUTPUT.Data(), OPTION_OUTPUT.Size())) {
            outputFiles.push_back(arg.c_str() + OPTION_OUTPUT.Size());
            continue;
        }
        if (arg.StartsWith(OPTION_MESSAGE.Data(), OPTION_MESSAGE.Size())) {
            Options_.ClassNames.push_back(arg.c_str() + OPTION_MESSAGE.Size());
            continue;
        }
        if (arg.StartsWith(OPTION_INCLUDE.Data(), OPTION_INCLUDE.Size())) {
            Options_.IncludeFolders.push_back(arg.c_str() + OPTION_INCLUDE.Size());
            continue;
        }
        if (arg.StartsWith(OPTION_DUMP.Data(), OPTION_DUMP.Size())) {
            Options_.DebugDump = arg.c_str() + OPTION_DUMP.Size();
            continue;
        }
        if (arg.StartsWith(OPTION_HELP.Data(), OPTION_HELP.Size())) {
            return false;
        }
        Cerr << "Undefined argument " << arg << Endl;
        return false;
    }

    // Validate options
    if (outputFiles.size() != inputFiles.size()) {
        Cerr << "--input and --output options count mismatch" << Endl;
        return false;
    }
    for (size_t i = 0; i<inputFiles.size(); i++) {
        Options_.CompileList.push_back(Options::InOut{inputFiles[i], outputFiles[i]});
    }
    if (Options_.ProtoRoots.empty()) {
        Cerr << "At least one --proto parameter must be present" << Endl;
        return false;
    }

    return true;
}

//
// Print command line options
//
void TJinja2Compiler::PrintUsage() {
    Cout << "Usage: jinja2_compiler [parameters]" << Endl;
    Cout << "Parameters are:" << Endl;
    Cout << " --proto=filename.proto - main protofile to compile" << Endl;
    Cout << " --input=filename.jinja2 - input template " << Endl;
    Cout << " --output=filename.ext - output file" << Endl;
    Cout << " --message=proto_message - protobuff message name to pass to jinja2 template renderer" << Endl;
    Cout << " --include=path - additional include path to load proto files" << Endl;
    Cout << " --dump=var - debug dump variable and its content" << Endl;
    Cout << " --help - display thos message" << Endl;
    Cout << Endl;
    Cout << "All options can be repeated as many times as need" << Endl;
    Cout << "--input and --output options must be related to each other (i.e. --input=file1.jinja --output=file1.cpp --input=file2.jinja --output=file2.cpp, etc)" << Endl;
}

//
// Main entry point for application
//
int TJinja2Compiler::Run() {
    //
    // Run protoc compiler to read all required proto files
    //
    TProtoCompiler compiler(Options_.IncludeFolders);
    const google::protobuf::DescriptorPool* pool = compiler.Compile(Options_.ProtoRoots);
    if (pool == nullptr) {
        Cerr << "Compilation failed." << Endl;
        return 1;
    }

    TJinja2Proto2Json proto2json;
    for (const auto& it : Options_.ClassNames) {
        const google::protobuf::FileDescriptor* originalFile =  pool->FindFileByName(Options_.ProtoRoots[0]);
        if (originalFile == nullptr) {
            Cout << "Source file not found for descriptor " << it << Endl;
            return 2;
        }
        const google::protobuf::Descriptor* descr = originalFile->FindMessageTypeByName(it);
        if (descr == nullptr) {
            Cout << "Can't find descriptor " << it << Endl;
            return 2;
        }
        proto2json.AddProtobufRoot(descr);
    }

    for (const auto& it : Options_.CompileList) {
        //
        // Setup Jinja2 Environment for protobuffs
        //
        TJinja2Render render;

        // Setup 'config' group
        render.AddConfigArray("messages", Options_.ClassNames);
        render.AddConfigArray("includes", Options_.IncludeFolders);
        render.AddConfigArray("protos", Options_.ProtoRoots);
        {
            TVector<TString> inputs;
            TVector<TString> outputs;
            for (const auto& it: Options_.CompileList) {
                inputs.push_back(it.InputTemplate);
                outputs.push_back(it.OutputFile);
            }
            render.AddConfigArray("inputs", inputs);
            render.AddConfigArray("outputs", outputs);
        }

        // Setup 'system' group
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%d-%m-%Y %H-%M-%S", &tm);
        jinja2::ValuesMap valuesSystemMap;
        valuesSystemMap["user"] = GetUsername().c_str();
        valuesSystemMap["time"] = buffer;
        valuesSystemMap["version"] = VERSION.Data();
        render.GetTemplateEnv().AddGlobal("system", valuesSystemMap);

        render.GetTemplateEnv().AddGlobal("header1", "#");
        render.GetTemplateEnv().AddGlobal("header2", "##");
        render.GetTemplateEnv().AddGlobal("header3", "###");
        render.GetTemplateEnv().AddGlobal("header4", "####");
        render.GetTemplateEnv().AddGlobal("header5", "#####");

        proto2json.PrepareVars(render.GetTemplateEnv());

        if (Options_.DebugDump != "") {
            proto2json.DebugDump(Options_.DebugDump, render.GetTemplateEnv());
        }

        // Load and parse files
        Cout << "Loading " << it.InputTemplate << "..." << Endl;
        render.LoadTemplateFile(it.InputTemplate);

        Cout << "Parsing " << it.InputTemplate << "..." << Endl;
        std::string str = render.Render();

        Cout << "Saving " << it.OutputFile << "..." << Endl;
        TFileHandle outputFile(it.OutputFile, CreateAlways | WrOnly);
        if (!outputFile.IsOpen()) {
            Cout << "Can't open destination file " << it.OutputFile << Endl;
            return 3;
        }
        outputFile.Write(str.c_str(), str.size());
    }
    return 0;
}

} // namespace NAlice
