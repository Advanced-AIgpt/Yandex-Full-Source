#include "session_context_proxy.h"
#include <util/generic/hash.h>

namespace NVoice::NExperiments {

TSessionContextGetter GetSessionContextGetter(const TStringBuf path)
{
    static const THashMap<TString, TSessionContextGetter> SESSION_CONTEXT_FIELDS({
        {".lang", &GetLang},
        {".vinsUrl", &GetVinsUrl},
        {".key", &GetAppToken},
        {".apiKey", &GetAppToken},
        {".vins.application.app_id", &GetAppId},
        {".device_model", &GetDeviceModel}
    });

    if (const TSessionContextGetter* getter = SESSION_CONTEXT_FIELDS.FindPtr(path))
        return *getter;
    return nullptr;
}

} // namespace NVoice::NExperiments
