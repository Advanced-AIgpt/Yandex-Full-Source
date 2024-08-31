#pragma once

#include <alice/cuttlefish/library/proto_configs/cuttlefish.cfgproto.pb.h>

#include <alice/cuttlefish/library/logging/log_context.h>

#include <apphost/api/service/cpp/service.h>

#include <util/generic/vector.h>
#include <util/generic/strbuf.h>

namespace NAlice::NCuttlefish {

    using TRequestServiceHandler = std::function<void(NAppHost::IServiceContext&, TLogContext)>;
    using TStreamServiceHandler = std::function<NThreading::TPromise<void>(TIntrusivePtr<NAppHost::IServiceContext>, TLogContext)>;

    struct TServiceHandler {
        TServiceHandler(TStringBuf path, TRequestServiceHandler requestHandler)
            : Path(path)
            , Type("request")
            , RequestHandler(requestHandler)
        {}
        TServiceHandler(TStringBuf path, TStreamServiceHandler streamHandler)
            : Path(path)
            , Type("stream")
            , StreamHandler(streamHandler)
        {}

        TStringBuf Path;
        TStringBuf Type;
        TRequestServiceHandler RequestHandler;
        TStreamServiceHandler StreamHandler;
    };

    TVector<TServiceHandler> GetAppHostServices(const NAliceCuttlefishConfig::TConfig& config);

}  // namespace NAlice::NCuttlefish
