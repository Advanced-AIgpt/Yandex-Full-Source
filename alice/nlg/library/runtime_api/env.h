#pragma once

#include "call_stack.h"
#include "caller.h"
#include "globals.h"
#include "postprocess.h"
#include "text.h"
#include "translations.h"
#include "value.h"

#include <alice/library/util/rng.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/array_ref.h>
#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>

#include <functional>
#include <memory>

namespace NAlice::NNlg {

class TPhraseNotFoundError : public yexception {
public:
    using yexception::yexception;
};

class TCardNotFoundError : public yexception {
public:
    using yexception::yexception;
};

TValue TransformForm(TValue&& rawForm);

class TEnvironment;

// call context, only includes stuff that won't be changed in the nested calls
struct TCallCtx {
    const TEnvironment& Env;
    IRng& Rng;
    TCallStack& CallStack;
    TStringBuf Language;
};

// call context with mutable environment, needed for variable initialization
struct TMutableCallCtx {
    TEnvironment& Env;
    IRng& Rng;
    TCallStack& CallStack;
    TStringBuf Language;

    // neede to implicitly convert to non-mutable context when calling a macro
    // during initialization
    operator TCallCtx() const {
        return {Env, Rng, CallStack, Language};
    }
};

struct TPhraseCardParams {
    TValue Context = TValue::Dict();
    TValue Form = TValue::Dict();
    TValue ReqInfo = TValue::Dict();
};

class TEnvironment {
public:
    using TInitPtr = void (*)(const TMutableCallCtx&, TGlobals&);
    using TMacroZeroArgsPtr = void (*)(const TCallCtx& ctx, const TCaller*, const TGlobalsChain*, IOutputStream&);

public:
    void RegisterInit(TStringBuf module, TInitPtr callback) {
        Inits[module] = callback;
    }

    void SetTranslationsContainer(ITranslationsContainerPtr translationsContainer) {
        TranslationsContainer_ = std::move(translationsContainer);
    }

    void InitializeAllGlobals(IRng& rng);

    void RegisterPhrase(TStringBuf templateId, TStringBuf phrase, TMacroZeroArgsPtr callback) {
        std::pair key(templateId, phrase);
        Phrases[key] = callback;
    }

    void RegisterCard(TStringBuf templateId, TStringBuf phrase, TMacroZeroArgsPtr callback) {
        std::pair key(templateId, phrase);
        Cards[key] = callback;
    }

    TGlobals& InitializeGlobals(const TMutableCallCtx& ctx, TStringBuf module);
    const TGlobals& GetGlobals(TStringBuf module) const;

    bool HasPhrase(const TStringBuf templateId, const TStringBuf phraseId, const ELanguage language) const {
        return static_cast<bool>(GetPhrase(templateId, phraseId, language));
    }

    bool HasCard(const TStringBuf templateId, const TStringBuf cardId, const ELanguage language) const {
        return static_cast<bool>(GetCard(templateId, cardId, language));
    }

    TPhraseOutput RenderPhrase(TStringBuf templateId, TStringBuf phrase, const ELanguage language, IRng& rng,
                               TPhraseCardParams&& params) const;
    NJson::TJsonValue RenderCard(const TStringBuf templateId, const TStringBuf card, const ELanguage language, IRng& rng,
                                 TPhraseCardParams&& params, const bool reduceWhitespace = false) const;

    const TString& GetTranslation(const ELanguage language, const TStringBuf key) const {
        Y_ENSURE(TranslationsContainer_, "Translations container is not set");
        return TranslationsContainer_->GetTranslation(language, key);
    }

private:
    TText RenderMacro(TStringBuf templateId, TMacroZeroArgsPtr macro, const ELanguage language, TStringBuf frameName,
                      IRng& rng, TPhraseCardParams&& params) const;
    TMacroZeroArgsPtr GetPhrase(TStringBuf templateId, TStringBuf phrase, const ELanguage language) const;
    TMacroZeroArgsPtr GetCard(TStringBuf templateId, TStringBuf card, const ELanguage language) const;

private:
    THashMap<TString, TGlobals> Globals;
    THashMap<TString, TInitPtr> Inits;
    THashMap<std::pair<TString, TString>, TMacroZeroArgsPtr> Phrases;
    THashMap<std::pair<TString, TString>, TMacroZeroArgsPtr> Cards;
    ITranslationsContainerPtr TranslationsContainer_;
};

} // namespace NAlice::NNlg
