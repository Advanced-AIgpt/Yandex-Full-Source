#pragma once

#include <util/generic/ptr.h>

namespace NHttpFetcher {

template <class TResponse>
class IRequestHandle;

class TFetcher;
class THandle;
class TProxySettings;
class TRequest;
class TRequestBuilder;
class TResponse;

using TProxySettingsPtr = TIntrusiveConstPtr<TProxySettings>;
using TRequestPtr = THolder<TRequest>;

} // namespace NHttpFetcher
