#include "antirobot.h"
#include "blackbox.h"
#include "cachalot_mm_session.h"
#include "contacts.h"
#include "datasync.h"
#include "flags_json.h"
#include "iot.h"
#include "laas.h"
#include "memento.h"
#include "selector.h"
#include "starter.h"


namespace NAlice::NCuttlefish::NAppHostServices {

    void StartContextLoad(
        const NVoicetech::NUniproxy2::TMessage& message,
        const NAliceProtocol::TSessionContext& sessionContext,
        const NAliceProtocol::TRequestContext& requestContext,
        NAppHost::TServiceContextPtr appHostContext
    ) {
        const TContextLoadSourceSelector selector(requestContext);

        #define SETUP_SOURCE_IMPL(SourceName, Target)                                                       \
            do {                                                                                            \
                if (selector.Target.Use##SourceName) {                                                      \
                    Setup##SourceName##For##Target(message, sessionContext, requestContext, appHostContext);\
                }                                                                                           \
            } while (false)

        #define SETUP_SOURCE_FOR_GUEST(SourceName) SETUP_SOURCE_IMPL(SourceName, Guest)
        #define SETUP_SOURCE_FOR_OWNER(SourceName) SETUP_SOURCE_IMPL(SourceName, Owner)

        SETUP_SOURCE_FOR_GUEST(Blackbox);
        SETUP_SOURCE_FOR_GUEST(Datasync);
        SETUP_SOURCE_FOR_OWNER(Antirobot);
        SETUP_SOURCE_FOR_OWNER(Blackbox);
        SETUP_SOURCE_FOR_OWNER(CachalotMMSession);
        SETUP_SOURCE_FOR_OWNER(ContactsJson);
        SETUP_SOURCE_FOR_OWNER(ContactsProto);
        SETUP_SOURCE_FOR_OWNER(Datasync);
        SETUP_SOURCE_FOR_OWNER(FlagsJson);
        SETUP_SOURCE_FOR_OWNER(IotUserInfo);
        SETUP_SOURCE_FOR_OWNER(Laas);
        SETUP_SOURCE_FOR_OWNER(Memento);

        #undef SETUP_SOURCE_FOR_OWNER
        #undef SETUP_SOURCE_FOR_GUEST
        #undef SETUP_SOURCE_FOR_IMPL
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices
