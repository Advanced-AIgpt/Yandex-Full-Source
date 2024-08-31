#include "find_contacts_directive_model.h"

namespace NAlice::NMegamind {

TFindContactsDirectiveModel::TRequestPart::TRequestPart(const TString& tag)
    : Tag(tag) {
}

TFindContactsDirectiveModel::TRequestPart& TFindContactsDirectiveModel::TRequestPart::AddValue(const TString& value) {
    Values.push_back(value);
    return *this;
}

const TString& TFindContactsDirectiveModel::TRequestPart::GetTag() const {
    return Tag;
}

const TVector<TString>& TFindContactsDirectiveModel::TRequestPart::GetValues() const {
    return Values;
}

// TFindContactsDirectiveModel -------------------------------------------------
void TFindContactsDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TVector<TString>& TFindContactsDirectiveModel::GetMimeTypeWhiteListColumn() const {
    return MimeTypeWhiteListColumn;
}

const TVector<TString>& TFindContactsDirectiveModel::GetMimeTypeWhiteListName() const {
    return MimeTypeWhiteListName;
}

const google::protobuf::Struct& TFindContactsDirectiveModel::GetOnPermissionDeniedCallbackPayload() const {
    return OnPermissionDeniedCallbackPayload;
}

const TVector<TFindContactsDirectiveModel::TRequestPart>& TFindContactsDirectiveModel::GetRequest() const {
    return Request;
}

const TVector<TString>& TFindContactsDirectiveModel::GetValues() const {
    return Values;
}

} // namespace NAlice::NMegamind
