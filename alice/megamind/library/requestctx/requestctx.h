#pragma once

#include "common.h"
#include "fwd.h"
#include "stage_timers.h"

#include <alice/megamind/library/config/protos/classification_config.pb.h>
#include <alice/megamind/library/config/protos/config.pb.h>
#include <alice/megamind/library/globalctx/fwd.h>
#include <alice/megamind/library/util/http_response.h>

#include <alice/megamind/library/apphost_request/protos/request_meta.pb.h>

#include <alice/library/logger/logger.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/uri/uri.h>

#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>

#include <functional>
#include <memory>

class THttpHeaders;

namespace NAlice {

class TRequestCtx : private NNonCopyable::TNonCopyable {
public:
    using EContentType = NMegamindAppHost::TRequestMeta::EContentType;
    using TRequestMeta = NMegamindAppHost::TRequestMeta;

    class IInitializer : public NNonCopyable::TMoveOnly {
    public:
        using TStageTimersPtr = std::unique_ptr<NMegamind::TStageTimers>;

    public:
        virtual ~IInitializer() = default;

        // Returned logger must be owned by somebody else.
        virtual TRTLogger& Logger() = 0;

        virtual TCgiParameters StealCgi() = 0;
        virtual NUri::TUri StealUri() = 0;
        virtual THttpHeaders StealHeaders() = 0;
        // TODO (petrk) Get rid off default implementation when apphost will prevails!
        virtual TStageTimersPtr StealStageTimers();
    };

    struct TBadRequestException : public yexception {};

public:
    TRequestCtx(IGlobalCtx& globalCtx, IInitializer&& initializer);
    virtual ~TRequestCtx() = default;

    IGlobalCtx& GlobalCtx();

    const IGlobalCtx& GlobalCtx() const;

    const TConfig& Config() const;
    const NMegamind::TClassificationConfig& ClassificationConfig() const;

    TRTLogger& RTLogger();

    NMegamind::TStageTimers& StageTimers() {
        return *StageTimers_;
    }

    EContentType ContentType() const {
        return ContentType_;
    }

public:
    const TCgiParameters& Cgi() const {
        return Cgi_;
    }

    const THttpHeaders& Headers() const {
        return Headers_;
    }

    const NUri::TUri& Uri() const {
        return Uri_;
    }

    // TODO (petrk) Get rid of the method when apphost take overs the http!
    virtual THolder<IHttpResponse> CreateHttpResponse() const = 0;
    virtual const TString& Body() const = 0;
    virtual TStringBuf NodeLocation() const = 0;

private:
    IGlobalCtx& GlobalCtx_;
    TRTLogger& Logger_;
    IInitializer::TStageTimersPtr StageTimers_;

protected:
    TCgiParameters Cgi_;
    THttpHeaders Headers_;
    NUri::TUri Uri_;
    EContentType ContentType_;
};

void AddSensorForHttpResponseCode(NMetrics::ISensors& sensors, int code);

} // namespace NAlice
