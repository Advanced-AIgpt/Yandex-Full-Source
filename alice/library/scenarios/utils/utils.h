#pragma once

#include <alice/library/network/common.h>
#include <alice/library/network/headers.h>

#include <alice/megamind/library/util/status.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <util/string/ascii.h>

namespace NAlice {

template <typename TProto>
TErrorOr<TProto> ParseProto(const TString& data, TStringBuf method) {
    TProto proto;
    if (!proto.ParseFromString(data)) {
        return TError{TError::EType::Parse} << "Failed to parse proto from the " << method << " response";
    }
    return proto;
}

template <typename TProto>
[[nodiscard]] TErrorOr<TProto> ParseScenarioResponse(const TErrorOr<NAppHostHttp::THttpResponse>& response, TStringBuf method) {
    if (response.Error() != nullptr) {
        return *response.Error();
    }
    const NAppHostHttp::THttpResponse& responseProto = response.Value();
    if (int code = responseProto.GetStatusCode(); code != HttpCodes::HTTP_OK) {
        return TError{TError::EType::Logic} << "Failed to get response from the " << method << " request, code: " << code
                                            << ", error: " << responseProto.GetContent();
    }

    const auto it = FindIf(responseProto.GetHeaders().begin(), responseProto.GetHeaders().end(),
        [](const NAppHostHttp::THeader& header) {
            return AsciiCompareIgnoreCase(header.GetName(), NNetwork::HEADER_CONTENT_TYPE) == 0;
        }
    );
    if (it == responseProto.GetHeaders().end()) {
        return TError{TError::EType::Http} << "No " << NNetwork::HEADER_CONTENT_TYPE << " in response";
    }
    if (it->GetValue() != NContentTypes::APPLICATION_PROTOBUF) {
        return TError{TError::EType::Http} << "Unsupported " << NNetwork::HEADER_CONTENT_TYPE
                                           << " value: " << it->GetValue();
    }

    return ParseProto<TProto>(responseProto.GetContent(), method);
}

} // namespace NAlice
