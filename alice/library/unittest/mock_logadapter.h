#pragma once

#include <alice/library/logger/logadapter.h>

namespace NAlice {

class TMockLogAdapter final : public TLogAdapter {
public:
    void LogImpl(TStringBuf msg, const TSourceLocation& location, ELogAdapterType type) const override {
        Cerr << type << ": " << location.File << ":" << location.Line << " " << msg;
    }
};

} // namespace NAlice
