#include "version.h"

namespace NAlice {

NJson::TJsonValue CreateVersionData() {
    NJson::TJsonValue result;
    result["branch"] = GetBranch();
    result["tag"] = GetTag();
    return result;
}

} // namespace NAlice
