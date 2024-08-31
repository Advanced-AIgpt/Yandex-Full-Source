#include "defs.h"

#include <util/stream/output.h>

namespace {
struct TOutputVisitor {
    explicit TOutputVisitor(IOutputStream& out)
        : Out(out) {
    }

    template <typename T>
    void operator()(const T& t) {
        Out << t;
    }

    IOutputStream& Out;
};

} // namespace

template <>
void Out<NBASS::NVideo::TSerialIndex>(IOutputStream& out, const NBASS::NVideo::TSerialIndex& index) {
    std::visit(TOutputVisitor{out}, index);
}
