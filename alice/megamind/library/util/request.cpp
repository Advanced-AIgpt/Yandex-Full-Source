#include "request.h"

#include <util/stream/str.h>


namespace NAlice {
namespace NMegamind {

TDialogIdSplit SplitDialogId(const TString& dialogId) {
    TDialogIdSplit split{/* ScenarioName= */ "", /* ScenarioDialogId= */ dialogId};
    if (dialogId.Contains(':')) {
        TStringStream stream(dialogId);
        split.ScenarioName = stream.ReadTo(':');
        split.ScenarioDialogId = stream.ReadAll();
    }
    return split;
}

} // namespace NMegamind
} // namespace NAlice
