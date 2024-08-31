#include "versioning.h"

#include <alice/library/experiments/flags.h>

namespace NAlice {

namespace {

constexpr TStringBuf GIF_URI_PREFIX = "https://static-alice.s3.yandex.net/led-production/";
constexpr TStringBuf GIF_PATH_SEP = "/";

}

TString FormatGifVersion(const TStringBuf name, const TStringBuf suffix, const TStringBuf version, const TStringBuf subversion) {
    return FormatVersion(
        TString::Join(GIF_URI_PREFIX, name),
        suffix,
        version,
        subversion,
        /* sep = */ GIF_PATH_SEP
    );
}

} // namespace NAlice
