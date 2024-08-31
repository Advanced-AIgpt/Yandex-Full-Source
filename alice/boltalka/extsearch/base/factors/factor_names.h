#pragma once

#include <alice/boltalka/extsearch/base/factors/factors_gen.h>

#include <util/generic/string.h>

class IFactorsInfo;

namespace NNlg {

const static TString BASIC_TEXT_FACTORS_NAME = "BasicTextFactors";
const static TString DSSM_COS_FACTORS_NAME_PREFIX = "DssmCosFactors_";
const static TString INFORMATIVENESS_FACTOR_NAME = "InformativenessFactor";
const static TString IS_DSSM_INDEX_FACTOR_NAME_PREFIX = "IsDssmIndexFactor_";
const static TString IS_KNN_INDEX_FACTOR_NAME_PREFIX = "IsKnnIndexFactor_";
const static TString PRONOUN_FACTORS_NAME = "PronounFactors";
const static TString RUS_LISTER_FACTORS_NAME = "RusListerFactors";
const static TString SEQ2SEQ_FACTOR_NAME = "Seq2SeqFactor";
const static TString TEXT_INTERSECTION_FACTORS_NAME = "TextIntersectionFactors";

const IFactorsInfo* GetNlgFactorsInfo();

};
