#include "search_convert.h"

#include <util/string/subst.h>
#include <util/string/util.h>

namespace NAlice {

TString ConvertUuidForSearch(TString uuid) {
    RemoveAll(uuid, '-');
    return uuid;
}

TString ConvertUserAgentForSearch(TString userAgent) {
    SubstGlobal(userAgent, "Yandex", "yandex");
    return userAgent;
}

} // namespace NAlice
