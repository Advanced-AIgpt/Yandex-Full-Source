#include "scorer_config.h"

#include <alice/boltalka/generative/service/server/config/config.pb.h>
#include <alice/library/proto/proto.h>
#include <library/cpp/json/json_reader.h>


namespace {

    class TAbsPathGetter : NBg::TDiskFileSystem {
    public:
        explicit TAbsPathGetter(const TFileSystem* fs)
            : TDiskFileSystem(*GetDiskFileSystem(fs))
        {}

        TFsPath GetAbsPath(const TFsPath& relPath) const {
            return DeducePath(relPath);
        }

    private:
        static const TDiskFileSystem* GetDiskFileSystem(const TFileSystem* fs) {
            const NBg::TDiskFileSystem* const diskFileSystem = NBg::TryGetDiskFileSystem(fs);
            Y_ENSURE(diskFileSystem, "This wrapper does not work with non-disk-backed file systems.");
            return diskFileSystem;
        }
    };

}


NGenerativeBoltalka::TGenerativeBoltalka::TParams LoadScoringModelConfig(const NBg::TFileSystem& fs,
                                                                         const TStringBuf& zeliboba_model_config_file) {
    const TAbsPathGetter absPathGetter(&fs);

    const auto modelConfigPath = absPathGetter.GetAbsPath(zeliboba_model_config_file);
    Y_ENSURE(modelConfigPath.Exists(), modelConfigPath << " does not exists");

    auto boltalkaConfig = NAlice::ParseProtoText<NGenerativeBoltalka::TConfig::TGenerativeBoltalka>(fs.LoadInputStream(zeliboba_model_config_file)->ReadAll());

    const auto relativeModelDirPath = boltalkaConfig.GetFolder();
    const auto absoluteModelDirPath = absPathGetter.GetAbsPath(relativeModelDirPath);
    Y_ENSURE(absoluteModelDirPath.Exists(), absoluteModelDirPath << " does not exists");
    boltalkaConfig.SetFolder(absoluteModelDirPath.c_str());

    NGenerativeBoltalka::TGenerativeBoltalka::TParams generativeBoltalkaParams;

    generativeBoltalkaParams.ModelParams.ShouldInitializeTranslator = false;
    generativeBoltalkaParams.ModelParams.ShouldInitializeScorer = true;

    if (fs.Exists("gpu.json")) {
        NJson::TJsonValue config = NJson::ReadJsonTree(fs.LoadInputStream("gpu.json").Get(), true);
        generativeBoltalkaParams.ModelParams.ShouldUseGpus = true;
        auto gpuDeviceId = config["device"].GetIntegerSafe();
        generativeBoltalkaParams.ModelParams.GpuIds.push_back(gpuDeviceId);
        Cerr << "[ZELIBOBA] gpu.json is found" << Endl;
    } else {
        generativeBoltalkaParams.ModelParams.ShouldUseGpus = boltalkaConfig.HasGpuBackend();
        if (generativeBoltalkaParams.ModelParams.ShouldUseGpus) {
            const auto &gpuIds = boltalkaConfig.GetGpuBackend().GetGpuIds();
            generativeBoltalkaParams.ModelParams.GpuIds.insert(generativeBoltalkaParams.ModelParams.GpuIds.begin(),
                                                            gpuIds.begin(), gpuIds.end());
        } else {
            generativeBoltalkaParams.ModelParams.NumCpuBackends = boltalkaConfig.GetCpuBackend().GetNumBackends();
        }
        Cerr << "[ZELIBOBA] gpu.json is not found" << Endl;
    }

    generativeBoltalkaParams.ModelParams.NumThreadsPerSession = boltalkaConfig.GetNumThreadsPerSession();
    generativeBoltalkaParams.ModelParams.BatchSize = boltalkaConfig.GetBatchSize();

    generativeBoltalkaParams.ModelParams.MaxInpLen = boltalkaConfig.GetMaxInputLen();

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

    if (boltalkaConfig.HasHeadWeightsName()) {
        generativeBoltalkaParams.ModelParams.HeadWeightsName = boltalkaConfig.GetHeadWeightsName();
    }
    generativeBoltalkaParams.ModelParams.NormalizeOutTokensRepresentations = boltalkaConfig.GetNormalizeOutTokensRepresentations();

    generativeBoltalkaParams.TokenizerParams.BpeVocPath = absoluteModelDirPath / "bpe.voc";
    generativeBoltalkaParams.TokenizerParams.TokenToIdVocPath = absoluteModelDirPath / "token_to_id.voc";
    generativeBoltalkaParams.TokenizerParams.TokenizerType = boltalkaConfig.GetTokenizerType();

    generativeBoltalkaParams.TokenizerParams.MaxLen = boltalkaConfig.GetMaxInputLen();
    generativeBoltalkaParams.TokenizerParams.TruncatePolicy = boltalkaConfig.GetTruncatePolicy();
    auto modelPath = absoluteModelDirPath / "model.npz";
    generativeBoltalkaParams.ModelParams.CheckpointPath = modelPath;
    auto ptunePath = absoluteModelDirPath / "ptune.npz";
    if (ptunePath.Exists()) {
        Cerr << "Loading p-tune embeddings from " << ptunePath << Endl;
        generativeBoltalkaParams.PtunePath = ptunePath;
    } else {
        Cerr << "Skipping p-tune embeddings cause file doesn't exist: " << ptunePath << Endl;
    }
    generativeBoltalkaParams.ModelParams.MaxPtuneLen = boltalkaConfig.GetMaxPtuneLen();
    generativeBoltalkaParams.ModelParams.UsePrefixedInference = boltalkaConfig.GetUsePrefixedInference();
    generativeBoltalkaParams.FilterParams.BadWordsDictPath = absoluteModelDirPath / "bad_dict.txt";

    return generativeBoltalkaParams;
}
