#pragma once

#include <alice/gproxy/library/gproxy/config.pb.h>


namespace NGProxy {

class TApplicationConfig {
public:
    TApplicationConfig(int argc, const char **argv);

    inline const TAppHostClientConfig& GetAppHostClientConfig() const {
        return Config_.GetAppHost();
    }

    inline const TGrpcServerConfig& GetGrpcServerConfig() const {
        return Config_.GetGrpc();
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
    TServiceConfig Config_; 
    bool           IsBadConfig_ { false };
};

}
