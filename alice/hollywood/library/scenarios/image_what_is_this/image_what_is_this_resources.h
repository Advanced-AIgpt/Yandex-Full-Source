#include <alice/hollywood/library/resources/resources.h>

#include <util/generic/hash_set.h>
#include <library/cpp/scheme/scheme.h>

namespace NAlice::NHollywood::NImage {

class TImageWhatIsThisResources final : public IResourceContainer {
public:
    void LoadFromPath(const TFsPath& dirPath) override;

    const THashSet<TString>& GetSwearWords() const;
    const NSc::TValue& GetEntitySearchJokes() const;

private:
    void LoadSwearWords(const TFsPath& swearPath, const TFsPath& badPath);
    void LoadEntitySearchJokes(const TFsPath& entitySearchJokesPath);

private:
    THashSet<TString> SwearWords;
    NSc::TValue EntitySearchJokes;
};

}
