#include "server.h"

#include <regex>

#include <alice/nlu/granet/lib/compiler/compiler_error.h>
#include <alice/nlu/granet/lib/granet.h>

#include <alice/paskills/granet_server/library/granet_wrapper.h>
#include <alice/paskills/granet_server/proto/compile_grammar_request.pb.h>
#include <alice/paskills/granet_server/proto/compile_grammar_response.pb.h>

#include <library/cpp/http/misc/http_headers.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/writer/json.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/string/strip.h>

namespace NGranetServer {

namespace {

    constexpr TStringBuf CONTENT_TYPE_APPLICATION_JSON = "application/json";

    constexpr TStringBuf GRANET_TRANSLATION_NEEDED_MARKER = "<TRANSLATE>";
    constexpr auto GRANET_DEFAULT_ERROR_MESSAGE = "Ошибка";

    const std::regex RE_WHITESPACE("^\\s*$");

    NProtobufJson::TJson2ProtoConfig Json2ProtoConfig() {
        return NProtobufJson::TJson2ProtoConfig()
                              .SetMapAsObject(true)
                              .SetUseJsonName(true);
    }

    NProtobufJson::TProto2JsonConfig Proto2JsonConfig() {
        return NProtobufJson::TProto2JsonConfig()
                              .SetMapAsObject(true)
                              .SetUseJsonName(true)
                              .SetAddMissingFields(true)
                              .SetMissingRepeatedKeyMode(NProtobufJson::TProto2JsonConfig::MissingKeyDefault);

    }

    TCompileGrammarResponse SerializeCompiledGrammar(TGranetCompilerResult& compiledGrammar) {
        TCompileGrammarResponse responseBody;
        auto result = responseBody.MutableSuccess();
        result->SetGrammarBase64(compiledGrammar.Base64Grammar);
        *result->MutableTruePositives() = { compiledGrammar.TruePositives.begin(), compiledGrammar.TruePositives.end() };
        *result->MutableTrueNegatives() = { compiledGrammar.TrueNegatives.begin(), compiledGrammar.TrueNegatives.end() };
        *result->MutableFalsePositives() = { compiledGrammar.FalsePositives.begin(), compiledGrammar.FalsePositives.end() };
        *result->MutableFalseNegatives() = { compiledGrammar.FalseNegatives.begin(), compiledGrammar.FalseNegatives.end() };
        return responseBody;
    }

    TCompileGrammarResponse SerializeCompilerError(const NGranet::NCompiler::TCompilerError& compilerError) {
        TCompileGrammarResponse responseBody;
        auto serializedError = responseBody.MutableError();
        const TString message = compilerError.Message.find(GRANET_TRANSLATION_NEEDED_MARKER) == TString::npos
            ? compilerError.Message
            : GRANET_DEFAULT_ERROR_MESSAGE;
        serializedError->SetText(message);
        serializedError->SetLineIndex(compilerError.LineIndex);
        serializedError->SetColumnIndex(compilerError.ColumnIndex);
        serializedError->SetCharCount(compilerError.CharCount);
        return responseBody;
    }

} // anonymous namespace

TRequestHandler::TRequestHandler(NServer::TServer& server, const TGranetServerConfig& config)
    : NServer::TRequest{server}
    , Config(config) {
}

bool TRequestHandler::HandleCompileRequest(THttpResponse& response) {
    TString body = TString(Buf.AsCharPtr(), Buf.Size());
    INFO_LOG << "Compile grammar request: " << body;
    TCompileGrammarRequest request;
    try {
        NJson::TJsonValue requestJson;
        NJson::ReadJsonTree(body, &requestJson, /* throwOnError */ true);
        NProtobufJson::Json2Proto(requestJson, request, Json2ProtoConfig());
    } catch (const yexception& e) {
        TCompileGrammarResponse responseBody;
        WARNING_LOG << "Failed to parse compile grammar request: " << e.what();
        responseBody.MutableError()->SetText(e.what());
        response = TextResponse(NProtobufJson::Proto2Json(responseBody, Proto2JsonConfig()));
        response.SetHttpCode(HttpCodes::HTTP_BAD_REQUEST);
        response.SetContentType(CONTENT_TYPE_APPLICATION_JSON);
        return true;
    }

    THashMap<TString, TString> grammars;
    TVector<TString> expectedPositives, expectedNegatives;
    for (const auto& test: request.GetPositiveTests()) {
        expectedPositives.emplace_back(test);
    }
    for (const auto& test: request.GetNegativeTests()) {
        expectedNegatives.emplace_back(test);
    }
    for (const auto& [key, value]: request.GetGrammars()) {
        if (!std::regex_match(value.data(), RE_WHITESPACE)) {
            grammars[key] = value;
        }
    }

    TCompileGrammarResponse responseBody;
    HttpCodes responseCode = HTTP_OK;
    if (grammars.size() == 0) {
        responseCode = HTTP_BAD_REQUEST;
        // leave response body empty so that UI won't highlight any errors
        DEBUG_LOG << "Empty grammar";
    } else {
        try {
            TGranetCompilerResult compiledGrammar = CompileGrammar(Config.GetWizard(), grammars, expectedPositives, expectedNegatives);
            responseBody = SerializeCompiledGrammar(compiledGrammar);
        } catch (const NGranet::NCompiler::TCompilerError& compilerError) {
            WARNING_LOG << "Failed to compile grammar: " << compilerError.what();
            responseBody = SerializeCompilerError(compilerError);
            responseCode = HTTP_BAD_REQUEST;
        }
    }
    // serialize response
    const TString responseText = NProtobufJson::Proto2Json(responseBody, Proto2JsonConfig());
    INFO_LOG << "Compile grammar response: " << responseText;
    response = TextResponse(responseText);
    response.SetHttpCode(responseCode);
    response.SetContentType(CONTENT_TYPE_APPLICATION_JSON);
    return true;
}

bool TRequestHandler::DoReply(const TString& script, THttpResponse& response) {
    try {
        if (script == "/granet/compile" && GetMethod() == "POST") {
            return HandleCompileRequest(response);
        } else if (script == "ping") {
            response = TextResponse("pong");
            return true;
        } else if (script == "/version" && GetMethod() == "GET") {
            TString version = TStringBuilder{} << GetBranch() << '@' << GetArcadiaLastChange();
            response = TextResponse(version);
            return true;
        }
    } catch (const yexception& exc) {
        ERROR_LOG << "Error handling " << GetMethod() << " " << script << ": " << exc.what();
        throw;
    }

    return false;
}

} // NGranetServer
