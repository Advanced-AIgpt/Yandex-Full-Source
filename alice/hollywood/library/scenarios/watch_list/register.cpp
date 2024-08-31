#include "setup_add.h"
#include "setup_delete.h"
#include "process.h"

#include <alice/hollywood/library/registry/registry.h>

namespace NAlice::NHollywood::NWatchList {

    REGISTER_SCENARIO("watch_list", AddHandle<TTvWatchListAddSetupHandle>()
                                        .AddHandle<TTvWatchListDeleteSetupHandle>()
                                        .AddHandle<TTvWatchListProcessHandle>());
}
