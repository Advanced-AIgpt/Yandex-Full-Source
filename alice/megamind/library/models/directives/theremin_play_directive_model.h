#pragma once

#include <alice/megamind/library/models/directives/client_directive_model.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <utility>

enum class EThereminPlayDirectiveSet {
    ExternalSet /* "external_set" */,
    InternalSet /* "internal_set" */
};

namespace NAlice::NMegamind {

class IThereminPlayDirectiveSetModel : public IModel, public virtual TThrRefBase {
public:
    [[nodiscard]] virtual EThereminPlayDirectiveSet GetType() const = 0;
};

class TThereminPlayDirectiveExternalSetSampleModel final {
public:
    explicit TThereminPlayDirectiveExternalSetSampleModel(const TString& url);

    [[nodiscard]] const TString& GetUrl() const;

private:
    const TString Url;
};

class TThereminPlayDirectiveExternalSetModel final : public virtual IThereminPlayDirectiveSetModel {
public:
    TThereminPlayDirectiveExternalSetModel(bool noOverlaySamples, bool repeatSoundInside, bool stopOnCeil,
                                           TVector<TThereminPlayDirectiveExternalSetSampleModel> samples);

    void Accept(IModelSerializer& serializer) const final;
    [[nodiscard]] EThereminPlayDirectiveSet GetType() const final;

    [[nodiscard]] bool GetRepeatSoundInside() const;
    [[nodiscard]] bool GetNoOverlaySamples() const;
    [[nodiscard]] bool GetStopOnCeil() const;
    [[nodiscard]] const TVector<TThereminPlayDirectiveExternalSetSampleModel>& GetSamples() const;

private:
    const bool NoOverlaySamples;
    const bool RepeatSoundInside;
    const bool StopOnCeil;

    const TVector<TThereminPlayDirectiveExternalSetSampleModel> Samples;
};

class TThereminPlayDirectiveInternalSetModel final : public virtual IThereminPlayDirectiveSetModel {
public:
    explicit TThereminPlayDirectiveInternalSetModel(int mode);

    void Accept(IModelSerializer& serializer) const final;
    [[nodiscard]] EThereminPlayDirectiveSet GetType() const final;

    [[nodiscard]] int GetMode() const;

private:
    const int Mode;
};

class TThereminPlayDirectiveModel final : public virtual TClientDirectiveModel {
public:
    // TODO(alkapov): replace intrusive model with 2 constructors
    TThereminPlayDirectiveModel(const TString& analyticsType, TIntrusivePtr<IThereminPlayDirectiveSetModel> set);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const TIntrusivePtr<IThereminPlayDirectiveSetModel>& GetSet() const;

private:
    const TIntrusivePtr<IThereminPlayDirectiveSetModel> Set;
};

} // namespace NAlice::NMegamind
