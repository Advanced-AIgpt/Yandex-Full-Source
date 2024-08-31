#include "utils.h"
#include <library/cpp/json/json_writer.h>


namespace {

template <unsigned BytePos, typename NumType>
void AddByteAsHex(TString& dst, NumType num) {
    static const char HEX[] = "0123456789abcdef";
    dst.append(HEX[(num >> (8 * BytePos) + 4) & 0xF]);
    dst.append(HEX[(num >> 8 * BytePos) & 0xF]);
}

}  // anonymous namespace


TString GuidToUuidString(const TGUID& g)
{
    TString uuid;
    uuid.reserve(37);

    AddByteAsHex<3>(uuid, g.dw[0]);
    AddByteAsHex<2>(uuid, g.dw[0]);
    AddByteAsHex<1>(uuid, g.dw[0]);
    AddByteAsHex<0>(uuid, g.dw[0]);
    uuid.append('-');
    AddByteAsHex<3>(uuid, g.dw[1]);
    AddByteAsHex<2>(uuid, g.dw[1]);
    uuid.append('-');
    AddByteAsHex<1>(uuid, g.dw[1]);
    AddByteAsHex<0>(uuid, g.dw[1]);
    uuid.append('-');
    AddByteAsHex<3>(uuid, g.dw[2]);
    AddByteAsHex<2>(uuid, g.dw[2]);
    uuid.append('-');
    AddByteAsHex<1>(uuid, g.dw[2]);
    AddByteAsHex<0>(uuid, g.dw[2]);
    AddByteAsHex<3>(uuid, g.dw[3]);
    AddByteAsHex<2>(uuid, g.dw[3]);
    AddByteAsHex<1>(uuid, g.dw[3]);
    AddByteAsHex<0>(uuid, g.dw[3]);

    return uuid;
}


template <>
void Out<TJsonAsPretty>(IOutputStream& out, const TJsonAsPretty& x) {
    NJson::WriteJson(&out, &x.Ref, /* formatOutput = */ true, /* sortkeys = */ true);
}

template <>
void Out<TJsonAsDense>(IOutputStream& out, const TJsonAsDense& x) {
    NJson::WriteJson(&out, &x.Ref, /* formatOutput = */ false, /* sortkeys = */ false);
}

TString ExtractSessionIdFromCookie(TStringBuf cookie) {
    while (cookie) {
        TStringBuf key{StripStringLeft(cookie.NextTok("="))};
        if (key == "Session_id") {
            return TString(cookie.NextTok(";"));
        }
        cookie.NextTok(";");
    }
    return "";
}
