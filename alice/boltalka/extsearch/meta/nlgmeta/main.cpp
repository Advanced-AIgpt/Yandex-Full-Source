#include <search/daemons/httpsearch/httpsearch.h>
#include <search/factory/middle/factory.h>

int main(int argc, char** argv) {
    return RunPureMain(argc, argv, NMiddleSearchFactory::SearchFactory());
}
