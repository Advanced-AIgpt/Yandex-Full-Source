#include "nlg_library_registry.h"
#include <util/generic/singleton.h>

namespace NAlice::NNlg {

void TNlgLibraryRegistry::AddNlgLibraryRegisterFunction(const TStringBuf nlgLibraryPath, TRegisterFunction registerFunction) {
    Y_ENSURE(registerFunction, "Invalid register function for nlg library " << nlgLibraryPath);
    const auto [it, inserted] = Registrations_.emplace(TString(nlgLibraryPath), std::move(registerFunction));
    Y_ENSURE(inserted, "Duplicated registration for nlg library " << nlgLibraryPath);
}

TNlgLibraryRegistry::TRegisterFunction TNlgLibraryRegistry::TryGetRegisterFunctionByNlgLibraryPath(const TStringBuf nlgLibraryPath) const {
    return Registrations_.Value(nlgLibraryPath, TRegisterFunction());
}

TNlgLibraryRegistry& TNlgLibraryRegistry::Instance() {
    return *Singleton<TNlgLibraryRegistry>();
}

TNlgLibraryRegistrator::TNlgLibraryRegistrator(const TStringBuf nlgLibraryPath, TNlgLibraryRegistry::TRegisterFunction registerFunction) {
    TNlgLibraryRegistry::Instance().AddNlgLibraryRegisterFunction(nlgLibraryPath, std::move(registerFunction));
}

} // namespace NAlice::NNlg
