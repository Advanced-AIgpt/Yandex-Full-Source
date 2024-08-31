#pragma once
#include <util/generic/strbuf.h>
#include <alice/cuttlefish/library/protos/session.pb.h>


namespace NAlice::NCuttlefish::NAppHostServices {

ui16 ChannelsFromMime(const TString& mime);
TString ConstructMdsFilename(const NAliceProtocol::TRequestContext& requestContext, bool isSpotter);
TString GetKeyFromMdsSaveResponse(const TString& content);  // assuming that "content" is body from HTTP OK mds response

}  // namespace NAlice::NCuttlefish::NAppHostServices

