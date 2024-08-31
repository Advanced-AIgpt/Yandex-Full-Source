#include "domain.h"
#include <util/string/builder.h>

namespace NGranet {

namespace {
    const TStringBuf PASKILLS_PREFIX = "paskills_";
    const TStringBuf WIZARD_PREFIX = "wizard_";
    const TStringBuf SNEZHANA_PREFIX = "snezhana_";
}

bool TGranetDomain::IsAlice() const {
    return !IsPASkills && !IsWizard && !IsSnezhana;
}

TString TGranetDomain::GetLangName() const {
    return IsoNameByLanguage(Lang);
}

TString TGranetDomain::GetDirName() const {
    TStringBuilder str;
    if (IsPASkills) {
        str << PASKILLS_PREFIX;
    } else if (IsWizard) {
        str << WIZARD_PREFIX;
    } else if (IsSnezhana) {
        str << SNEZHANA_PREFIX;
    }
    str << GetLangName();
    return str;
}

bool TGranetDomain::TryFromDirName(TStringBuf name) {
    IsPASkills = false;
    IsWizard = false;
    IsSnezhana = false;
    if (name.SkipPrefix(PASKILLS_PREFIX)) {
        IsPASkills = true;
    } else if (name.SkipPrefix(WIZARD_PREFIX)) {
        IsWizard = true;
    } else if (name.SkipPrefix(SNEZHANA_PREFIX)) {
        IsSnezhana = true;
    }
    Lang = LanguageByName(name);
    return Lang != LANG_UNK;
}

void TGranetDomain::WriteToProto(NProto::TGranetDomain* proto) const {
    Y_ENSURE(proto);
    proto->SetLanguage(Lang);
    proto->SetIsPASkills(IsPASkills);
    proto->SetIsWizard(IsWizard);
    proto->SetIsSnezhana(IsSnezhana);
}

// static
TGranetDomain TGranetDomain::ReadFromProto(const NProto::TGranetDomain& proto) {
    return {
        .Lang = static_cast<ELanguage>(proto.GetLanguage()),
        .IsPASkills = proto.GetIsPASkills(),
        .IsWizard = proto.GetIsWizard(),
        .IsSnezhana = proto.GetIsSnezhana()
    };
}

IOutputStream& operator<<(IOutputStream& out, const TGranetDomain& domain) {
    out << domain.GetDirName();
    return out;
}

} // namespace NGranet
