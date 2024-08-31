#include "registrator.h"

namespace NBASS {

namespace {

class TFormHandler : public IHandler {
public:
    using THandler = std::function<void(TContext&)>;

public:
    explicit TFormHandler(THandler cb)
        : Handler(cb)
    {
    }

    TResultValue Do(TRequestHandler& r) final {
        Handler(r.Ctx());
        return Nothing();
    }

private:
    THandler Handler;
};

} // namespace

void Register(THandlersMap* handlers, const TVector<TFormHandlerPair>& pairs) {
    for (const auto& p : pairs) {
        const auto handler = p.Handler;
        Y_ASSERT(handler);
        handlers->RegisterFormHandler(p.Name, [handler]() { return MakeHolder<TFormHandler>(handler); });
    }
}

} // namespace NBASS
