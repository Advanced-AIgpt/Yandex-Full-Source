package ru.yandex.alice.nlu.libs.fstnormalizer;

public enum Lang {
    UNK("unk"),  // Unknown
    RUS("rus"),  // Russian
    ENG("eng"),  // English
    POL("pol"),  // Polish
    HUN("hun"),  // Hungarian
    UKR("ukr"),  // Ukrainian
    GER("ger"),  // German
    FRE("fre"),  // French
    TAT("tat"),  // Tatar
    BEL("bel"),  // Belarusian
    KAZ("kaz"),  // Kazakh
    ALB("alb"),  // Albanian
    SPA("spa"),  // Spanish
    ITA("ita"),  // Italian
    ARM("arm"),  // Armenian
    DAN("dan"),  // Danish
    POR("por"),  // Portuguese
    ICE("ice"),  // Icelandic
    SLO("slo"),  // Slovak
    SLV("slv"),  // Slovene
    DUT("dut"),  // Dutch (Netherlandish language)
    BUL("bul"),  // Bulgarian
    CAT("cat"),  // Catalan
    HRV("hrv"),  // Croatian
    CZE("cze"),  // Czech
    GRE("gre"),  // Greek
    HEB("heb"),  // Hebrew
    NOR("nor"),  // Norwegian
    MAC("mac"),  // Macedonian
    SWE("swe"),  // Swedish
    KOR("kor"),  // Korean
    LAT("lat"),  // Latin
    BASIC_RUS  ("basic-rus"), // Simplified version of Russian (used at lemmer only)
    BOS("bos"),  // Bosnian
    MLT("mlt"),  // Maltese
    EMPTY("empty"),  // Indicate that document is empty
    UNK_LAT ("unklat"),  // Any unrecognized latin language
    UNK_CYR ("unkcyr"),  // Any unrecognized cyrillic language
    UNK_ALPHA ("unkalpha"),  // Any unrecognized alphabetic language not fit into previous categories
    FIN("fin"),  // Finnish
    EST("est"),  // Estonian
    LAV("lav"),  // Latvian
    LIT("lit"),  // Lithuanian
    BAK("bak"),  // Bashkir
    TUR("tur"),  // Turkish
    RUM("rum"),  // Romanian (also Moldavian)
    MON("mon"),  // Mongolian
    UZB("uzb"),  // Uzbek
    KIR("kir"),  // Kirghiz
    TGK("tgk"),  // Tajik
    TUK("tuk"),  // Turkmen
    SRP("srp"),  // Serbian
    AZE("aze"),  // Azerbaijani
    BASIC_ENG("basic-eng"), // Simplified version of English (used at lemmer only)
    GEO("geo"),  // Georgian
    ARA("ara"),  // Arabic
    PER("per"),  // Persian
    CHU("chu"),  // Church Slavonic
    CHI("chi"),  // Chinese
    JPN("jpn"),  // Japanese
    IND("ind"),  // Indonesian
    MAY("may"),  // Malay
    THA("tha"),  // Thai
    VIE("vie"),  // Vietnamese
    GLE("gle"),  // Irish (Gaelic)
    TGL("tgl"),  // Tagalog (Filipino)
    HIN("hin"),  // Hindi
    AFR("afr"),  // Afrikaans
    URD("urd"),  // Urdu
    MYA("mya"),  // Burmese
    KHM("khm"),  // Khmer
    LAO("lao"),  // Lao
    TAM("tam"),  // Tamil
    BEN("ben"),  // Bengali
    GUJ("guj"),  // Gujarati
    KAN("kan"),  // Kannada
    PAN("pan"),  // Punjabi
    SIN("sin"),  // Sinhalese
    SWA("swa"),  // Swahili
    BAQ("baq"),  // Basque
    WEL("wel"),  // Welsh
    GLG("glg"),  // Galician
    HAT("hat"),  // Haitian Creole
    MLG("mlg"),  // Malagasy
    CHV("chv"),  // Chuvash
    UDM("udm"),  // Udmurt
    KPV("kpv"),  // Komi-Zyrian
    MHR("mhr"),  // Meadow Mari (Eastern Mari)
    SJN("sjn"),  // Sindarin
    MRJ("mrj"),  // Hill Mari (Western Mari)
    KOI("koi"),  // Komi-Permyak
    LTZ("ltz"),  // Luxembourgish
    GLA("gla"),  // Scottish Gaelic
    CEB("ceb"),  // Cebuano
    PUS("pus"),  // Pashto
    KMR("kmr"),  // Kurmanji
    AMH("amh"),  // Amharic
    ZUL("zul"),  // Zulu
    IBO("ibo"),  // Igbo
    YOR("yor"),  // Yoruba
    COS("cos"),  // Corsican
    XHO("xho"),  // Xhosa
    JAV("jav"),  // Javanese
    NEP("nep"),  // Nepali
    SND("snd"),  // Sindhi
    SOM("som"),  // Somali
    EPO("epo"),  // Esperanto
    TEL("tel"),  // Telugu
    MAR("mar"),  // Marathi
    HAU("hau"),  // Hausa
    YID("yid"),  // Yiddish
    MAL("mal"),  // Malayalam
    MAO("mao"),  // Maori
    SUN("sun"),  // Sundanese
    PAP("pap"),  // Papiamento
    UZB_CYR("uzbcyr"),  // Cyrillic Uzbek
    TRANSCR_IPA ("transcr-ipa"), // International Phonetic Alphabet Transcription
    EMJ("emj"),  // Emoji
    UYG("uig"),  // Uyghur
    BRE("bre"),  // Breton
    SAH("sah"),  // Yakut
    KAZ_LAT("kazlat");  // Latin Kazakh

    private final String isoCode;
    
    Lang(String isoCode) {
        this.isoCode = isoCode;
    }

    public String getCode() {
        return isoCode;
    }
}