#include <alice/library/client/client_info.h>

namespace NAlice::NMegamind {
inline bool HasClassificationMusicFormulas(const TClientInfo& clientInfo) {
    // TODO(https://st.yandex-team.ru/MEGAMIND-2739) get rid of this hardcoded function and use TFormulasStorage instead
    return !clientInfo.IsElariWatch();
}
} // namespace NAlice::NMegamind
