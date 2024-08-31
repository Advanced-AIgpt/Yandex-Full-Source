#include "globals.h"

#include "exceptions.h"

namespace NAlice::NNlg {

TValue TGlobals::ResolveLoad(TStringBuf name, TStringBuf module) const {
    if (module) {
        // look inside the imported module's globals,
        // do not look in that module's imports
        // because imports are non-transitive
        auto* modulePtr = Imports.FindPtr(module);
        if (!modulePtr) {
            ythrow TImportError() << "Module not registered: " << module;
        }

        if (auto* ptr = (*modulePtr)->Globals.FindPtr(name)) {
            return *ptr;
        }

        return TValue::Undefined();
    }

    if (auto* ptr = Globals.FindPtr(name)) {
        return *ptr;
    }

    if (auto* globalsName = FromImports.FindPtr(name)) {
        // look inside the imported module's gloabls,
        // do not look into that module's imports
        // because imports are non-transitive
        const TGlobals* importedGlobals = globalsName->first;
        const TString& importedName = globalsName->second;
        if (auto* ptr = importedGlobals->Globals.FindPtr(importedName)) {
            return *ptr;
        }
    }

    return TValue::Undefined();
}

TValue& TGlobals::ResolveStore(TStringBuf name) {
    // XXX(a-square): if no assignment is made, the value remains TUndefined
    return Globals[name];
}

void TGlobals::RegisterImport(TStringBuf path, const TGlobals& globals) {
    Imports[path] = &globals;
}

void TGlobals::RegisterFromImport(TStringBuf name, TStringBuf target, const TGlobals& globals) {
    FromImports[target] = std::pair(&globals, TString{name});
}

void TGlobals::Freeze() {
    for (auto& [key, value] : Globals) {
        value.Freeze();
    }
}

TValue TGlobalsChain::ResolveLoad(TStringBuf name, TStringBuf module) const {
    if (Top && !module) {
        TValue result = Top->ResolveLoad(name, {});
        if (!result.IsUndefined()) {
            return result;
        }
    }

    if (Bottom) {
        return Bottom->ResolveLoad(name, module);
    }

    return TValue::Undefined();
}

} // namespace NAlice::NNlg
