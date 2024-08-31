#include "utils.h"

#include <util/string/subst.h>
#include <util/string/builder.h>

#include <util/string/cast.h>
#include <util/string/printf.h>
#include <util/string/split.h>
#include <util/string/join.h>

namespace NAlice::NHollywood::NSearch {

namespace {

static constexpr TStringBuf HL_OPEN_TAG = "\007[";
static constexpr TStringBuf HL_CLOSE_TAG = "\007]";

} // namespace

TString RemoveHighlight(TStringBuf str) {
    if (str.empty()) {
        return TString();
    }
    TString s{TString{str}};
    SubstGlobal(s, HL_OPEN_TAG, TStringBuf());
    SubstGlobal(s, HL_CLOSE_TAG, TStringBuf());
    return s;
}

bool AddDivCardImage(const TString& src, ui16 w, ui16 h, NJson::TJsonValue& card) {
    if (src.size()) {
        NJson::TJsonValue image;
        image["src"] = src.StartsWith('/') ? TStringBuilder() << "https:" << src : src;
        image["w"] = w;
        image["h"] = h;
        card["image"] = image;
        return true;
    }
    return false;
}

TString CreateAvatarIdImageSrc(const NJson::TJsonValue& cardData, ui16 w, ui16 h) {
    if (TStringBuf id = cardData["avatar_id"].GetString()) {
        return TStringBuilder() << "https://avatars.mds.yandex.net/get-entity_search/" << id << "/S" << w << "x" << h;
    }
    return TString();
}

TString GetHostName(const NJson::TJsonValue& snippet) {
    const auto* text = snippet.GetValueByPath(TStringBuf("path.items.[0].text"));
    if (text) {
        return text->GetString();
    } else {
        return TString();
    }
}

TString ParseHostName(const TString& url) {
    TVector<TStringBuf> values;
    StringSplitter(url).Split('/').Take(3).AddTo(&values);
    return JoinSeq(TStringBuf("/"), values);
}

TString ForceString(const NJson::TJsonValue& value) {
    if (value.IsString() || value.IsInteger() || value.IsUInteger() || value.IsDouble()) {
        return value.GetStringRobust();
    }

    return TString();
}

bool IsNumber(const NJson::TJsonValue& value) {
    return value.IsInteger() || value.IsUInteger() || value.IsDouble();
}

long long ForceInteger(const NJson::TJsonValue& value) {
     if (value.IsString() || value.IsInteger() || value.IsUInteger() || value.IsDouble()) {
        return value.GetIntegerRobust();
    }

    return 0;
}

void AppendFormattedTime(ui32 value, TStringBuf* names, TStringBuilder& dst) {
    dst << value;
    if (value > 10 && value < 20) {
        dst << names[2];
    } else {
        switch (value % 10) {
            case 1:
                dst << names[0];
                break;
            case 2:
            case 3:
            case 4:
                dst << names[1];
                break;
            default:
                dst << names[2];
        }
    }
}

TString FormatTimeDifference(i32 diff, TStringBuf tld) {
    ui32 absDiff = abs(diff);
    ui32 hours = absDiff / 60;
    ui32 minutes = absDiff % 60;

    if (tld != TStringBuf("ru")) {
        return Sprintf("%.2d:%.2d", (diff < 0 ? -hours : hours), minutes);
    }

    if (diff == 0) {
        return "Нет разницы во времени";
    }

    static TStringBuf Hours[] = {" час" , " часа", " часов"};
    static TStringBuf Minutes[] = {" минута" , " минуты", " минут"};

    TStringBuilder text;

    if (diff < 0) {
        text << "-";
    }

    if (hours > 0) {
        AppendFormattedTime(hours, Hours, text);
    }

    if (minutes > 0) {
        if (hours > 0) {
            text << " ";
        }
        AppendFormattedTime(minutes, Minutes, text);
    }

    return text;
}

TString TryRoundFloat(const TString& value) {
    float floatValue;
    if (!TryFromString<float>(value, floatValue)) {
        return value;
    }
    size_t dotPos = value.find('.');
    if (dotPos == TString::npos || value.size() - dotPos <= 5) {
        return value;
    }
    return Sprintf("примерно %.4f", floatValue);
}

TString JoinListFact(const TString& text, const TVector<TString>& items, bool isOrdered, bool isTts) {
    TStringBuilder builder;
    builder << text;
    TString delim = isTts ? " sil<[0]> " : (isOrdered ? "" : " - ");
    for (size_t i = 0; i < items.size(); i++) {
        if (!isTts) {
            builder << "\n";
        }
        if (delim.Empty()) {  // ordered list
            builder << ToString(i + 1) << ". ";
        } else {
            builder << delim;
        }
        builder << items[i];
    }
    return builder;
}

} // namespace NAlice::NHollywood
