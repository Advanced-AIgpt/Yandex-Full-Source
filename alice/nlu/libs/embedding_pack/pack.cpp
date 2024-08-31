#include "pack.h"
#include <library/cpp/string_utils/base64/base64.h>
#include <util/stream/str.h>
#include <util/string/join.h>
#include <util/ysaveload.h>

namespace NAlice {

    TVector<TFloat16> PackEmbeddingToFloat16(const TVector<float>& src) {
        TVector<TFloat16> dst(src.size());
        NFloat16Ops::PackFloat16SequenceAuto(src.data(), dst.data(), dst.size());
        return dst;
    }

    TVector<float> UnpackEmbeddingFromFloat16(const TVector<TFloat16>& src) {
        TVector<float> dst(src.size());
        NFloat16Ops::UnpackFloat16SequenceAuto(src.data(), dst.data(), dst.size());
        return dst;
    }

    TString PackEmbeddingToString(const TVector<float>& embedding) {
        TString bytes;
        TStringOutput output(bytes);
        for (const TFloat16 item : PackEmbeddingToFloat16(embedding)) {
            Save(&output, item.Save());
        }
        return Base64Encode(bytes);
    }

    TVector<float> UnpackEmbeddingFromString(TStringBuf str) {
        const TString bytes = Base64DecodeUneven(str);
        TStringInput input(bytes);
        const size_t size = bytes.length() / 2;
        TVector<TFloat16> embedding(size);
        for (TFloat16& item : embedding) {
            Load(&input, item.Data);
        }
        return UnpackEmbeddingFromFloat16(embedding);
    }

} // NAlice
