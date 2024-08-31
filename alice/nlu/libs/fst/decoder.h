#pragma once

#include "data_loader.h"

#include <alice/bitbucket/pynorm/normalize/normalize.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <library/cpp/archive/yarchive.h>

#include <memory>

namespace NAlice {

    class TNormalizerDataDeleter {
    public:
        static inline void Destroy(norm_data_t* data) {
            norm_data_free(data);
        }
    };

    class TFstDecoder final {
    public:
        explicit TFstDecoder(const TString& normalizerDataPath);
        explicit TFstDecoder(const IDataLoader& loader);
        TFstDecoder(const IDataLoader& loader, TVector<TString> blackList);

        TString Normalize(const TString& text) const;

    private:
        THolder<norm_data_t, TNormalizerDataDeleter> NormalizerData;
        TVector<TString> BlackList;
        TVector<const char*> BlackListConverted;
    };

} // namespace NAlice
