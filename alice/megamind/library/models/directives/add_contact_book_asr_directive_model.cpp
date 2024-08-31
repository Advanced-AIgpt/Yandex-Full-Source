#include "add_contact_book_asr_directive_model.h"

namespace NAlice::NMegamind {

TAddContactBookAsrDirectiveModel::TAddContactBookAsrDirectiveModel()
    : TUniproxyDirectiveModel("add_contact_book_asr") {
}

void TAddContactBookAsrDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

} // namespace NAlice::NMegamind
