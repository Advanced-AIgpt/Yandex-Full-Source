#pragma once

#include <util/generic/hash.h>
#include <util/generic/string.h>

struct TAvatar {
    TAvatar(TStringBuf http, TStringBuf https);

    const TString Http;
    const TString Https;
};

class TAvatarsMap {
public:
    /** Load avatars from internal resources
     */
    TAvatarsMap();
    /** Load avatars from a given file
     */
    TAvatarsMap(TStringBuf fileName);

    const TAvatar* Get(TStringBuf ns, TStringBuf key) const;

private:
    const THashMap<TString, TAvatar> Map;
};
