#include <alice/rtlog/evloganalize/split_by_reqid/library/splitter.h>

#include <library/cpp/getopt/last_getopt.h>
#include <util/folder/path.h>

using namespace NAlice::NSplitter;

int main(int argc, char** argv) {
    TFsPath savePath;
    TVector<TFsPath> logPaths;

    NLastGetopt::TOpts opts;
    opts.AddLongOption("save").Help("Directory to save logs").Required().StoreResult(&savePath);
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    auto args = res.GetFreeArgs();
    Copy(args.begin(), args.end(), std::back_inserter(logPaths));

    TSplitter(std::move(savePath), std::move(logPaths)).Split();
    return EXIT_SUCCESS;
}
