#include "avatars.h"

#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <util/charset/utf8.h>
#include <util/generic/hash.h>
#include <util/string/ascii.h>
#include <library/cpp/string_utils/quote/quote.h>

namespace {

const THashMap<TStringBuf, TStringBuf> AvatarFilenameMapping{
        {TStringBuf("а"), TStringBuf("a")},
        {TStringBuf("б"), TStringBuf("b")},
        {TStringBuf("в"), TStringBuf("v")},
        {TStringBuf("г"), TStringBuf("g")},
        {TStringBuf("д"), TStringBuf("d")},
        {TStringBuf("е"), TStringBuf("e")},
        {TStringBuf("ё"), TStringBuf("jo")},
        {TStringBuf("ж"), TStringBuf("zh")},
        {TStringBuf("з"), TStringBuf("z")},
        {TStringBuf("и"), TStringBuf("i")},
        {TStringBuf("й"), TStringBuf("jj")},
        {TStringBuf("к"), TStringBuf("k")},
        {TStringBuf("л"), TStringBuf("l")},
        {TStringBuf("м"), TStringBuf("m")},
        {TStringBuf("н"), TStringBuf("n")},
        {TStringBuf("о"), TStringBuf("o")},
        {TStringBuf("п"), TStringBuf("p")},
        {TStringBuf("р"), TStringBuf("r")},
        {TStringBuf("с"), TStringBuf("s")},
        {TStringBuf("т"), TStringBuf("t")},
        {TStringBuf("у"), TStringBuf("u")},
        {TStringBuf("ф"), TStringBuf("f")},
        {TStringBuf("х"), TStringBuf("kh")},
        {TStringBuf("ц"), TStringBuf("c")},
        {TStringBuf("ч"), TStringBuf("ch")},
        {TStringBuf("ш"), TStringBuf("sh")},
        {TStringBuf("щ"), TStringBuf("shh")},
        {TStringBuf("ь"), TStringBuf("mz")},
        {TStringBuf("ъ"), TStringBuf("tz")},
        {TStringBuf("ы"), TStringBuf("y")},
        {TStringBuf("э"), TStringBuf("eh")},
        {TStringBuf("ю"), TStringBuf("ju")},
        {TStringBuf("я"), TStringBuf("ja")}
};

TStringBuf GetPrefix(char symbol) {
    if (IsAsciiDigit(symbol))
        return "dig_";
    if (IsAsciiAlpha(symbol))
        return "eng_";
    return "rus_";
}

TString GetFilename(TStringBuf fio) {
    if (fio.empty())
        return {};

    TStringBuf firstSymbol = SubstrUTF8(fio, 0, 1);
    const TString key = ToLowerUTF8(firstSymbol);
    auto iter = AvatarFilenameMapping.find(key);

    Y_ASSERT(!key.empty());
    return TString{GetPrefix(key[0])} + (iter != AvatarFilenameMapping.end() ? iter->second : key);
}

} // anonymous namespace

namespace NBASS {
namespace NContacts {
namespace NAvatars {

TString MakeUrl(const NSc::TValue& contact, const TContext& ctx) {
    const NSc::TArray& phones = contact["phones"].GetArray();
    if (phones.empty()) {
        LOG(ERR) << "Client sent empty field 'phone' for contact, uuid=" << ctx.Meta().UUID() << Endl;
        return "";
    }

    const auto id = phones[0]["contact_id"];
    const auto name = GetFilename(contact["name"]);
    const TAvatar* avatar = ctx.Avatar(TStringBuf("computer_vision"), name);

    TString fallback = (avatar ? avatar->Https : "");
    CGIEscape(fallback);
    TStringBuilder result;
    result << "avatar://contact?id=" << id << "&fallback_url=" << fallback;

    return result;
}

} // namespace NAvatars
} // namespace NContacts
} // namespace NBASS
