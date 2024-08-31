#include <util/stream/file.h>
#include <util/string/strip.h>

#include <alice/boltalka/bert/lib/main.h>

TVector<TString> ReadSamples(const TString& filename) {
    TIFStream file(filename);
    TVector<TString> result;
    TString str;
    while (file.ReadLine(str))
        result.push_back(StripString(str));
    return result;
}

template <typename TFloatType>
void RunAndReport(const TVector<TVector<int>>& data,
         const TVector<TString>& samples,
         int maxBatchSize,
         int maxInputLength,
         TMaybe<int> numThreads,
         const TString& weightsFilename,
         bool useCpu,
         int deviceIndex) {
    TVector<TFloatType> results = Run<TFloatType>(data, samples, maxBatchSize, maxInputLength, numThreads, weightsFilename, useCpu, deviceIndex);
    for (const auto& el : results) {
        Cout << el << Endl;
    }
}

int main(int argc, char* argv[]) {
    size_t maxBatchSize;
    size_t maxInputLength;
    TString samplesFilename;
    TString weightsFilename;
    TString startTrieFilename;
    TString contTrieFilename;
    TString vocabFilename;
    bool useCPU = false;
    bool useFp16 = false;
    int deviceIndex = 0;
    TMaybe<int> numThreads;

    auto opts = NLastGetopt::TOpts::Default();
    opts.AddLongOption("input")
        .StoreResult(&samplesFilename)
        .Required()
        .Help("file with the input queries");
    opts.AddLongOption("weights")
        .StoreResult(&weightsFilename)
        .Required()
        .Help("path to a file with the model weights");
    opts.AddLongOption("starttrie")
        .StoreResult(&startTrieFilename)
        .Required()
        .Help("path to a file with a start trie for tokenization");
    opts.AddLongOption("conttrie")
        .StoreResult(&contTrieFilename)
        .Required()
        .Help("path to a file with a start trie for tokenization");
    opts.AddLongOption("vocab")
        .StoreResult(&vocabFilename)
        .Required()
        .Help("path to a file with a vocabulary");
    opts.AddLongOption("cpu")
        .NoArgument()
        .StoreValue(&useCPU, true);
    opts.AddLongOption("fp16")
        .NoArgument()
        .Help("load a model with FP16 weights (use dict/mt/make/tools/tfnn/convert_to_fp16 to convert an existing FP32 model)")
        .StoreValue(&useFp16, true);
    opts.AddLongOption("device")
        .Help("when using GPU, selects the index of GPU to use")
        .StoreResult(&deviceIndex);
    opts.AddLongOption("threads")
        .Help("when using CPU, sets the thread count for OpenMP/MKL")
        .Handler1T<int>([&](int value) {
            numThreads = value;
        });
    opts.AddLongOption("batchsize").StoreResult(&maxBatchSize).DefaultValue(32);
    opts.AddLongOption("maxinputlength").StoreResult(&maxInputLength).DefaultValue(127);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    auto samples = ReadSamples(samplesFilename);
    if (samples.size() == 0)
        return 0;

    auto data = PreprocessSamples(
        samples,
        maxInputLength,
        startTrieFilename,
        contTrieFilename,
        vocabFilename);

    if (useFp16) {
        RunAndReport<TFloat16>(data, samples, maxBatchSize, maxInputLength, numThreads, weightsFilename, useCPU, deviceIndex);
    } else {
        RunAndReport<float>(data, samples, maxBatchSize, maxInputLength, numThreads, weightsFilename, useCPU, deviceIndex);
    }

    return 0;
}

