#pragma once

#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <util/generic/strbuf.h>
#include <util/generic/maybe.h>

namespace NAlice::NHollywood::NMusic {

constexpr TStringBuf ALBUM_PODUSHKI_SHOW_ID = "17277367";

TMaybe<TContentId> ContentIdFromText(const TStringBuf type, const TStringBuf id);

TString ContentTypeToText(TContentId::EContentType contentType);

TString ContentTypeToNLGType(TContentId::EContentType contentType);

} // namespace NAlice::NHollywood::NMusic
