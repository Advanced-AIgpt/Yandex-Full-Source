#pragma once

#include <alice/cuttlefish/library/protos/session.pb.h>


namespace NAlice::NCuttlefish::NAppHostServices {

    class TGuestContextLoadSourceSelector {
    public:
        explicit TGuestContextLoadSourceSelector(
            const NAliceProtocol::TRequestContext& requestContext
        );

    private:
        explicit TGuestContextLoadSourceSelector(
            const bool ignoreContexts
        );

    public:
        const bool UseBlackbox = true;
        const bool UseDatasync = true;
    };


    class TOwnerContextLoadSourceSelector {
    public:
        explicit TOwnerContextLoadSourceSelector(
            const NAliceProtocol::TRequestContext& requestContext
        );

    private:
        explicit TOwnerContextLoadSourceSelector(
            const bool ignoreSecondaryContexts,
            const bool hasPredefinedMegamindSession,
            const bool useContacts,
            const bool useContactsAsProto,
            const bool disregardUaas
        );

    public:
        const bool UseAntirobot = true;
        const bool UseBlackbox = true;
        const bool UseCachalotMMSession = true;
        const bool UseContactsJson = false;
        const bool UseContactsProto = false;
        const bool UseDatasync = true;
        const bool UseFlagsJson = true;
        const bool UseIotUserInfo = true;
        const bool UseLaas = true;
        const bool UseMemento = true;
    };


    class TContextLoadSourceSelector {
    public:
        explicit TContextLoadSourceSelector(
            const NAliceProtocol::TRequestContext& requestContext
        );

    public:
        const TGuestContextLoadSourceSelector Guest;
        const TOwnerContextLoadSourceSelector Owner;
    };

}  // namespace NAlice::NCuttlefish::NAppHostServices
