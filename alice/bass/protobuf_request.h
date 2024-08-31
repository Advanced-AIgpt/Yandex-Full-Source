#pragma once

#include "http_request.h"

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/library/network/common.h>

#include <library/cpp/http/io/headers.h>
#include <library/cpp/http/server/response.h>

#include <util/generic/string.h>

#include <contrib/libs/protobuf/src/google/protobuf/message_lite.h>

namespace NBASS {

template <class TRequestProto, class TResponseProto>
class TProtobufRequestHandler : public TLoggingHttpRequestHandler {
public:
    virtual HttpCodes DoProtobufReply(TGlobalContextPtr globalCtx, const THttpHeaders& httpHeaders,
                                      const TRequestProto& requestProto, TResponseProto& responseProto) = 0;

    THttpResponse DoTextReply(TGlobalContextPtr globalCtx, const TString& requestText,
                              const TParsedHttpFull& /*httpRequest*/, const THttpHeaders& httpHeaders) override
    {
        TRequestProto requestProto;
        Y_PROTOBUF_SUPPRESS_NODISCARD requestProto.ParseFromString(requestText);

        TResponseProto responseProto;
        THttpResponse httpResponse(DoProtobufReply(globalCtx, httpHeaders, requestProto, responseProto));

        TString responseText;
        Y_PROTOBUF_SUPPRESS_NODISCARD responseProto.SerializeToString(&responseText);
        httpResponse.SetContent(responseText, NAlice::NContentTypes::APPLICATION_PROTOBUF);
        return httpResponse;
    }
};

} // namespace NBASS
