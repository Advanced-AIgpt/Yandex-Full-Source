#include "utils.h"

#include <alice/cuttlefish/library/logging/dlog.h>

#include <library/cpp/json/json_value.h>


bool EnsureVinsExperimentsFormat(NJson::TJsonValue* flags) {
    bool ok = true;

    if (flags->IsArray()) {
        NJson::TJsonValue map(NJson::JSON_MAP);
        for (const NJson::TJsonValue& it : flags->GetArray()) {
            if (it.IsString()) {
                map.InsertValue(it.GetString(), "1");
            } else {
                DLOG("Experiment flag is not a string but " << it.GetType());
                ok = false;
            }
        }
        *flags = std::move(map);
    } else if (!flags->IsMap()) {
        DLOG("Experiment flags isn't Array neither Map but " << flags->GetType() << ", set empty Map");
        flags->SetType(NJson::JSON_MAP);
        ok = false;
    }

    return ok;
}
