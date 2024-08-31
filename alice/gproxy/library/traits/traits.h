#pragma once

#include <util/generic/typelist.h>


namespace NGProxyTraits {

template <typename T>
class TGProxyService {
public:
    static constexpr const char *ServiceName = "stub";

    using TMethodList = TTypeList<>;
};

}
