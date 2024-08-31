#pragma once

#include <alice/tests/difftest/joker_light/library/core/ydb.h>
#include <alice/tests/difftest/joker_light/library/core/config.h>

#include <util/folder/path.h>
#include <util/generic/noncopyable.h>

namespace NAlice::NJokerLight {

class TContext : NNonCopyable::TNonCopyable {
public:
    TContext(const TFsPath& configFilePath);

    const TConfig& Config() const {
        return Config_;
    }

    TYdb& Ydb() {
        return Ydb_;
    }

private:
    const TConfig Config_;
    TYdb Ydb_;
};

} // namespace NAlice::NJokerLight
