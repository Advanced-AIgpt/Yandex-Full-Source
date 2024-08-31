#include "config.h"

#include <alice/joker/library/log/log.h>

#include <util/stream/file.h>

namespace NAlice::NShooter {

TConfig::TConfig(const TString& configFileName)
    : NSc::TValue{NSc::TValue::FromJsonThrow(TFileInput(configFileName).ReadAll())}
    , TScheme{this}
{
    auto validate = [](TStringBuf path, TStringBuf error) {
        LOG(ERROR) << "Config: " << path << " : " << error << Endl;
    };
    if (!Validate(/* path = */ "", /* strict = */ false, validate)) {
        ythrow yexception() << "Config " << configFileName.Quote() << " is invalid";
    }
}

} // namespace NAlice::NShooter
