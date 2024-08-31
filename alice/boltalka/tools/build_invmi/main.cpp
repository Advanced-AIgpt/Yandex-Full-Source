#include <alice/boltalka/libs/invmi/index.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/stream/file.h>
#include <util/string/cast.h>

void PerformSelfCheck(const TString& invMiModelPath,
                      const TString& vectorsPath,
                      size_t dimension) {

    using TCluster = TVector<ui32>;
    using TClusters = TVector<TCluster>;

    TInvMiIndex index(invMiModelPath, dimension);

    TClusters originalClusters;
    for (size_t i = 0; i < index.GetNumClusters(); ++i) {
        auto cluster = index.GetCluster(i);
        originalClusters.emplace_back(cluster.begin(), cluster.end());
        std::sort(originalClusters.back().begin(), originalClusters.back().end());
    }

    TBlob vectors = TBlob::PrechargedFromFile(vectorsPath);
    index.BuildIndex(vectors);
    TClusters newClusters;
    for (size_t i = 0; i < index.GetNumClusters(); ++i) {
        auto cluster = index.GetCluster(i);
        newClusters.emplace_back(cluster.begin(), cluster.end());
    }

    Y_VERIFY(originalClusters == newClusters);
    Cout << "Passed self check!" << Endl;
}

int main(int argc, char** argv) {
    using namespace NLastGetopt;
    TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('i', "invmi-model")
        .Required()
        .Help("Path to output of learnIMI.py\n\n\n");
    opts
        .AddLongOption('v', "vectors")
        .Required()
        .Help("Path to vectors to index by invmi-model\n\n\n");
    opts
        .AddLongOption('d', "dimension") // TODO(alipov): serialize dimension with invmi-model
        .Required()
        .Help("Dimension of vectors\n\n\n");
    opts
        .AddLongOption("self-check")
        .NoArgument()
        .Help("Builds index on vectors and compares it against index in invmi-model\n\n\n");

    TOptsParseResult res(&opts, argc, argv);

    if (res.Has("self-check")) {
        PerformSelfCheck(res.Get('i'), res.Get('v'), res.Get<size_t>('d'));
        return 0;
    }

    return 0;
}
