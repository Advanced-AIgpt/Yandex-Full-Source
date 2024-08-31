#pragma once

#include <util/generic/fwd.h>
#include <library/cpp/yson/node/node_io.h>

namespace NAlice::NHollywoodFw::NOrder { 

NJson::TJsonValue GetCustomJsonResponse(const TString& flag);

}