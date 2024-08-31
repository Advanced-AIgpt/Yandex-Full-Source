#pragma once

#include <alice/megamind/library/config/config.h>
#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/request_composite/event.h>
#include <alice/megamind/library/request_composite/view.h>
#include <alice/megamind/protos/speechkit/request.pb.h>

#include <alice/library/client/client_features.h>
#include <alice/library/util/status.h>

#include <library/cpp/http/io/headers.h>
#include <library/cpp/uri/uri.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <functional>

namespace NAlice {

namespace NTestSuiteTestRequest {
struct TTestCaseTestDialogovoScenarioName;
}

class TSpeechKitUtils;

} // namespace NAlice

namespace NAlice::NMegamind {

class TSpeechKitInitContext;

// TODO (petrk) Split it into small pieces!
class TRequestParts {
public:
    struct TView {
        TView(const TRequestParts& partsRef)
            : PartsRef{partsRef}
        {
        }

        const THttpHeaders& HttpHeaders() const {
            return PartsRef.HttpHeaders();
        }

        const TSpeechKitRequestProto* operator->() const {
            return &Proto();
        }
        const TSpeechKitRequestProto& operator*() const {
            return Proto();
        }
        const TSpeechKitRequestProto& Proto() const {
            return PartsRef.Proto();
        }

        const TString& Path() const {
            return PartsRef.Path();
        }

        // Generates a hash of proto fields that uniquely determine the request's identity.
        ui64 GetSeed() const {
            return PartsRef.GetSeed();
        }

        bool GetVoiceSession() const {
            return PartsRef.Proto().GetRequest().GetVoiceSession();
        }

        bool GetResetSession() const {
            return PartsRef.Proto().GetRequest().GetResetSession();
        }

        const TString& RequestId() const {
            return Proto().GetHeader().GetRequestId();
        }

        const TString& PrevReqId() const {
            return Proto().GetHeader().GetPrevReqId();
        }

        ui32 SequenceNumber() const {
            return Proto().GetHeader().GetSequenceNumber();
        }

        const TClientInfoProto& ClientInfoProto() const {
            return Proto().GetApplication();
        }

        const auto& ExperimentsProto() const {
            return Proto().GetRequest().GetExperiments();
        }

    protected:
        friend class NAlice::TSpeechKitUtils;
        friend class TVinsRequestBuilder;

    private:
        const TRequestParts& PartsRef;
    };

public:
    TRequestParts(TRequestParts&&) = default;
    TRequestParts& operator=(TRequestParts&&) = default;

    static TErrorOr<TRequestParts> Create(TSpeechKitInitContext& ctx, TRequestComponentsView<TEventComponent> view);
    virtual ~TRequestParts() = default;

    const THttpHeaders& HttpHeaders() const {
        return Headers_;
    }

    const TSpeechKitRequestProto& Proto() const {
        return *Proto_;
    }

    TSpeechKitRequestProto& Proto() {
        return *Proto_;
    }

    const TString& Path() const {
        return Path_;
    }

    ui64 GetSeed() const {
        return Seed_;
    }

protected:
    TRequestParts(TSpeechKitInitContext& ctx, TRequestComponentsView<TEventComponent> event);

private:
    TSimpleSharedPtr<TSpeechKitRequestProto> Proto_;
    THttpHeaders Headers_;
    TString Path_;
    ui64 Seed_ = 42;
};

} // namespace NAlice::NMegamind
