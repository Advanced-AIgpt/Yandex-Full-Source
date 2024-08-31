#include "error.h"

#include <util/generic/maybe.h>

template <>
void Out<NBASS::TError>(IOutputStream& out, const NBASS::TError& err) {
    out << TStringBuf("error '") << ToString(err.Type) << '\'';
    if (!err.Msg.empty()) {
        out << TStringBuf(", msg: '") << err.Msg << '\'';
    }
}

template <>
void Out<TMaybe<NBASS::TError>>(IOutputStream& out, const TMaybe<NBASS::TError>& err) {
    if (err)
        out << *err;
    else
        out << TStringBuf("no error");
}
