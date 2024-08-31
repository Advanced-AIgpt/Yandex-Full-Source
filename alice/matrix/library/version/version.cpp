#include "version.h"

#include <build/scripts/c_templates/svnversion.h>

#include <util/string/cast.h>


namespace NMatrix {

namespace {

TStringBuf GetShortStr(TStringBuf st) {
    if (st.size() >= 10) {
        return st.substr(0, 10);
    }
    return st;
}

static const TString VERSION = []() -> TString {
    TString branch = GetBranch();
    TString hash = TString(GetShortStr(GetProgramHash()));

    static constexpr TStringBuf releasesPrefix = "releases/";
    if (auto pos = branch.find(releasesPrefix); pos != TString::npos) {
        pos += releasesPrefix.size();
        return TString::Join(
            branch.substr(pos),
            '@', ToString(GetArcadiaPatchNumber()),
            '~', hash
        );
    } else {
        return TString::Join(
            ((branch.find("trunk") != TString::npos) ? "trunk" : GetShortStr(branch)),
            '@', ToString(GetArcadiaPatchNumber()),
            '~', hash
        );
    }
}();

} // namespace

const TString& GetVersion() {
    return VERSION;
}

} // namespace NMatrix
