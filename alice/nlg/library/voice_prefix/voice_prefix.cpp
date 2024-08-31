#include "voice_prefix.h"

namespace NAlice::NNlg {

TStringBuf GetVoicePrefixForLanguage(const ELanguage language) {
    switch (language) {
    case ELanguage::LANG_ARA:
        return TStringBuf(R"(<speaker voice="arabic.gpu" lang="ar">)");
    default:
        return TStringBuf();
    }
}

}
