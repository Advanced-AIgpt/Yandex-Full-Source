#pragma once

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <functional>

namespace NAlice::NNlg {

class TEnvironment;

class TNlgLibraryRegistry {
public:
    using TRegisterFunction = std::function<void(TEnvironment&)>;
public:
    void AddNlgLibraryRegisterFunction(const TStringBuf nlgLibraryPath, TRegisterFunction registerFunction);
    TRegisterFunction TryGetRegisterFunctionByNlgLibraryPath(const TStringBuf nlgLibraryPath) const;
public:
    static TNlgLibraryRegistry& Instance();
private:
    THashMap<TString, TRegisterFunction> Registrations_;
};

struct TNlgLibraryRegistrator {
    explicit TNlgLibraryRegistrator(const TStringBuf nlgLibraryPath, TNlgLibraryRegistry::TRegisterFunction registerFunction);
};

} // namespace NAlice::NNlg

#define REGISTER_NLG_LIBRARY(nlgLibraryPath, registerFunction) \
    static const NAlice::NNlg::TNlgLibraryRegistrator \
    Y_GENERATE_UNIQUE_ID(NlgLibraryRegistrator)((nlgLibraryPath), (registerFunction))
