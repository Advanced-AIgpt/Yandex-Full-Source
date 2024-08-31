#include "news.h"
#include "news_fast_data.h"
#include "prepare_handle.h"
#include "render_handle.h"

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/news/nlg/register.h>
#include <alice/hollywood/library/scenarios/news/proto/news.pb.h>
#include <alice/hollywood/library/scenarios/news/proto/news_fast_data.pb.h>

#include <alice/hollywood/library/registry/registry.h>

namespace NAlice::NHollywood {

REGISTER_SCENARIO("news", AddHandle<TBassNewsPrepareHandle>()
                          .AddHandle<TBassNewsRenderHandle>()
                          .SetNlgRegistration(
                          NAlice::NHollywood::NLibrary::NScenarios::NNews::NNlg::RegisterAll));

} // namespace NAlice::NHollywood

namespace NAlice::NHollywoodFw {

HW_REGISTER(TNewsScenarioDispatch);

}
