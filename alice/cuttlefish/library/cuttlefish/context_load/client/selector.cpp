#include "selector.h"

#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>


using namespace NAlice::NCuttlefish::NExpFlags;

namespace NAlice::NCuttlefish::NAppHostServices {

    TGuestContextLoadSourceSelector::TGuestContextLoadSourceSelector(
        const NAliceProtocol::TRequestContext& requestContext
    )
        : TGuestContextLoadSourceSelector(requestContext.GetAdditionalOptions().GetIgnoreGuestContext())
    {
    }

    TGuestContextLoadSourceSelector::TGuestContextLoadSourceSelector(
        const bool ignoreContexts
    )
        : UseBlackbox(!ignoreContexts)
        , UseDatasync(!ignoreContexts)
    {
    }

    TOwnerContextLoadSourceSelector::TOwnerContextLoadSourceSelector(
        const NAliceProtocol::TRequestContext& requestContext
    )
        : TOwnerContextLoadSourceSelector(
            requestContext.GetAdditionalOptions().GetIgnoreSecondaryContext(),
            requestContext.GetPredefinedResults().HasMegamindSession(),
            ExperimentFlagHasTrueValue(requestContext, "use_contacts"),
            ExperimentFlagHasTrueValue(requestContext, "contacts_as_proto"),
            ExperimentFlagHasTrueValue(requestContext, DISREGARD_UAAS)
        )
    {
    }

    TOwnerContextLoadSourceSelector::TOwnerContextLoadSourceSelector(
        const bool ignoreSecondaryContexts,
        const bool hasPredefinedMegamindSession,
        const bool useContacts,
        const bool useContactsAsProto,
        const bool disregardUaas
    )
        : UseCachalotMMSession(!ignoreSecondaryContexts && !hasPredefinedMegamindSession)
        , UseContactsJson(!ignoreSecondaryContexts && useContacts)
        , UseContactsProto(!ignoreSecondaryContexts && useContactsAsProto)
        , UseDatasync(!ignoreSecondaryContexts)
        // TODO (paxakor): make it right after next release (uniproxy-147).
        , UseFlagsJson(true || !disregardUaas)
        , UseIotUserInfo(!ignoreSecondaryContexts)
        , UseMemento(!ignoreSecondaryContexts)
    {
    }

    TContextLoadSourceSelector::TContextLoadSourceSelector(
        const NAliceProtocol::TRequestContext& requestContext
    )
        : Guest(requestContext)
        , Owner(requestContext)
    {
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices
