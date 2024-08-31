#pragma once

#include "scenario_env.h"

#include <alice/library/request/slot.h>
#include <alice/megamind/library/context/context.h>

#include <kernel/factor_storage/factor_storage.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {

class TScenario {
public:
    enum class EApplyMode {
        Skip /* "skip" */,
        Call /* "call" */,
        Continue /* "continue" */
    };

    explicit TScenario(TStringBuf name)
        : Name(TString{name})
    {
    }

    virtual ~TScenario() = default;

    const TString& GetName() const {
        return Name;
    }

    virtual bool IsEnabled(const IContext& /* ctx */) const {
        return false;
    }

    virtual bool IsProtocol() const {
        return false;
    }

    virtual TVector<TString> GetAcceptedFrames() const = 0;

    virtual bool AcceptsAnyUtterance() const {
        return false;
    }

    virtual bool AcceptsImageInput() const {
        return false;
    }

    virtual bool AcceptsMusicInput() const {
        return false;
    }

    virtual bool DependsOnWebSearchResult() const {
        return false;
    }

    virtual TVector<EDataSourceType> GetDataSources() const {
        return {};
    }

    virtual TVector<EDataSourceType> GetRequiredDataSources() const {
        return {};
    }

    virtual bool IsLanguageSupported(const ELanguage& /* language */) const {
        return true;
    }

    virtual bool AlwaysRecieveAllParsedSemanticFrames() const {
        return false;
    }

private:
    const TString Name;
};

} // namespace NAlice
