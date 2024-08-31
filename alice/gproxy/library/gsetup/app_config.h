#pragma once

#include <alice/gproxy/library/gsetup/config.pb.h>


namespace NGProxy {

using namespace NGSetup;


class TApplicationConfig {
public:
    TApplicationConfig(int argc, const char **argv);

    inline const TServantConfig& GetServantConfig() const {
        return Config_.GetServant();
    }
    inline const THttpServerConfig& GetHttpServerConfig() const {
        return Config_.GetHttp();
    }

    inline const TLoggingConfig& GetLoggingConfig() const {
        return Config_.GetLogging();
    }

    inline bool GetNoMLock() const {
        return Config_.GetNoMLock();
    }

    inline bool IsBad() const {
        return IsBadConfig_;
    }

    inline bool IsGood() const {
        return !IsBad();
    }

private:
    NGSetup::TServiceConfig Config_;
    bool           IsBadConfig_ { false };
};

}
