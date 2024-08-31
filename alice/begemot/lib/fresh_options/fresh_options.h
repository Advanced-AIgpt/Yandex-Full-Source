#include <alice/begemot/lib/fresh_options/proto/fresh_options.pb.h>

#include <search/begemot/apphost/context.h>

namespace NAlice {

void ReadAliceFreshOptions(const NBg::TInput& input, TFreshOptions* options);
void ReadGranetFreshOptions(const NBg::TInput& input, TFreshOptions* options);

bool ShouldUseFreshForForm(const TFreshOptions& freshOptions, TStringBuf name);

} // namespace NAlice
