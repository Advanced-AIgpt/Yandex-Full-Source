package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

internal enum class InstrumentGroup(val instruments: List<Instrument>) {
    piano(listOf(Instrument.PIANO, Instrument.DIGITAL_PIANO, Instrument.PIANO_CHORDS)),
    guitar(
        listOf(
            Instrument.GUITAR,
            Instrument.NYLON_GUITAR,
            Instrument.GUITAR_CHORDS,
            Instrument.RUSSIAN_ROCK,
            Instrument.ACOUSTIC_GUITAR,
            Instrument.ELECTRIC_GUITAR,
            Instrument.SOLO,
            Instrument.TSOY,
            Instrument.KISH
        )
    ),
    drums(
        listOf(
            Instrument.DRUMS,
            Instrument.JAMBEI,
            Instrument.CREEN,
            Instrument.AFRICAN_PERCUSSION,
            Instrument.PERCUSSION,
            Instrument.ARCADE
        )
    ),
    violin(
        listOf(
            Instrument.VIOLIN, Instrument.PIZZICATO
        )
    ),
    bass(listOf(Instrument.BASS, Instrument.HOUSE, Instrument.CLUB, Instrument.BASS_GUITAR)),
    bells(
        listOf(
            Instrument.BELLS, Instrument.POT, Instrument.CAMPANELLA, Instrument.CHIME, Instrument.GLOCKENSPIEL
        )
    ),
    organ(
        listOf(
            Instrument.ORGAN, Instrument.HOCKEY, Instrument.QBITE
        )
    ),
    voice(listOf(Instrument.VOICE, Instrument.MALE_VOICE)),
    weapons(
        listOf(
            Instrument.BLADES
        )
    ),
    digital(
        listOf(
            Instrument.GLITCH,
            Instrument.DENDI,
            Instrument.CYBERPUNK,
            Instrument.BIRDY,
            Instrument.DUBSTEP,
            Instrument.TOY_SYNTHS,
            Instrument.SCI_FI,
            Instrument.TRAP,
            Instrument.MK,
            Instrument.R2D2
        )
    ),
    harmonic(
        listOf(
            Instrument.HARMONIC, Instrument.BAYAN
        )
    ),
    space(listOf(Instrument.SPACE, Instrument.TOY, Instrument.MARS, Instrument.STREAM)),
    flute(
        listOf(
            Instrument.FLUTE
        )
    ),
    animal(listOf(Instrument.CAT, Instrument.DOG)),
    strings(
        listOf(
            Instrument.KOTO, Instrument.HARP
        )
    );

    fun getCode(): String {
        return name
    }

    companion object {
        private val BY_NAMES = values().associateBy { it.name }
        fun byName(groupName: String): InstrumentGroup? = BY_NAMES[groupName]
    }
}
