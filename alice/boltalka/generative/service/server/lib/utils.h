#pragma once

#include <alice/boltalka/generative/inference/core/model.h>
#include <alice/boltalka/generative/service/server/config/config.pb.h>


using namespace NGenerativeBoltalka;

TConfig LoadConfig(int argc, const char** argv);

TGenerativeBoltalka::TParams ParseBoltalkaParams(const TConfig::TGenerativeBoltalka& boltalkaConfig);
