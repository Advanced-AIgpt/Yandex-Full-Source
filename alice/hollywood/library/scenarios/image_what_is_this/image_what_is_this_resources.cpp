#include <alice/hollywood/library/scenarios/image_what_is_this/image_what_is_this_resources.h>

#include <util/stream/file.h>

using namespace NAlice::NHollywood::NImage;


void TImageWhatIsThisResources::LoadFromPath(const TFsPath& dirPath) {
    const TFsPath& swearPath = dirPath / "swear.txt";
    const TFsPath& badPath = dirPath / "bad.txt";
    LoadSwearWords(swearPath, badPath);

    const TFsPath& entitySearchJokesPath = dirPath / "entity_search_jokes.json";
    LoadEntitySearchJokes(entitySearchJokesPath);
}

const THashSet<TString>& TImageWhatIsThisResources::GetSwearWords() const {
    return SwearWords;
}

const NSc::TValue& TImageWhatIsThisResources::GetEntitySearchJokes() const {
    return EntitySearchJokes;
}

void TImageWhatIsThisResources::LoadSwearWords(const TFsPath& swearPath, const TFsPath& badPath) {
    TFileInput swearInput(swearPath);
    for (TString line; swearInput.ReadLine(line); ) {
        SwearWords.insert(line);
    }

    TFileInput badInput(badPath);
    for (TString line; badInput.ReadLine(line); ) {
        SwearWords.insert(line);
    }
}

void TImageWhatIsThisResources::LoadEntitySearchJokes(const TFsPath& entitySearchJokesPath) {
    TFileInput entitySearchJokesInput(entitySearchJokesPath);
    EntitySearchJokes = NSc::TValue::FromJson(entitySearchJokesInput.ReadAll());
}
