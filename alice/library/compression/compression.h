#pragma once

#include <util/generic/strbuf.h>
#include <util/stream/zlib.h>

namespace NAlice {

TString ZLibCompress(TStringBuf str, ZLib::StreamType type = ZLib::GZip);
TString ZLibDecompress(TStringBuf str);

}
