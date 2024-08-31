#pragma once

#include <library/cpp/float16/float16.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NAlice {

    TVector<TFloat16> PackEmbeddingToFloat16(const TVector<float>& src);
    TVector<float> UnpackEmbeddingFromFloat16(const TVector<TFloat16>& src);

    // Compatible Python code to pack/unpack embeddings:
    //
    // def embedding_to_str(v):
    //     return str(base64.b64encode(v.astype(np.float16).tobytes()), 'utf-8')
    //
    // def str_to_embedding(s):
    //     return np.frombuffer(base64.b64decode(s), dtype=np.float16)
    //
    TString PackEmbeddingToString(const TVector<float>& embedding);
    TVector<float> UnpackEmbeddingFromString(TStringBuf str);

} // NAlice
