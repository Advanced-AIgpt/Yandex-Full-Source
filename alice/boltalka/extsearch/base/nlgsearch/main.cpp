#include <alice/boltalka/extsearch/base/search/search.h>
#include <search/daemons/extbasesearch/extbasesearch.h>
#include <kernel/externalrelev/relev.h>
#include <util/string/util.h>

namespace NNlg {

class TNlgSearchFactory: public IExternalSearchFactory {
public:
    IExternalSearch* CreateSearcher(const char* className, const TSearchConfig& /*config*/) override {
        if (::stricmp(className, "nlg") == 0) {
            return CreateNlgSearch();
        }
        return nullptr;
    }
};

}

int main(int argc, char** argv) {
    NNlg::TNlgSearchFactory factory;
    return StartServer(argc, argv, factory);
}

