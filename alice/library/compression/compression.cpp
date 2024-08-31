#include "compression.h"

#include <util/stream/mem.h>
#include <util/stream/str.h>
#include <util/stream/zlib.h>

namespace NAlice {

TString ZLibCompress(TStringBuf str, ZLib::StreamType type) {
    TString result;
    TStringOutput output(result);
    TZLibCompress compress(&output, type);
    compress.Write(str);
    compress.Finish();
    return result;
}

TString ZLibDecompress(TStringBuf str) {
    TMemoryInput input(str);
    return TBufferedZLibDecompress(&input).ReadAll();
}

}
