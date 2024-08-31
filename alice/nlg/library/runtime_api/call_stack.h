#pragma once

#include <util/generic/deque.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>

namespace NAlice::NNlg {

// WARNING(a-square): do not give a stack frame non-fixed strings,
// it will probably outlive whatever scope you're in
struct TCallStackFrame {
    TStringBuf Name;
    TStringBuf Template;
    TMaybe<i64> Line;
};

using TCallStack = TDeque<TCallStackFrame>;

}  // namespace NAlice::NNlg
