#include "serialization.h"

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/streams/factory/factory.h>
#include <library/cpp/streams/lzma/lzma.h>
#include <library/cpp/string_utils/base64/base64.h>

namespace NHttpFetcher {

NSc::TValue ResponseToJson(TResponse::TRef response) {
    NSc::TValue json;

    TStringStream str;
    {
        TLzmaCompress out(&str, /*level*/ 3);
        out << response->Data;
    }

    json["Result"].SetString(ToString(response->Result));
    json["Code"].SetIntNumber(response->Code);
    json["Data"].SetString(Base64Encode(str.Str()));

    auto& headers = json["Headers"];
    for (auto& h : response->Headers) {
        headers[h.Name()] = h.Value();
    }
    json["DurationUS"].SetIntNumber(response->Duration.MicroSeconds());
    json["ErrMsg"].SetString(response->GetErrorText());

    return json;
}

TResponse::TRef ResponseFromJson(const NSc::TValue& json) {
    THttpHeaders headers;
    const NSc::TValue& jsonHeaders = json["Headers"];
    for (const auto& h : jsonHeaders.GetDict()) {
        headers.AddHeader(TString{h.first}, h.second.GetString());
    }

    TString result = Base64Decode(json["Data"].GetString());
    TMemoryInput mem(result.data(), result.size());

    TStringStream data;
    {
        TLzmaDecompress decompress(&mem);
        TransferData(&decompress, &data);
    }

    TResponse::TRef r = MakeIntrusive<TResponse>(
        static_cast<TResponse::THttpCode>(json["Code"].GetIntNumber()),
        data.Str(),
        TDuration::MicroSeconds(json["DurationUS"].GetIntNumber()),
        json["ErrMsg"].GetString(),
        std::move(headers)
        );

    r->Result = FromString<TResponse::EResult>(json["Result"].GetString());
    return r;
}

} // namespace NHttpFetcher
