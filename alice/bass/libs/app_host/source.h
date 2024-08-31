#pragma once

#include <library/cpp/scheme/scheme.h>

namespace NBASS {

class IAppHostSource {
public:
    virtual ~IAppHostSource() = default;
    virtual NSc::TValue GetSourceInit() const = 0;
};

class TGenericAppHostSource : public IAppHostSource {
public:
    TGenericAppHostSource(NSc::TValue data);
    NSc::TValue GetSourceInit() const override;

private:
    NSc::TValue Data;
};

} // NBASS
