#pragma once

#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NGranet {

namespace NEntityTypes {
    inline const TString NONSENSE               = "nonsense";
    inline const TString STRING                 = "string";

    inline const TString SYS_NUM                = "sys.num";
    inline const TString SYS_FLOAT              = "sys.float";

    inline const TString SYS_TIME               = "sys.time";
    inline const TString SYS_DATE               = "sys.date";
    inline const TString SYS_DATETIME           = "sys.datetime";
    inline const TString SYS_DATETIME_RANGE     = "sys.datetime_range";
    inline const TString SYS_UNITS_TIME         = "sys.units_time";
    inline const TString SYS_WEEKDAYS           = "sys.weekdays";

    inline const TString SYS_FIO                = "sys.fio";
    inline const TString SYS_FIO_NAME           = "sys.fio.name";
    inline const TString SYS_FIO_PATRONYM       = "sys.fio.patronym";
    inline const TString SYS_FIO_SURNAME        = "sys.fio.surname";

    inline const TString SYS_CALC               = "sys.calc";
    inline const TString SYS_CURRENCY           = "sys.currency";
    inline const TString SYS_GEO                = "sys.geo";
    inline const TString SYS_POI_CATEGORY_RU    = "sys.poi_category_ru";

    inline const TString SYS_ALBUM              = "sys.album";
    inline const TString SYS_ARTIST             = "sys.artist";
    inline const TString SYS_FILMS_100_750      = "sys.films_100_750";
    inline const TString SYS_FILMS_50_FILTERED  = "sys.films_50_filtered";
    inline const TString SYS_SITE               = "sys.site";
    inline const TString SYS_SOFT               = "sys.soft";
    inline const TString SYS_SWEAR              = "sys.swear";
    inline const TString SYS_TRACK              = "sys.track";

    inline const TString GEO_ADDR_ADDRESS       = "GeoAddr.Address";

    inline const TString PA_SKILLS_NONSENSE     = "YANDEX.NONSENSE";
    inline const TString PA_SKILLS_NUMBER       = "YANDEX.NUMBER";
    inline const TString PA_SKILLS_GEO          = "YANDEX.GEO";
    inline const TString PA_SKILLS_DATETIME     = "YANDEX.DATETIME";
    inline const TString PA_SKILLS_FIO          = "YANDEX.FIO";

    inline const TString SYN_THESAURUS_LEMMA              = "syn.thesaurus_lemma";
    inline const TString SYN_THESAURUS_DIMIN_NAME_LEMMA   = "syn.thesaurus_dimin_name_lemma";
    inline const TString SYN_THESAURUS_SYNSET_LEMMA       = "syn.thesaurus_synset_lemma";
    inline const TString SYN_THESAURUS_TRANSLIT_LEMMA     = "syn.thesaurus_translit_lemma";
    inline const TString SYN_THESAURUS_TRANSLIT_EN_LEMMA  = "syn.thesaurus_translit_en_lemma";
    inline const TString SYN_THESAURUS_TRANSLIT_RU_LEMMA  = "syn.thesaurus_translit_ru_lemma";
    inline const TString SYN_THESAURUS_INFLECT            = "syn.thesaurus_inflect";
    inline const TString SYN_TRANSLIT_EN                  = "syn.translit_en";
    inline const TString SYN_TRANSLIT_EN_LEMMA            = "syn.translit_en_lemma";
    inline const TString SYN_TRANSLIT_RU                  = "syn.translit_ru";
    inline const TString SYN_TRANSLIT_RU_LEMMA            = "syn.translit_ru_lemma";
}

namespace NEntityTypePrefixes {
    inline const TString SYN                    = "syn.";
    inline const TString SYS                    = "sys.";
    inline const TString FST                    = "fst."; // deprecated
    inline const TString CUSTOM                 = "custom.";
    inline const TString USER                   = "user.";
    inline const TString DEPRECATED_IOT         = "device.iot.";
    inline const TString IOT                    = "user.iot.";
    inline const TString ALICE_TYPE_PARSER      = "typeparser.";
    inline const TString GEO_ADDR               = "GeoAddr.";
    inline const TString PA_SKILLS              = "YANDEX.";
    inline const TString ENTITY_SEARCH          = "entity_search.";
    inline const TString SCENARIO               = "scenario.";
    inline const TString DEVICE                 = "device.";
}

// Default LogProbabilities for some types of entities.
namespace NEntityLogProbs {
    const double NONSENSE_WORD = -5; // for each word
    const double SYS = -3;
    const double CUSTOM = -4;
    const double USER = -2;
    const double IOT = -4;
    const double GEO_ADDR = -5;
    const double GEO_ADDR_ALTERNATIVE = -6;
    const double PA_SKILLS_DATETIME = -3;
    const double PA_SKILLS_FIO = -5;
    const double ENTITY_SEARCH = -4;
    const double THESAURUS = -1.5; // smaller LEMMA_LOG_PROB (-1)
    const double TRANSLIT = -0.9;  // bigger LEMMA_LOG_PROB (-1)
    const double UNKNOWN = -3;
}

// For deterministic resolving of ambiguity.
// Details in https://st.yandex-team.ru/DIALOG-8372
inline const THashMap<TString, double> AMBIGUOUS_ENTITY_EXTRA_LOG_PROB = {
    {"user.iot.group", -0.01},
    {"user.iot.device", -0.02},
};

// For TEntity::Source
namespace NEntitySources {
    inline const TString GRANET = "granet";
    inline const TString USER_ENTITY_FINDER = "user_entity_finder";
}

// For TEntity::Flags
namespace NEntityFlags {
    inline const TString GALLERY_VISIBLE_ITEM = "gallery_visible_item";
}

inline const THashSet<TString> ALLOWED_ENTITY_GROUPS = {
    "ner",
    "nonsense",
    "fst",
    "sys",
    "custom",
    "user",
    "typeparser",
    "entity_search",
    "scenario",
    "device"
};

// For backward compatibility.
// Prefix 'fst' is deprecated, use 'sys' instead.
inline const THashSet<TString> ALLOWED_FST_TYPES = {
    "fst.date",
    "fst.datetime",
    "fst.fio.name",
    "fst.geo",
    "fst.num",
    "fst.swear",
    "fst.time",
    "fst.weekdays",
};

} // namespace NGranet
