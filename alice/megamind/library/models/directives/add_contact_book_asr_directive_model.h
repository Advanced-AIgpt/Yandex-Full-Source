#pragma once

#include <alice/megamind/library/models/directives/uniproxy_directive_model.h>

namespace NAlice::NMegamind {

class TAddContactBookAsrDirectiveModel final : public virtual TUniproxyDirectiveModel {
public:
    TAddContactBookAsrDirectiveModel();

    void Accept(IModelSerializer& serializer) const final;
};

} // namespace NAlice::NMegamind
