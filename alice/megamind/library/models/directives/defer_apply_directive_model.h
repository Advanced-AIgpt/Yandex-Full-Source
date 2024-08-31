#pragma once

#include <alice/megamind/library/models/directives/uniproxy_directive_model.h>

namespace NAlice::NMegamind {

class TDeferApplyDirectiveModel final : public TUniproxyDirectiveModel {
public:
    explicit TDeferApplyDirectiveModel(const TString& session);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const TString& GetSession() const;

private:
    const TString Session;
};

} // namespace NAlice::NMegamind
