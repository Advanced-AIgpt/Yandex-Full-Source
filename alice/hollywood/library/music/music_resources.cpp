#include "music_resources.h"

#include <alice/library/json/json.h>

#include <util/stream/file.h>

namespace NAlice::NHollywood::NMusic {

TMusicResources::TMusicResources() {
    // these are hand-picked values, we only fall back to automatic generation
    // as a temporary measure when a new value is added
    Novelties_["new"] = {"новые", false};

    NeedSimilar_["need_similar"] = {"похожие", false};

    Moods_["summer"] = {"летнее", false};
    Moods_["sentimental"] = {"сентиментальное", false};
    Moods_["lullaby"] = {"колыбельная", false};
    Moods_["relaxed"] = {"медленное", false};
    Moods_["dark"] = {"мрачное", false};
    Moods_["aggressive"] = {"агрессивное", false};
    Moods_["beautiful"] = {"красивое", false};
    Moods_["newyear"] = {"новогоднее", false};
    Moods_["haunting"] = {"мистическое", false};
    Moods_["cool"] = {"крутое", false};
    Moods_["sad"] = {"грустное", false};
    Moods_["epic"] = {"эпичное", false};
    Moods_["happy"] = {"веселое", false};
    Moods_["spring"] = {"весеннее", false};
    Moods_["winter"] = {"зимнее", false};
    Moods_["dream"] = {"возвышенное", false};
    Moods_["rainy"] = {"дождливое", false};
    Moods_["healing"] = {"исцеляющее", false};
    Moods_["energetic"] = {"бодрое", false};
    Moods_["autumn"] = {"осеннее", false};

    Genres_["rap"] = {"rap", false};
    Genres_["foreignbard"] = {"иностранная авторская песня", false};
    Genres_["folk"] = {"folk", false};
    Genres_["house"] = {"house", false};
    Genres_["tvseries"] = {"сериал", false};
    Genres_["balkan"] = {"балканское", false};
    Genres_["country"] = {"country", false};
    Genres_["rusfolk"] = {"русское народное", false};
    Genres_["caucasian"] = {"кавказское", false};
    Genres_["disco"] = {"disco", false};
    Genres_["sport"] = {"спортивное", false};
    Genres_["experimental"] = {"экспериментальное", false};
    Genres_["rock"] = {"rock", false};
    Genres_["hardrock"] = {"hard rock", false};
    Genres_["tatar"] = {"татарское", false};
    Genres_["trance"] = {"trance", false};
    Genres_["soundtrack"] = {"саундтрек", false};
    Genres_["soviet"] = {"советское", false};
    Genres_["postrock"] = {"post-rock", false};
    Genres_["posthardcore"] = {"post-hardcore", false};
    Genres_["epicmetal"] = {"эпик метал", false};
    Genres_["numetal"] = {"nu metal", false};
    Genres_["pop"] = {"поп", false};
    Genres_["rusbards"] = {"русские бардовские", false};
    Genres_["industrial"] = {"industrial", false};
    Genres_["conjazz"] = {"современный джаз", false};
    Genres_["romances"] = {"романс", false};
    Genres_["forchildren"] = {"детское", false};
    Genres_["folkmetal"] = {"folk metal", false};
    Genres_["argentinetango"] = {"аргентинское танго", false};
    Genres_["ukrrock"] = {"украинский рок", false};
    Genres_["folkrock"] = {"фолк-рок", false};
    Genres_["bollywood"] = {"болливуд", false};
    Genres_["electronics"] = {"электроника", false};
    Genres_["classicmetal"] = {"классический метал", false};
    Genres_["jazz"] = {"jazz", false};
    Genres_["urban"] = {"городское", false};
    Genres_["dub"] = {"dub", false};
    Genres_["local-indie"] = {"русское инди", false};
    Genres_["blues"] = {"blues", false};
    Genres_["rusestrada"] = {"русская эстрада", false};
    Genres_["metal"] = {"metal", false};
    Genres_["newage"] = {"new age", false};
    Genres_["rusrock"] = {"русский рок", false};
    Genres_["bard"] = {"бардовское", false};
    Genres_["rnr"] = {"rock n roll", false};
    Genres_["punk"] = {"punk", false};
    Genres_["lounge"] = {"lounge", false};
    Genres_["newwave"] = {"new wave", false};
    Genres_["reggaeton"] = {"reggaeton", false};
    Genres_["dnb"] = {"drum and bass", false};
    Genres_["reggae"] = {"reggae", false};
    Genres_["films"] = {"кино", false};
    Genres_["foreignrap"] = {"зарубежный рэп", false};
    Genres_["dubstep"] = {"dubstep", false};
    Genres_["tradjazz"] = {"jazz традиционная", false};
    Genres_["hardcore"] = {"hardcore", false};
    Genres_["indie"] = {"indie", false};
    Genres_["shanson"] = {"шансон", false};
    Genres_["classical"] = {"классическая", false};
    Genres_["classicalmasterpieces"] = {"классическая", false};
    Genres_["alternative"] = {"альтернатива", false};
    Genres_["progmetal"] = {"prog metal", false};
    Genres_["relax"] = {"легкая", false};
    Genres_["techno"] = {"techno", false};
    Genres_["celtic"] = {"кельтская", false};
    Genres_["african"] = {"африканская", false};
    Genres_["ruspop"] = {"русский поп", false};
    Genres_["meditation"] = {"медитация", false}; // XXX(a-square): ambient?
    Genres_["estrada"] = {"эстрада", false};
    Genres_["latinfolk"] = {"латиноамериканские", false};
    Genres_["rnb"] = {"R&B", false};
    Genres_["videogame"] = {"видео игры", false};
    Genres_["vocal"] = {"вокальная", false};
    Genres_["eurofolk"] = {"европейское народное", false};
    Genres_["amerfolk"] = {"американское народное", false};
    Genres_["animated"] = {"мультяшное", false};
    Genres_["rusrap"] = {"русский рэп", false};
    Genres_["musical"] = {"мюзикла", false};
    Genres_["funk"] = {"funk", false};
    Genres_["modern"] = {"современная классическая", false};
    Genres_["prog"] = {"prog rock", false};
    Genres_["kpop"] = {"k-pop", false};
    Genres_["soul"] = {"soul", false};
    Genres_["ska"] = {"ska", false};
    Genres_["stonerrock"] = {"стоунер рок", false};
    Genres_["dance"] = {"клубная", false};
    Genres_["jewish"] = {"еврейская", false};
    Genres_["extrememetal"] = {"extreme metal", false};

    Activities_["beloved"] = {"романтичное", false};
    Activities_["sex"] = {"сексуальное", false};
    Activities_["driving"] = {"езды на автомобиле", false};
    Activities_["road-trip"] = {"дорожная", false};
    Activities_["party"] = {"вечеринки", false};
    Activities_["fall-asleep"] = {"засыпания", false};
    Activities_["wake-up"] = {"пробуждения", false};
    Activities_["work-background"] = {"фоновая", false};
    Activities_["workout"] = {"занятия спортом", false};
    Activities_["bicycle"] = {"занятий велосипедом", false};
    Activities_["run"] = {"занятия бегом", false};
    Activities_["go"] = {"го", false};

    Epochs_["zeroes"] = {"нулевых", false};
    Epochs_["fifties"] = {"50-х", false};
    Epochs_["sixties"] = {"60-х", false};
    Epochs_["eighties"] = {"80-х", false};
    Epochs_["the-greatest-hits"] = {"вечное", false};
    Epochs_["nineties"] = {"90-х", false};
    Epochs_["seventies"] = {"70-х", false};

    Languages_["romanian"] = {"по-румынски", false};
    Languages_["bosnian"] = {"боснийская", false};
    Languages_["not-russian"] = {"зарубежная", false};
    Languages_["ukrainian"] = {"по-украински", false};
    Languages_["netherlandish"] = {"голландская", false};
    Languages_["turkish"] = {"по-турецки", false};
    Languages_["portuguese"] = {"по-португальски", false};
    Languages_["french"] = {"по-французски", false};
    Languages_["spanish"] = {"испанская", false};
    Languages_["malay"] = {"малазийская", false};
    Languages_["finnish"] = {"по-фински", false};
    Languages_["swedish"] = {"по-шведски", false};
    Languages_["italian"] = {"итальянские", false};
    Languages_["german"] = {"немецкая", false};
    Languages_["russian"] = {"русская", false};
    Languages_["swahili"] = {"суахили", false};

    Vocals_["female"] = {"женский", false};
    Vocals_["instrumental"] = {"инструментальное", false};
    Vocals_["male"] = {"мужской", false};
}

void TMusicResources::LoadFromPath(const TFsPath& dirPath) {
    // load fallback values in case some were added to JSON files
    auto add = [](TEnumStringValues& enumValues, const TFsPath& path) {
        TFileInput input(path);
        const auto content = input.ReadAll();
        const auto json = JsonFromString(content);

        for (const auto& [key, values] : json.GetMap()) {
            enumValues.insert({key, {values.GetArray().at(0).GetString(), /* Autogenerated= */ true}});
        }
    };

    add(Novelties_, dirPath / "novelty.json");
    add(NeedSimilar_, dirPath / "need_similar.json");
    add(Moods_, dirPath / "mood.json");
    add(Genres_, dirPath / "genre.json");
    add(Activities_, dirPath / "activity.json");
    add(Epochs_, dirPath / "epoch.json");
    add(Languages_, dirPath / "language.json");
    add(Vocals_, dirPath / "vocal.json");

    TFileInput radioStationInput(dirPath / RADIO_STATION_RESOURCE_NAME);
    const TString rawRadioStationData = radioStationInput.ReadAll();
    NJson::TJsonValue radioStationJson = JsonFromString(rawRadioStationData);

    TFileInput radioStationsInput(dirPath / RADIO_STATIONS_RESOURCE_NAME);
    const TString rawRadioStationsData = radioStationsInput.ReadAll();
    NJson::TJsonValue radioStationsJson = JsonFromString(rawRadioStationsData);

    FmRadioResources_ = TFmRadioResources(radioStationJson, radioStationsJson);
}

std::function<TString(const TStringBuf key, const TString& value)>
SlotToTextProvider(const TMusicResources& musicResources) {
    const auto provider = [&musicResources](const TStringBuf key, const TString& value) -> TString
    {
        if (const auto* mapping = musicResources.EnumStringValues(key)) {
            if (const auto* stringValue = mapping->FindPtr(value)) {
                return stringValue->Value;
            }
        }
        return value;
    };

    return provider;
}

} // namespace NAlice::NHollywood::NMusic
