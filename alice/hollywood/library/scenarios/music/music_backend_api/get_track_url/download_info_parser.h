#pragma once

#include "download_info.h"

#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood::NMusic {

TDownloadInfoOptions ParseDownloadInfo(const TStringBuf jsonString, TRTLogger& logger);

} // namespace NAlice::NHollywood::NMusic
