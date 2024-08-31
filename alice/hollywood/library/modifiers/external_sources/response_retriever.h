#pragma once

#include <alice/hollywood/library/util/service_context.h>

namespace NAlice::NHollywood::NModifiers {

    class TExternalSourcesResponseRetriever {
    public:
        explicit TExternalSourcesResponseRetriever(const NAppHost::IServiceContext& apphostContext)
            : ApphostContext_(apphostContext)
        {
        }

        template <typename TProto>
        [[nodiscard]] TMaybe<TProto> TryGetResponse(const TStringBuf type) const {
            return GetMaybeOnlyProto<TProto>(ApphostContext_, type);
        }
    private:
        const NAppHost::IServiceContext& ApphostContext_;
    };

}
