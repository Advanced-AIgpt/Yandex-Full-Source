#pragma once

namespace NBASS {

class IGlobalContext;
class THandlersMap;

void RegisterHandlers(THandlersMap& handlers, IGlobalContext& globalCtx);

} // namespace NBASS
