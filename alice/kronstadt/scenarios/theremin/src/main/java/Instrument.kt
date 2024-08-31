package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

internal enum class Instrument(
    override val index: Int,
    val enumName: String,   //дай звук <suggest>
    val suggest: String
) : GenericInstument {
    PIANO(1, "piano", "пианино"),
    DIGITAL_PIANO(2, "digital_piano", "цифрового пианино"),
    NYLON_GUITAR(3, "nylon_guitar", "гитары на нейлоне"),
    GUITAR_CHORDS(4, "guitar_chords", "гитарных аккордов"),
    DRUMS(5, "drums", "барабанов"),
    JAMBEI(6, "jambei", "дарбука"),
    VIOLIN(7, "violin", "скрипки"),
    PIZZICATO(8, "pizzicato", "кап+ели"),
    HOUSE(9, "house", "хаус"),
    CARTOON(10, "cartoon", "мультфильма"),
    CREEN(11, "creen", "бамбука"),
    BELLS(12, "bells", "колокольчика"),
    BASS_GUITAR(13, "bass_guitar", "бас-гитары"),
    ORGAN(14, "organ", "орг+ана"),
    PIANO_CHORDS(15, "piano_chords", "аккордов на пианино"),
    HOCKEY(16, "hockey", "спортивного орг+ана"),
    VOICE(17, "voice", "вокала"),
    BLADES(18, "blades", "мечей"),
    CYBERPUNK(19, "cyberpunk", "киберпанка"),
    BIRDY(20, "birdy", "свистка"),
    HARMONIC(21, "harmonic", "губной гармошки"),
    DENDI(22, "dendi", "приставки"),
    BASS(23, "bass", "баса"),
    GLITCH(24, "glitch", "помех"),
    QBITE(25, "qbite", "электроорг+ана"),
    GUITAR(26, "guitar", "гитары"),
    POT(27, "pot", "кастрюли"),
    MARS(28, "mars", "марса"),
    SPACE(29, "space", "космоса"),
    STREAM(30, "stream", "космического пианино"),
    TOY(31, "toy", "космического сигнала"),
    VIBRAPHONE(32, "vibraphone", "ксилофона"),
    FLUTE(33, "flute", "флейты"),
    AFRICAN_PERCUSSION(34, "african_percussion", "глухого барабана"),
    DUBSTEP(35, "dubstep", "тёрки"),
    RUSSIAN_ROCK(36, "russian_rock", "русского рока"),
    ACOUSTIC_GUITAR(37, "acoustic_guitar", "акустической гитары"),
    CLUB(38, "club", "клубного бита"),
    CAMPANELLA(39, "campanella", "перезвона"),
    CAT(40, "cat", "кота"),
    KOTO(41, "koto", "барабанов кото"),
    TOY_SYNTHS(42, "toy_synths", "пищалки"),
    MALE_VOICE(43, "male_voice", "мужского голоса"),
    PERCUSSION(44, "percussion", "перкуссии"),
    DOG(45, "dog", "собаки"),
    SCI_FI(46, "sci_fi", "лазера"),
    TRAP(47, "trap", "стыковки"),
    R2D2(48, "r2d2", "дроида"),
    CHIME(49, "chime", "колокола"),
    HARP(50, "harp", "арфы"),
    ELECTRIC_GUITAR(51, "electric_guitar", "электрогитары"),
    SOLO(52, "solo", "соло"),
    MK(53, "mk", "фаталити"),
    BAYAN(54, "bayan", "баяна"),
    ARCADE(55, "arcade", "аркады"),
    TSOY(56, "tsoy", "белый снег"),
    KISH(57, "kish", "колдуна"),
    GLOCKENSPIEL(58, "glockenspiel", "клавесина");

    override val id: String
        get() = enumName

    override val developerType: DeveloperType
        get() = DeveloperType.Yandex

    override val public: Boolean
        get() = true

    override val humanReadableName: String
        get() = suggest

    companion object {
        private val BY_ENUM_NAME: Map<String, Instrument> = values().associateBy { it.enumName }
        private val BY_INDEX: Map<Int, Instrument> = values().associateBy { it.index }

        fun byName(enumName: String): Instrument? = BY_ENUM_NAME[enumName]

        fun byIndex(index: Int): Instrument? = BY_INDEX[index]

        fun size(): Int = BY_INDEX.size
    }
}
