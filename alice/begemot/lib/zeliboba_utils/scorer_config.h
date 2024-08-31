#pragma once

#include <alice/boltalka/generative/inference/core/model.h>
#include <search/begemot/core/filesystem.h>

NGenerativeBoltalka::TGenerativeBoltalka::TParams LoadScoringModelConfig(const NBg::TFileSystem& fs,
                                                                         const TStringBuf& zeliboba_model_config_file);
