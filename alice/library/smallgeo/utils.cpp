#include "utils.h"

TString NAlice::NSmallGeo::RemoveParentheses(const TString& name) {
    size_t posOpen = name.find('(');
    size_t posClose = name.find(')');

    if (posOpen != TString::npos && posOpen != 0 && posClose == name.size() - 1) {
        return Strip(name.substr(0, posOpen));
    }
    return name;
}
