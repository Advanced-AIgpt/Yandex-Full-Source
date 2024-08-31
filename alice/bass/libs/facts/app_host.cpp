#include "app_host.h"

namespace NBASS {

TFactsAppHostSource::TFactsAppHostSource(TStringBuf part, TStringBuf tld, TStringBuf uil) {
    NSc::TValue& globalCtxJson = Data["global_ctx"];
    globalCtxJson["fact_experiment"].Push("dialogs");
    globalCtxJson["part"].Push(part);
    globalCtxJson["service"].Push("assistant.yandex");
    globalCtxJson["tld"].Push(tld);
    globalCtxJson["uil"].Push(uil);
    Data["sub_source"].SetString("SUGGESTFACTS2");
    Data["type"].SetString("suggestfacts2_setup");
}

NSc::TValue TFactsAppHostSource::GetSourceInit() const {
    return Data;
}

} // namespace NBASS
