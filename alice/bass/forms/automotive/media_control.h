#pragma once

#include <alice/bass/forms/vins.h>

namespace NBASS {
namespace NAutomotive {

TResultValue HandleMediaControl(TContext& ctx, const TStringBuf& command);
TResultValue HandleMediaControlSource(TContext& ctx, const TStringBuf& command);

}
}
