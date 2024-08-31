#include "logging.h"

#include <alice/megamind/library/experiments/flags.h>

#include <kernel/factor_storage/factor_storage.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/hash.h>
#include <util/string/builder.h>

namespace NAlice::NMegamind::NFeatures {

namespace {

enum class ESerializationMode {
    Compressed,
    HumanReadable,
};

TQualityStorage SerializeFactorStorage(const TFactorStorage& storage, const ESerializationMode mode) {
    TQualityStorage qualityStorage;

    switch (mode) {
        case ESerializationMode::Compressed: {
            TStringStream strStream;
            NFSSaveLoad::Serialize(storage, &strStream);
            qualityStorage.SetFactorStorage(Base64Encode(strStream.Str()));
            break;
        }
        case ESerializationMode::HumanReadable: {
            TStringBuilder strBuilder;
            for (ui32 i = 0; i < storage.Size(); ++i) {
                strBuilder << (i == 0 ? "" : "\t") << storage[i];
            }
            qualityStorage.SetFactorStorage(strBuilder);
            break;
        }
    }

    return qualityStorage;
}

}

void LogFeatures(const IContext& ctx, const TFactorStorage& factorStorage, TQualityStorage& qualityStorage) {
    const ui64 idHash = THash<TString>{}(ctx.ClientInfo().Uuid);
    bool shouldRandomLog = idHash % 100 == 66;
    if (ctx.HasExpFlag(EXP_ENABLE_RANKING_FEATURES_LOGGING) || shouldRandomLog) {
        const ESerializationMode mode = ctx.HasExpFlag(EXP_HR_RANKING_FEATURES)
            ? ESerializationMode::HumanReadable
            : ESerializationMode::Compressed;
        const TQualityStorage qs = SerializeFactorStorage(factorStorage, mode);
        qualityStorage.MergeFrom(qs);
    }
}

}
