#pragma once

#include <alice/hollywood/library/framework/core/request.h>

#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

NAppHostHttp::THttpRequest PrepareCommonHttpRequest(const TRequest& request, TStringBuf userId, TStringBuf path, TString name = Default<TString>());

} // namespace NAlice::NHollywoodFw::NMusic
