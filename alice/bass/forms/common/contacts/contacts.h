#pragma once

#include <alice/bass/forms/common/contacts/contacts.sc.h>
#include <alice/bass/forms/context/context.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/hash.h>

namespace NContact {
    using TFio = NBassApi::TFio<TSchemeTraits>;
    using TRawContact = NBassApi::TRawContact<TSchemeTraits>;
    using TPhone = NBassApi::TPhone<TSchemeTraits>;
    using TContact = NBassApi::TContact<TSchemeTraits>;
    using TContactSearchQuery = NBassApi::TContactSearchQuery<TSchemeTraits>;
}

namespace NBASS {

class TContacts {
public:
    TContacts(const NSc::TValue& items, const TContext& context);

    NSc::TValue Preprocess(const TSlot& recipient);

private:
    void MakePhonesUnique(NSc::TValue& phones);

    bool IsGrouped() const;
    THashMap<i64, NSc::TValue> GroupBy(TStringBuf attribute = "contact_id") const;
    void Reorder(TVector<NSc::TValue>& items, TStringBuf relevantText) const;
    void Filter(const TUtf16String& surname);

private:
    NSc::TValue Items;
    const TContext& Ctx;
};

NSc::TValue GetFindContactsPayload(const TContext& ctx);

} // namespace NBASS
