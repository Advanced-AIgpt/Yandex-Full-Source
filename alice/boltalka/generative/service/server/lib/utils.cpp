#include "utils.h"

#include <alice/scenarios/lib/http_service.h>

#include <library/cpp/getoptpb/getoptpb.h>
#include <util/folder/path.h>


TConfig LoadConfig(int argc, const char** argv) {
    TString errorMsg;
    TConfig config;
    NGetoptPb::TGetoptPbSettings settings{};

    NGetoptPb::TGetoptPb configPathOpt(settings);
    configPathOpt.AddOptions(config);
    Y_ENSURE(configPathOpt.ParseArgs(argc, argv, config, errorMsg),
             "Can not parse command line options and/or prototext config, explanation: " << errorMsg);

    const TFsPath pathToConfig = configPathOpt.GetOptsParseResult().Get(settings.ConfPathLong);
    const TFsPath configFolderPath = pathToConfig.Dirname();

    for (auto mutableGenerativeBoltalka : *config.MutableGenerativeBoltalkas()) {
        const auto& folder = mutableGenerativeBoltalka.GetFolder();
        if (!TFsPath(folder).Exists()) {
            mutableGenerativeBoltalka.SetFolder(configFolderPath / folder);
        }
    }

    if (settings.DumpConfig) {
        Cerr << "Using settings:\n"
             << "====================\n";
        configPathOpt.DumpMsg(config, Cerr);
        Cerr << "====================\n";
    }

    const auto& httpServerConfig = config.GetHttpServerConfig();
    Y_ENSURE(
        httpServerConfig.GetPort() > 0 && httpServerConfig.GetPort() < Max<ui16>(),
        "HttpServer->Port must be > 0 and < 65536"
    );

    return config;
}

TGenerativeBoltalka::TParams ParseBoltalkaParams(const NGenerativeBoltalka::TConfig::TGenerativeBoltalka& boltalkaConfig) {
    TGenerativeBoltalka::TParams generativeBoltalkaParams;
    generativeBoltalkaParams.ModelParams.SamplingStrategy = boltalkaConfig.GetSamplingStrategy();
    generativeBoltalkaParams.ModelParams.ReturnAllBeamSearchHypothesis = boltalkaConfig.GetReturnAllBeamSearchHypothesis();
    generativeBoltalkaParams.ModelParams.IncludeLateHypotheses = boltalkaConfig.GetIncludeLateHypotheses();
    generativeBoltalkaParams.ModelParams.ReturnUnfinishedHypotheses = boltalkaConfig.GetReturnUnfinishedHypotheses();

    if (boltalkaConfig.NGramPenaltySize() > 0) {
        for (const auto& params: boltalkaConfig.GetNGramPenalty()) {
            generativeBoltalkaParams.ModelParams.NGramPenaltyParams.push_back({params.GetNGramSize(), params.GetMaxPenalty(),
                                                                                        params.GetMinPenalty(), params.GetExpDecayRate(),
                                                                                        params.GetLinearDecayRate()});
        }
    }

    for (const auto& bannedTokenString : boltalkaConfig.GetBannedTokensToGenerate()) {
        generativeBoltalkaParams.ModelParams.BannedTokensToGenerate.push_back(bannedTokenString);
    }
    for (const auto& bannedTokenId : boltalkaConfig.GetBannedTokenIdsToGenerate()) {
        generativeBoltalkaParams.ModelParams.BannedTokenIdsToGenerate.push_back(bannedTokenId);
    }
    generativeBoltalkaParams.ModelParams.AllowBanUnkTokenToGenerate = boltalkaConfig.GetAllowBanUnkTokenToGenerate();

    generativeBoltalkaParams.ModelParams.ShouldUseGpus = boltalkaConfig.HasGpuBackend();
    if (generativeBoltalkaParams.ModelParams.ShouldUseGpus) {
        const auto &gpuIds = boltalkaConfig.GetGpuBackend().GetGpuIds();
        generativeBoltalkaParams.ModelParams.GpuIds.insert(generativeBoltalkaParams.ModelParams.GpuIds.begin(),
                                                            gpuIds.begin(), gpuIds.end());
    } else {
        generativeBoltalkaParams.ModelParams.NumCpuBackends = boltalkaConfig.GetCpuBackend().GetNumBackends();
    }
    generativeBoltalkaParams.ModelParams.NumThreadsPerSession = boltalkaConfig.GetNumThreadsPerSession();
    generativeBoltalkaParams.ModelParams.BatchSize = boltalkaConfig.GetBatchSize();
    generativeBoltalkaParams.ModelParams.BeamSize = boltalkaConfig.GetBeamSize();

    auto topNLogits = boltalkaConfig.GetSamplingTopNLogits();
    if (topNLogits != 0) {
        generativeBoltalkaParams.ModelParams.SamplingTopNLogits = topNLogits;
    } else {
        generativeBoltalkaParams.ModelParams.SamplingTopNLogits = Nothing();
    }
    generativeBoltalkaParams.ModelParams.SamplingTemperature = boltalkaConfig.GetSamplingTemperature();
    generativeBoltalkaParams.ModelParams.SamplingNucleus = boltalkaConfig.GetSamplingNucleus();
    generativeBoltalkaParams.ModelParams.MaxGenerationMsModel = boltalkaConfig.GetMaxGenerationMsModel();
    generativeBoltalkaParams.ModelParams.MaxGenerationMsExecutor = boltalkaConfig.GetMaxGenerationMsExecutor();
    generativeBoltalkaParams.ModelParams.MaxInpLen = boltalkaConfig.GetMaxInputLen();
    generativeBoltalkaParams.ModelParams.MaxOutLen = boltalkaConfig.GetModelMaxOutLen();
    generativeBoltalkaParams.ModelParams.MinOutLen = boltalkaConfig.GetModelMinOutLen();

    generativeBoltalkaParams.FilterParams.FilterDuplicateWords = boltalkaConfig.GetFilterDuplicateWords();
    generativeBoltalkaParams.FilterParams.FilterDuplicateNGrams = boltalkaConfig.GetFilterDuplicateNGrams();
    generativeBoltalkaParams.FilterParams.FilterByUniqueWordsRatio = boltalkaConfig.GetFilterByUniqueWordsRatio();
    generativeBoltalkaParams.FilterParams.FilterEmpty = boltalkaConfig.GetFilterEmptyHypotheses();
    generativeBoltalkaParams.FilterParams.FilterBadWords = boltalkaConfig.GetFilterBadWords();
    generativeBoltalkaParams.FilterParams.BadWordsAreRegexps = boltalkaConfig.GetBadWordsAreRegexps();

    auto& postprocessorParams = boltalkaConfig.GetPostProcessorParams();
    generativeBoltalkaParams.PostProcessorParams.AddHyphens = postprocessorParams.GetAddHyphens();
    generativeBoltalkaParams.PostProcessorParams.FixPunctuation = postprocessorParams.GetFixPunctuation();
    generativeBoltalkaParams.PostProcessorParams.Capitalize = postprocessorParams.GetCapitalize();
    for (const auto& [k, v] : postprocessorParams.GetPostProcessMapping()) {
        generativeBoltalkaParams.PostProcessorParams.PostProcessMapping[k] = v;
    }

    generativeBoltalkaParams.TokenizerParams.RemoveHyphensFromInput = boltalkaConfig.GetRemoveHyphensFromInput();
    generativeBoltalkaParams.TokenizerParams.ShouldAddCTRLToken = boltalkaConfig.GetShouldAddCTRLToken();
    generativeBoltalkaParams.TokenizerParams.HasSnippetInContext = boltalkaConfig.GetHasSnippetInContext();
    generativeBoltalkaParams.TokenizerParams.CTRLToken = boltalkaConfig.GetCTRLToken();
    generativeBoltalkaParams.TokenizerParams.Prefix = boltalkaConfig.GetTokenizerPrefix();
    generativeBoltalkaParams.TokenizerParams.AliceName = boltalkaConfig.GetTokenizerAliceName();
    generativeBoltalkaParams.TokenizerParams.UserName = boltalkaConfig.GetTokenizerUserName();
    generativeBoltalkaParams.TokenizerParams.TurnSeparator = boltalkaConfig.GetTokenizerTurnSeparator();
    generativeBoltalkaParams.TokenizerParams.NameSeparator = boltalkaConfig.GetTokenizerNameSeparator();
    generativeBoltalkaParams.TokenizerParams.BeforeInfoText = boltalkaConfig.GetTokenizerBeforeInfoText();
    generativeBoltalkaParams.TokenizerParams.BeforePrefixText = boltalkaConfig.GetTokenizerBeforePrefixText();
    generativeBoltalkaParams.TokenizerParams.BeforeSuffixText = boltalkaConfig.GetTokenizerBeforeSuffixText();

    TFsPath folder(boltalkaConfig.GetFolder());
    generativeBoltalkaParams.TokenizerParams.BpeVocPath = folder / "bpe.voc";
    generativeBoltalkaParams.TokenizerParams.TokenToIdVocPath = folder / "token_to_id.voc";
    generativeBoltalkaParams.TokenizerParams.TokenizerType = boltalkaConfig.GetTokenizerType();
    generativeBoltalkaParams.TokenizerParams.MaxLen = boltalkaConfig.GetMaxInputLen();
    generativeBoltalkaParams.TokenizerParams.TruncatePolicy = boltalkaConfig.GetTruncatePolicy();
    TFsPath modelPath = folder / "model.npz";
    generativeBoltalkaParams.ModelParams.CheckpointPath = modelPath;
    TFsPath ptunePath = folder / "ptune.npz";
    if (ptunePath.Exists()) {
        Cerr << "INFO: Loading p-tune embeddings from " << ptunePath << Endl;
        generativeBoltalkaParams.PtunePath = ptunePath;
    } else {
        Cerr << "INFO: Skipping p-tune embeddings cause file doesn't exist: " << ptunePath << Endl;
    }
    generativeBoltalkaParams.ModelParams.MaxPtuneLen = boltalkaConfig.GetMaxPtuneLen();
    generativeBoltalkaParams.ModelParams.CheckpointMemoryMode = boltalkaConfig.GetPrechargeModelBlob() ? EMemoryMode::Precharged : EMemoryMode::Locked;
    generativeBoltalkaParams.ModelParams.UsePrefixedInference = boltalkaConfig.GetUsePrefixedInference();
    TFsPath badDictPath = folder / "bad_dict.txt";
    if (!badDictPath.Exists()) {
        Cerr << "INFO: Skipping bad dict path cause file doesn't exist: " << ptunePath << Endl;
        generativeBoltalkaParams.FilterParams.FilterBadWords = false;
    } else {
        generativeBoltalkaParams.FilterParams.BadWordsDictPath = badDictPath;
    }

    generativeBoltalkaParams.ModelParams.GenerateShorter = boltalkaConfig.GetGenerateShorter();
    generativeBoltalkaParams.ModelParams.BoostEos = boltalkaConfig.GetBoostEos();

    generativeBoltalkaParams.ModelParams.SawtoothBoost = boltalkaConfig.GetSawtoothBoost();
    generativeBoltalkaParams.ModelParams.SawtoothTokenId = boltalkaConfig.GetSawtoothTokenId();
    generativeBoltalkaParams.ModelParams.SawtoothBoostMin = boltalkaConfig.GetSawtoothBoostMin();
    generativeBoltalkaParams.ModelParams.SawtoothBoostStep = boltalkaConfig.GetSawtoothBoostStep();
    generativeBoltalkaParams.ModelParams.SawtoothBoostMax = boltalkaConfig.GetSawtoothBoostMax();
    generativeBoltalkaParams.ModelParams.SawtoothConsumePrefix = boltalkaConfig.GetSawtoothConsumePrefix();

    auto fileForBannedTokensToGenerate = boltalkaConfig.GetFileForBannedTokensToGenerate();
    if (fileForBannedTokensToGenerate != "") {
        generativeBoltalkaParams.ModelParams.FileForBannedTokensToGenerate = folder / fileForBannedTokensToGenerate;
    }
    auto fileForBannedTokenIdsToGenerate = boltalkaConfig.GetFileForBannedTokenIdsToGenerate();
    if (fileForBannedTokenIdsToGenerate != "") {
        generativeBoltalkaParams.ModelParams.FileForBannedTokenIdsToGenerate = folder / fileForBannedTokenIdsToGenerate;
    }

    Y_ENSURE(
        boltalkaConfig.HasServiceEndpointSuffixGenerative() || boltalkaConfig.HasServiceEndpointSuffixScoring() || boltalkaConfig.HasServiceEndpointSuffixEmbedding() || boltalkaConfig.HasServiceEndpointSuffixPHead(),
        "You must specify one of generative, scoring or embedding handlers in the config."
    );

    generativeBoltalkaParams.ModelParams.ShouldInitializeTranslator = boltalkaConfig.HasServiceEndpointSuffixGenerative();
    generativeBoltalkaParams.ModelParams.ShouldInitializeScorer = boltalkaConfig.HasServiceEndpointSuffixScoring();
    generativeBoltalkaParams.ModelParams.ShouldInitializeEmbedder = boltalkaConfig.HasServiceEndpointSuffixEmbedding();
    generativeBoltalkaParams.ModelParams.ShouldInitializePHead = boltalkaConfig.HasServiceEndpointSuffixPHead();

    if (boltalkaConfig.HasRequestPreFilter()) {
        auto p = boltalkaConfig.GetRequestPreFilter();
        generativeBoltalkaParams.RequestPreFilterParams = IGenerativeFilter::TParams{
            .FilterEmpty = p.GetFilterEmptyHypotheses(),
            .FilterDuplicateWords = p.GetFilterDuplicateWords(),
            .FilterBadWords = p.GetFilterBadWords(),
            .BadWordsAreRegexps = p.GetFilterBadWordsAreRegexps(),
            .BadWordsDictPath = folder / p.GetFileName(),
            .FilterDuplicateNGrams = p.GetFilterDuplicateNGrams(),
            .FilterByUniqueWordsRatio = p.GetFilterByUniqueWordsRatio()
        };

        Cerr << "Use pre filter: " << generativeBoltalkaParams.RequestPreFilterParams->BadWordsDictPath << Endl;
    }

    NDict::NMT::NYNMT::EHeadMode headMode;
    switch (boltalkaConfig.GetHeadMode()) {
        case NGenerativeBoltalka::TConfig::TGenerativeBoltalka::Scoring:
            headMode = NDict::NMT::NYNMT::EHeadMode::Scoring;
            break;
        case NGenerativeBoltalka::TConfig::TGenerativeBoltalka::Linear:
            headMode = NDict::NMT::NYNMT::EHeadMode::Linear;
            break;
        case NGenerativeBoltalka::TConfig::TGenerativeBoltalka::OutTokensRepresentations:
            headMode = NDict::NMT::NYNMT::EHeadMode::OutTokensRepresentations;
            break;
        case NGenerativeBoltalka::TConfig::TGenerativeBoltalka::LastLayerTokensRepresentation:
            headMode = NDict::NMT::NYNMT::EHeadMode::LastLayerTokensRepresentation;
            break;
    }
    generativeBoltalkaParams.ModelParams.HeadMode = headMode;
    generativeBoltalkaParams.ModelParams.NormalizeOutTokensRepresentations = boltalkaConfig.GetNormalizeOutTokensRepresentations();
    if (boltalkaConfig.HasMaxLinearHeadOutputDim()) {
        generativeBoltalkaParams.ModelParams.MaxLinearHeadOutputDim = boltalkaConfig.GetMaxLinearHeadOutputDim();
    }

    if (boltalkaConfig.HasHeadWeightsName()) {
        generativeBoltalkaParams.ModelParams.HeadWeightsName = boltalkaConfig.GetHeadWeightsName();
    }

    generativeBoltalkaParams.PrefixOnlyPtune = boltalkaConfig.GetPrefixOnlyPtune();

    generativeBoltalkaParams.EmbeddingsAddMask = boltalkaConfig.GetEmbeddingsAddMask();
    generativeBoltalkaParams.EmbeddingsAddSep = boltalkaConfig.GetEmbeddingsAddSep();
    generativeBoltalkaParams.EmbeddingsDoReverseContext = boltalkaConfig.GetEmbeddingsDoReverseContext();

    return generativeBoltalkaParams;
}
