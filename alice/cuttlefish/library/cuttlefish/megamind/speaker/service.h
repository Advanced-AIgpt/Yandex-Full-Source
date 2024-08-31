#pragma once

#include <alice/cuttlefish/library/cuttlefish/megamind/speaker/context.h>
#include <alice/cuttlefish/library/protos/personalization.pb.h>
#include <apphost/lib/proto_answers/http.pb.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    class IActiveSpeakerService {
    public:
        [[nodiscard]] virtual TAtomicSharedPtr<TSpeakerContext> GetActiveSpeaker() const = 0;
    };

    class ISpeakerDataService {
    public:
        using TTicket = int;

        virtual void OnMatch(NAliceProtocol::TMatchVoiceprintResult matchResult, TTicket ticket) = 0;
        virtual void OnNoMatch() = 0;
        virtual void OnDatasyncResponse(NAppHostHttp::THttpResponse response, TTicket ticket) = 0;
        virtual void OnBlackboxResponse(NAppHostHttp::THttpResponse response, TTicket ticket) = 0;

        virtual ~ISpeakerDataService() = default;
    };

    class TSpeakerService : public ISpeakerDataService
                          , public IActiveSpeakerService {
    public:
        using TSpeakerContextPtr = TAtomicSharedPtr<TSpeakerContext>;

        static constexpr int OWNER = 0;
        static constexpr int NOBODY = -1;

    public:
        TSpeakerService();

        void OnMatch(NAliceProtocol::TMatchVoiceprintResult matchResult, TTicket ticket) override;
        void OnNoMatch() override;
        void OnDatasyncResponse(NAppHostHttp::THttpResponse response, TTicket ticket) override;
        void OnBlackboxResponse(NAppHostHttp::THttpResponse response, TTicket ticket) override;
        [[nodiscard]] TAtomicSharedPtr<TSpeakerContext> GetActiveSpeaker() const override;

    private:
        void ChangeActiveSpeaker(TTicket ticket);
        TSpeakerContextPtr GetOrCreate(TTicket ticket);

    private:
        TVector<TSpeakerContextPtr> Speakers;
        int CurrentActiveSpeaker;
    };
}