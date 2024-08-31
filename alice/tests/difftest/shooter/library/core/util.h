#pragma once

#include <alice/tests/difftest/shooter/library/core/context.h>

#include <util/generic/hash.h>

namespace NAlice::NShooter {

void CopyDir(const TString& source, const TString& destination);

TString MakeUrl(TStringBuf host, ui16 port);
TString MakeHostPort(TStringBuf host, ui16 port);

THashMap<TString, TString> MakeProxyHeaders(const IContext& ctx, TStringBuf requestId = "default", TStringBuf guid = TStringBuf());
TString BuildHeaders(const IContext& ctx, ui16 port, TStringBuf reqId, TStringBuf guid, TMaybe<TStringBuf> timestamp);

TString BeautifyJson(TString json);

} // namespace NAlice::NShooter
