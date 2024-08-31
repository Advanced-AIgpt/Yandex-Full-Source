#include "common.h"

#include <alice/nlu/libs/fst/fst_base.h>
#include <alice/nlu/libs/fst/fst_normalizer.h>
#include <alice/nlu/libs/fst/prefix_data_loader.h>
#include <alice/nlu/libs/fst/tokenize.h>

#include <util/string/cast.h>

namespace NAlice {

    namespace NTestHelpers {

    TFstNormalizer CreateNormalizer(ELanguage lang) {
        const TString name = IsoNameByLanguage(lang); // ru, tr
        return TFstNormalizer{
            TPrefixDataLoader{NFst::GetDataRoot() + "/denormalizer/" + name},
            TPrefixDataLoader{NFst::GetDataRoot() + "/normalizer/" + name}
        };
    }

    } // namespace NTestHeleprs

} // namespace NAlice
