#include <alice/hollywood/library/scenarios/image_what_is_this/nlg/register.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/image_what_is_this_handler.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/image_what_is_this_continue_handler.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/image_what_is_this_render.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/image_what_is_this_int_handler.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/image_what_is_this_resources.h>
#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/registry/registry.h>

namespace NAlice::NHollywood::NScenarios {

REGISTER_SCENARIO("image_what_is_this", AddHandle<NImage::TImageWhatIsThisHandle>()
                                        .AddHandle<NImage::TImageWhatIsThisContinueHandle>()
                                        .AddHandle<NImage::TImageWhatIsThisIntHandle>()
                                        .AddHandle<NImage::TImageWhatIsThisRender>()
                                        .SetResources<NImage::TImageWhatIsThisResources>()
                                        .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NImageWhatIsThis::NNlg::RegisterAll));

}
