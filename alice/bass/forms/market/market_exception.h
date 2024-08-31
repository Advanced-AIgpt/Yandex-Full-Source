#pragma once

#include <alice/bass/util/error.h>

namespace NBASS {

namespace NMarket {

class TMarketException: public TErrorException {
public:
    explicit TMarketException(const TStringBuf& msg);

    const TBackTrace* BackTrace() const noexcept override;

private:
#if !defined(NDEBUG)
    TBackTrace BT_;
#endif
};

// implementaion

inline TMarketException::TMarketException(const TStringBuf& msg)
    : TErrorException(NBASS::TError::EType::MARKETERROR, TStringBuilder() << msg << Endl)
{
#if !defined(NDEBUG)
    BT_.Capture();
#endif
}

inline const TBackTrace* TMarketException::BackTrace() const noexcept {
#if !defined(NDEBUG)
    return &BT_;
#else
    return 0;
#endif
}

} // namespace NMarket

} // namespace NBASS
