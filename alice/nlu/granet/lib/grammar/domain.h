#pragma once

#include <alice/nlu/granet/lib/grammar/proto/domain.pb.h>
#include <alice/nlu/libs/tuple_like_type/tuple_like_type.h>

#include <library/cpp/langs/langs.h>

namespace NGranet {

struct TGranetDomain {
    ELanguage Lang = LANG_RUS;
    bool IsPASkills = false;
    bool IsWizard = false;
    bool IsSnezhana = false;

    DECLARE_TUPLE_LIKE_TYPE(TGranetDomain, Lang, IsPASkills, IsWizard, IsSnezhana);

    bool IsAlice() const;

    TString GetLangName() const;
    TString GetDirName() const;
    bool TryFromDirName(TStringBuf name);

    void WriteToProto(NProto::TGranetDomain* proto) const;
    static TGranetDomain ReadFromProto(const NProto::TGranetDomain& proto);
};

IOutputStream& operator<<(IOutputStream& out, const TGranetDomain& domain);

} // namespace NGranet

template <>
struct THash<NGranet::TGranetDomain>: public TTupleLikeTypeHash {
};
