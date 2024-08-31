package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import org.hamcrest.CoreMatchers
import org.hamcrest.MatcherAssert
import org.hamcrest.Matchers
import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Disabled
import org.junit.jupiter.api.Test
import org.junit.jupiter.params.ParameterizedTest
import org.junit.jupiter.params.provider.MethodSource
import org.mockito.ArgumentMatchers
import org.mockito.Mockito
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.beans.factory.annotation.Value
import org.springframework.boot.autoconfigure.jackson.JacksonAutoConfiguration
import org.springframework.boot.test.context.SpringBootTest
import org.springframework.boot.test.mock.mockito.MockBean
import org.springframework.core.env.Environment
import org.springframework.test.context.ContextConfiguration
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.directive.ExternalThereminPlayDirective
import ru.yandex.alice.kronstadt.core.directive.InternalThereminPlayDirective
import ru.yandex.alice.kronstadt.core.input.Input
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame.Companion.create
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameSlot
import ru.yandex.alice.kronstadt.test.ClientInfoTestUtils
import ru.yandex.alice.paskill.dialogovo.scenarios.theremin.PlayScene.Companion.THEREMIN_SOUNDS_S3_PATH
import ru.yandex.alice.paskill.dialogovo.scenarios.theremin.ThereminSkillInfoDB.Sound
import ru.yandex.alice.paskill.dialogovo.scenarios.theremin.ThereminSkillInfoDB.SoundSet
import java.io.IOException
import java.time.Instant
import java.util.Random
import java.util.UUID
import java.util.regex.Pattern
import java.util.stream.Stream
import javax.sql.DataSource

@SpringBootTest
@ContextConfiguration(classes = [ThereminConfiguration::class, JacksonAutoConfiguration::class])
@MockBean(DataSource::class)
internal open class PlaySceneTest : ClientInfoTestUtils {
    private lateinit var thereminService: PlayScene

    @MockBean
    private lateinit var thereminSkillsDao: ThereminSkillsDao
    private val requestContext = RequestContext()

    @Autowired
    private lateinit var thereminPacks: ThereminPacksConfig

    //@Value("\${fillCachesOnStartUp}")
    private var fillCachesOnStartUp: Boolean = false

    @Value("\${thereminConfig.baseS3Url}")
    private lateinit var baseS3Url: String

    @Autowired
    private lateinit var env: Environment

    @BeforeEach
    @Throws(IOException::class)
    fun setUp() {
        requestContext.clear()
        thereminService = PlayScene(thereminSkillsDao, requestContext, thereminPacks, baseS3Url, fillCachesOnStartUp)

        Mockito.`when`(thereminSkillsDao.findThereminPrivateSkillsByUser(ArgumentMatchers.anyString()))
            .thenReturn(listOf())
        thereminService.refreshCache()
    }

    @ParameterizedTest
    @MethodSource("arguments")
    fun runTest(input: TInput) {
        val request = createRequest(input)
        val response = thereminService.processMiniRequest(request, request.input.semanticFrames[0])
        Assertions.assertTrue(response.isRelevant)
        val runResponse = response as RunOnlyResponse<ThereminState>
        val mode = (runResponse.body as ScenarioResponseBody).layout.directives
            .filterIsInstance(ExternalThereminPlayDirective::class.java)
            .map { it.skillId }
            .firstOrNull()
        Assertions.assertEquals(input.expectedMode.toString(), mode)
    }

    @Test
    fun testMielophone() {
        val input = TInput.mielophone()
        val request = createRequest(input)
        val response = thereminService.processMiniRequest(request, request.input.semanticFrames[0])
        Assertions.assertTrue(response.isRelevant)
        val runResponse = response as RunOnlyResponse<ThereminState>
        val mode = (runResponse.body as ScenarioResponseBody).layout.directives
            .filterIsInstance(InternalThereminPlayDirective::class.java)
            .map(InternalThereminPlayDirective::mode)
            .firstOrNull()
        Assertions.assertEquals(0, mode)
    }

    @Test
    fun testMielophoneNew() {
        val input = TInput.mielophone()
        val request = createRequest(input)
        val response = thereminService.processMiniRequest(request, request.input.semanticFrames[0])
        Assertions.assertTrue(response.isRelevant)
        val runResponse = response as RunOnlyResponse<ThereminState>
        val mode = (runResponse.body as ScenarioResponseBody).layout.directives
            .filterIsInstance(InternalThereminPlayDirective::class.java)
            .map(InternalThereminPlayDirective::mode)
            .firstOrNull()
        Assertions.assertEquals(0, mode)
    }

    @Test
    fun testIndexOutOfRange() {
        val request = createRequest(TInput.builder(-1, "семьсот").beatNumberSlot(700).build())
        val response = thereminService.processMiniRequest(request, request.input.semanticFrames[0])
        Assertions.assertTrue(response.isRelevant)
        val runResponse = response as RunOnlyResponse<ThereminState>
        MatcherAssert.assertThat(
            thereminService.OUT_OF_RANGE_INDEX_RESPONSES,
            CoreMatchers.hasItem((runResponse.body as ScenarioResponseBody).layout.getOutputSpeechO().get())
        )
        MatcherAssert.assertThat((runResponse.body as ScenarioResponseBody).layout.directives, Matchers.empty())
    }

    @Test
    fun testZeroIndexOutOfRange() {
        val request = createRequest(TInput.builder(-1, "ноль").beatNumberSlot(0).build())
        val response = thereminService.processMiniRequest(request, request.input.semanticFrames[0])
        Assertions.assertTrue(response.isRelevant)
        val runResponse = response as RunOnlyResponse<ThereminState>
        MatcherAssert.assertThat(
            thereminService.OUT_OF_RANGE_INDEX_RESPONSES,
            CoreMatchers.hasItem((runResponse.body as ScenarioResponseBody).layout.getOutputSpeechO().get())
        )
        MatcherAssert.assertThat((runResponse.body as ScenarioResponseBody).layout.directives, Matchers.empty())
    }

    @Test
    fun testSubIndexOutOfRange() {
        val request = createRequest(
            TInput.builder(-1, "рояль семьсот").beatGroupSlot("piano")
                .beanGroupIndexSlot(700).build()
        )
        val response = thereminService.processMiniRequest(request, request.input.semanticFrames[0])
        Assertions.assertTrue(response.isRelevant)
        val runResponse = response as RunOnlyResponse<ThereminState>
        MatcherAssert.assertThat(
            thereminService.NOT_INSTRUMENT_RESPONSES,
            CoreMatchers.hasItem((runResponse.body as ScenarioResponseBody).layout.getOutputSpeechO().get())
        )
        MatcherAssert.assertThat((runResponse.body as ScenarioResponseBody).layout.directives, Matchers.empty())
    }

    @Test
    fun testUnknownInstrumentName() {
        val request = createRequest(
            TInput.builder(-1, "рояль неизвестный")
                .beatTextSlot("рояль " + "неизвестный").build()
        )
        val response = thereminService.processMiniRequest(request, request.input.semanticFrames[0])
        Assertions.assertTrue(response.isRelevant)
        val runResponse = response as RunOnlyResponse<ThereminState>
        MatcherAssert.assertThat(
            thereminService.NOT_INSTRUMENT_RESPONSES,
            CoreMatchers.hasItem((runResponse.body as ScenarioResponseBody).layout.getOutputSpeechO().get())
        )
        MatcherAssert.assertThat((runResponse.body as ScenarioResponseBody).layout.directives, Matchers.empty())
    }

    @Test
    fun testUserInstrumentName() {
        // given
        requestContext.currentUserId = "123"
        val skill = ThereminSkillInfoDB(
            id = UUID.randomUUID(),
            onAir = true,
            userId = "123",
            name = "ботинок",
            backendSettings =
            ThereminSkillInfoDB.BackendSettings(
                SoundSet(
                    listOf(
                        Sound("1", "asd/ABC.mp3"),
                        Sound("1", "asd/CDE.mp3")
                    ),
                    ThereminSkillInfoDB.Settings(
                        noOverlaySamples = true,
                        repeatSoundInside = true,
                        stopOnCeil = true
                    )
                )
            ),
            developerType = "External",
            hideInStore = true
        )
        Mockito.`when`(thereminSkillsDao.findThereminPrivateSkillsByUser("123"))
            .thenReturn(listOf(skill))

        // when
        val request = createRequest(TInput.builder(-1, "ботинок").beatTextSlot("ботинок").build())
        val response = thereminService.processMiniRequest(request, request.input.semanticFrames[0])

        // then
        Assertions.assertTrue(response.isRelevant)
        val runResponse = response as RunOnlyResponse<ThereminState>
        val directive: ExternalThereminPlayDirective? = (runResponse.body as ScenarioResponseBody).layout.directives
            .filterIsInstance(ExternalThereminPlayDirective::class.java)
            .firstOrNull()
        val expected = ExternalThereminPlayDirective(
            skillId = skill.id.toString(),
            noOverlaySamples = true,
            repeatSoundInside = true,
            stopOnCeil = true,
            sampleUrls = listOf(
                "https://s3.mdst.yandex.net/dialogs/asd/CDE.mp3",
                "https://s3.mdst.yandex.net/dialogs/asd/ABC.mp3"
            )
        )

        Assertions.assertEquals(expected, directive)
    }

    @Test
    fun testUserInstrumentNameNew() {
        // given
        requestContext.currentUserId = "123"
        Mockito.`when`(thereminSkillsDao.findThereminPrivateSkillsByUser("123"))
            .thenReturn(listOf())

        // when
        val request = createRequest(TInput.builder(55, "аркады").beatEnumSlot("arcade").build())
        val response = thereminService.processMiniRequest(request, request.input.semanticFrames[0])

        // then
        Assertions.assertTrue(response.isRelevant)
        val runResponse = response as RunOnlyResponse<ThereminState>
        val directive: ExternalThereminPlayDirective? = (runResponse.body as ScenarioResponseBody).layout.directives
            .filterIsInstance(ExternalThereminPlayDirective::class.java)
            .firstOrNull()
        val expected = ExternalThereminPlayDirective(
            skillId = "55",
            noOverlaySamples = false,
            repeatSoundInside = false,
            stopOnCeil = false,
            sampleUrls = listOf(
                "https://s3.mdst.yandex.net/dialogs/${THEREMIN_SOUNDS_S3_PATH}55_234239733a6940f48db8287e1c899602/1.mp3",
                "https://s3.mdst.yandex.net/dialogs/${THEREMIN_SOUNDS_S3_PATH}55_234239733a6940f48db8287e1c899602/2.mp3",
                "https://s3.mdst.yandex.net/dialogs/${THEREMIN_SOUNDS_S3_PATH}55_234239733a6940f48db8287e1c899602/3.mp3",
                "https://s3.mdst.yandex.net/dialogs/${THEREMIN_SOUNDS_S3_PATH}55_234239733a6940f48db8287e1c899602/4.mp3"
            )
        )
        Assertions.assertEquals(expected, directive)
    }

    @Test
    fun testPublicInstrumentByName() {
        // given
        requestContext.currentUserId = "123"
        val skill = ThereminSkillInfoDB(
            id = UUID.randomUUID(),
            onAir = true,
            userId = "456",
            name = "ботинок",
            hideInStore = false, // public skill created by another user
            backendSettings =
            ThereminSkillInfoDB.BackendSettings(
                SoundSet(
                    listOf(
                        Sound("1", "asd/ABC.mp3"),
                        Sound("1", "asd/CDE.mp3")
                    ),
                    ThereminSkillInfoDB.Settings(true, true, true)
                )
            ),
            developerType = "External",
        )
        Mockito.`when`(thereminSkillsDao.findThereminAllPublicSkills())
            .thenReturn(listOf(skill))
        thereminService.refreshCache()

        // when
        val request = createRequest(TInput.builder(-1, "ботинок").beatTextSlot("ботинок").build())
        val response = thereminService.processMiniRequest(request, request.input.semanticFrames[0])

        // then
        Assertions.assertTrue(response.isRelevant)
        val runResponse = response as RunOnlyResponse<ThereminState>
        val directive: ExternalThereminPlayDirective? = (runResponse.body as ScenarioResponseBody).layout.directives
            .filterIsInstance(ExternalThereminPlayDirective::class.java)
            .firstOrNull()
        val expected = ExternalThereminPlayDirective(
            skillId = skill.id.toString(),
            noOverlaySamples = true,
            repeatSoundInside = true,
            stopOnCeil = true,
            sampleUrls = listOf(
                "https://s3.mdst.yandex.net/dialogs/asd/CDE.mp3",
                "https://s3.mdst.yandex.net/dialogs/asd/ABC.mp3"
            )
        )
        Assertions.assertEquals(expected, directive)
    }

    @Test
    fun testPublicInstrumentByNameWithNormalization() {
        // given
        requestContext.currentUserId = "123"
        val skill = ThereminSkillInfoDB(
            id = UUID.randomUUID(),
            onAir = true,
            userId = "456",
            name = "из 2008",
            inflectedActivationPhrases = listOf("из 2008"),
            hideInStore = false, // public skill created by another user
            backendSettings =
            ThereminSkillInfoDB.BackendSettings(
                SoundSet(
                    listOf(
                        Sound("1", "asd/ABC.mp3"),
                        Sound("1", "asd/CDE.mp3")
                    ),
                    ThereminSkillInfoDB.Settings(true, true, true)
                )
            ),
            developerType = "External"
        )
        Mockito.`when`(thereminSkillsDao.findThereminAllPublicSkills())
            .thenReturn(listOf(skill))
        thereminService.refreshCache()

        // when
        val request = createRequest(
            TInput.builder(-1, "из две тысячи восьмого")
                .beatTextSlot("из две тысячи восьмого").build()
        )
        val response = thereminService.processMiniRequest(request, request.input.semanticFrames[0])

        // then
        Assertions.assertTrue(response.isRelevant)
        val runResponse = response as RunOnlyResponse<ThereminState>
        val directive: ExternalThereminPlayDirective? = (runResponse.body as ScenarioResponseBody).layout.directives
            .filterIsInstance(ExternalThereminPlayDirective::class.java)
            .firstOrNull()

        val expected = ExternalThereminPlayDirective(
            skillId = skill.id.toString(),
            noOverlaySamples = true,
            repeatSoundInside = true,
            stopOnCeil = true,
            sampleUrls = listOf(
                "https://s3.mdst.yandex.net/dialogs/asd/CDE.mp3",
                "https://s3.mdst.yandex.net/dialogs/asd/ABC.mp3"
            )
        )

        Assertions.assertEquals(expected, directive)
    }

    @Test
    fun testOnboarding() {
        val request = createRequest(TInput.builder(-1, "").build())
        val response = thereminService.processMiniRequest(request, request.input.semanticFrames[0])
        Assertions.assertTrue(response.isRelevant)
        val runResponse = response as RunOnlyResponse<ThereminState>
        val r = Random(request.randomSeed)
        val expectedText = String.format(
            thereminService.ONBOARDING_GUIDE,
            Instrument.values()[r.nextInt(Instrument.size())].suggest,
            r.nextInt(Instrument.size()) + 1
        )
        Assertions.assertEquals(
            expectedText,
            (runResponse.body as ScenarioResponseBody).layout.getOutputSpeechO().get()
        )
        MatcherAssert.assertThat((runResponse.body as ScenarioResponseBody).layout.directives, Matchers.empty())
    }

    @Test
    @Disabled("disabled until mini release")
    fun testThereminOnStation() {
        val request = createRequest(TInput.builder(-1, "").build())
            .copy(clientInfo = station(ClientInfoTestUtils.DEFAULT_UUID))
        val response = thereminService.processMiniRequest(request, request.input.semanticFrames[0])
        Assertions.assertTrue(response.isRelevant)
        val runResponse = response as RunOnlyResponse<ThereminState>
        MatcherAssert.assertThat(
            thereminService.STATION_DISCLAIMER,
            CoreMatchers.hasItem((runResponse.body as ScenarioResponseBody).layout.getOutputSpeechO().get())
        )
        MatcherAssert.assertThat((runResponse.body as ScenarioResponseBody).layout.directives, Matchers.empty())
    }

    @Test
    fun assertCorrectSizeOfInstruments() {
        val compile = Pattern.compile("\\d+")
        for (text in thereminService.OUT_OF_RANGE_INDEX_RESPONSES) {
            val matcher = compile.matcher(text)
            if (matcher.find()) {
                val s = matcher.group().toInt()
                Assertions.assertEquals(Instrument.size(), s, "Wrong size in text: $text")
            }
        }
    }

    private fun createRequest(input: TInput, experiment: String? = null): MegaMindRequest<ThereminState> {
        val slots = ArrayList<SemanticFrameSlot>()
        if (input.beatTextSlot != null) {
            slots.add(SemanticFrameSlot("beat_text", null, input.beatTextSlot, listOf()))
        }
        if (input.beatEnumSlot != null) {
            slots.add(SemanticFrameSlot("beat_enum", null, input.beatEnumSlot, listOf()))
        }
        if (input.beatNumberSlot != null) {
            slots.add(
                SemanticFrameSlot(
                    "beat_number", null, input.beatNumberSlot.toString(),
                    listOf()
                )
            )
        }
        if (input.beatGroupSlot != null) {
            slots.add(SemanticFrameSlot("beat_group", null, input.beatGroupSlot, listOf()))
        }
        if (input.beanGroupIndexSlot != null) {
            slots.add(
                SemanticFrameSlot(
                    "beat_group_index", null,
                    input.beanGroupIndexSlot.toString(), listOf()
                )
            )
        }
        if (input.mielophone) {
            slots.add(SemanticFrameSlot("mielophone", null, input.text, listOf()))
        }
        val textInput = Input.Text(
            originalUtterance = "дай звук " + input.text,
            normalizedUtterance = "дай звук " + input.text,
            semanticFrames = listOf(
                create(THEREMIN_PLAY_SEMANTIC_FRAME_NAME, slots)
            )
        )
        return MegaMindRequest(
            requestId = "req-id",
            dialogId = null,
            scenarioMeta = THEREMIN,
            serverTime = Instant.now(),
            randomSeed = 0L,
            experiments = if (experiment != null) mutableSetOf(experiment) else mutableSetOf(),
            clientInfo = stationMini(ClientInfoTestUtils.DEFAULT_UUID),
            state = null,
            input = textInput,
            voiceSession = true,
        )
    }

    internal class TInput(
        val text: String,
        val beatNumberSlot: Int?,
        val beatTextSlot: String?,
        val beatEnumSlot: String?,
        val beatGroupSlot: String?,
        val beanGroupIndexSlot: Int?,
        private val currentModeIndex: Int?,
        val mielophone: Boolean,
        val expectedMode: Int
    ) {
        override fun toString(): String {
            return text
        }

        companion object {
            fun builder(expectedMode: Int, text: String?): TInputBuilder {
                return TInputBuilder().expectedMode(expectedMode).text(text)
            }

            fun mielophone(): TInput {
                return TInputBuilder().text /*expectedMode(0).*/("Доставай миелофон").mielophone(true).build()
            }
        }
    }

    class TInputBuilder {
        private var text: String? = null
        private var beatNumberSlot: Int? = null
        private var beatTextSlot: String? = null
        private var beatEnumSlot: String? = null
        private var beatGroupSlot: String? = null
        private var beanGroupIndexSlot: Int? = null
        private var currentModeIndex: Int? = null
        private var mielophone = false
        private var expectedMode = 0
        fun text(text: String?): TInputBuilder {
            this.text = text
            return this
        }

        fun beatNumberSlot(beatNumberSlot: Int?): TInputBuilder {
            this.beatNumberSlot = beatNumberSlot
            return this
        }

        fun beatTextSlot(beatTextSlot: String?): TInputBuilder {
            this.beatTextSlot = beatTextSlot
            return this
        }

        fun beatEnumSlot(beatEnumSlot: String?): TInputBuilder {
            this.beatEnumSlot = beatEnumSlot
            return this
        }

        fun beatGroupSlot(beatGroupSlot: String?): TInputBuilder {
            this.beatGroupSlot = beatGroupSlot
            return this
        }

        fun beanGroupIndexSlot(beanGroupIndexSlot: Int?): TInputBuilder {
            this.beanGroupIndexSlot = beanGroupIndexSlot
            return this
        }

        fun currentModeIndex(currentModeIndex: Int?): TInputBuilder {
            this.currentModeIndex = currentModeIndex
            return this
        }

        fun mielophone(mielophone: Boolean): TInputBuilder {
            this.mielophone = mielophone
            return this
        }

        fun expectedMode(expectedMode: Int): TInputBuilder {
            this.expectedMode = expectedMode
            return this
        }

        fun build(): TInput {
            return TInput(
                text!!,
                beatNumberSlot,
                beatTextSlot,
                beatEnumSlot,
                beatGroupSlot,
                beanGroupIndexSlot,
                currentModeIndex,
                mielophone,
                expectedMode
            )
        }
    }

    companion object {
        @JvmStatic
        fun arguments(): Stream<TInput> {
            return Stream.of(
                TInput.builder(1, "фортепиано один").beatGroupSlot("piano").beanGroupIndexSlot(1).build(),
                TInput.builder(2, "фортепиано два").beatGroupSlot("piano").beanGroupIndexSlot(2).build(),
                TInput.builder(1, "фортепиано").beatEnumSlot("piano").build(),
                TInput.builder(1, "фортепиано номер один").beatGroupSlot("piano").beanGroupIndexSlot(1)
                    .build(),  //Input.builder(4, "гитара два").beatGroupSlot("guitar").beanGroupIndexSlot(2).build(),
                TInput.builder(4, "гитара аккорды").beatEnumSlot("guitar_chords").build(),
                TInput.builder(5, "барабанов 1").beatGroupSlot("drums").beanGroupIndexSlot(1).build(),
                TInput.builder(5, "барабанов").beatEnumSlot("drums")
                    .build(),  //Input.builder(4, "вочьмидесятых").beatEnumSlot("eighties").build(),
                //Input.builder(4, "вочьмидесятых один").beatEnumSlot("eighties").beanGroupIndexSlot(1).build(),
                TInput.builder(4, "четыре").beatNumberSlot(4).build(),
                TInput.builder(1, "пианино 1").beatGroupSlot("piano").beanGroupIndexSlot(1).build(),
                TInput.builder(1, "пианино").beatEnumSlot("piano").build(),
                TInput.builder(1, "1").beatNumberSlot(1).build(),
                TInput.builder(2, "пианино 2").beatGroupSlot("piano").beanGroupIndexSlot(2).build(),
                TInput.builder(2, "цифрового пианино").beatEnumSlot("digital_piano").build(),
                TInput.builder(2, "2").beatNumberSlot(2).build(),
                TInput.builder(3, "гитара 2").beatGroupSlot("guitar").beanGroupIndexSlot(2).build(),
                TInput.builder(3, "гитары на нейлоне").beatEnumSlot("nylon_guitar").build(),
                TInput.builder(3, "3").beatNumberSlot(3).build(),
                TInput.builder(4, "гитара 3").beatGroupSlot("guitar").beanGroupIndexSlot(3).build(),
                TInput.builder(4, "гитарных аккордов").beatEnumSlot("guitar_chords").build(),
                TInput.builder(4, "4").beatNumberSlot(4).build(),
                TInput.builder(5, "барабаны 1").beatGroupSlot("drums").beanGroupIndexSlot(1).build(),
                TInput.builder(5, "барабанов").beatEnumSlot("drums").build(),
                TInput.builder(5, "5").beatNumberSlot(5).build(),
                TInput.builder(6, "барабаны 2").beatGroupSlot("drums").beanGroupIndexSlot(2).build(),
                TInput.builder(6, "дарбука").beatEnumSlot("jambei").build(),
                TInput.builder(6, "6").beatNumberSlot(6).build(),
                TInput.builder(7, "скрипка 1").beatGroupSlot("violin").beanGroupIndexSlot(1).build(),
                TInput.builder(7, "скрипки").beatEnumSlot("violin").build(),
                TInput.builder(7, "7").beatNumberSlot(7).build(),
                TInput.builder(8, "скрипка 2").beatGroupSlot("violin").beanGroupIndexSlot(2).build(),
                TInput.builder(8, "кап+ели").beatEnumSlot("pizzicato").build(),
                TInput.builder(8, "8").beatNumberSlot(8).build(),
                TInput.builder(9, "бас 2").beatGroupSlot("bass").beanGroupIndexSlot(2).build(),
                TInput.builder(9, "хаус").beatEnumSlot("house").build(),
                TInput.builder(9, "9").beatNumberSlot(9).build(),
                TInput.builder(10, "мультфильма").beatEnumSlot("cartoon").build(),
                TInput.builder(10, "10").beatNumberSlot(10).build(),
                TInput.builder(11, "барабаны 3").beatGroupSlot("drums").beanGroupIndexSlot(3).build(),
                TInput.builder(11, "бамбука").beatEnumSlot("creen").build(),
                TInput.builder(11, "11").beatNumberSlot(11).build(),
                TInput.builder(12, "колокольчик 1").beatGroupSlot("bells").beanGroupIndexSlot(1).build(),
                TInput.builder(12, "колокольчика").beatEnumSlot("bells").build(),
                TInput.builder(12, "12").beatNumberSlot(12).build(),
                TInput.builder(13, "бас 4").beatGroupSlot("bass").beanGroupIndexSlot(4).build(),
                TInput.builder(13, "бас-гитары").beatEnumSlot("bass_guitar").build(),
                TInput.builder(13, "13").beatNumberSlot(13).build(),
                TInput.builder(14, "орган 1").beatGroupSlot("organ").beanGroupIndexSlot(1).build(),
                TInput.builder(14, "органа").beatEnumSlot("organ").build(),
                TInput.builder(14, "14").beatNumberSlot(14).build(),
                TInput.builder(15, "пианино 3").beatGroupSlot("piano").beanGroupIndexSlot(3).build(),
                TInput.builder(15, "аккордов на пианино").beatEnumSlot("piano_chords").build(),
                TInput.builder(15, "15").beatNumberSlot(15).build(),
                TInput.builder(16, "орган 2").beatGroupSlot("organ").beanGroupIndexSlot(2).build(),
                TInput.builder(16, "спортивного органа").beatEnumSlot("hockey").build(),
                TInput.builder(16, "16").beatNumberSlot(16).build(),
                TInput.builder(17, "вокал 1").beatGroupSlot("voice").beanGroupIndexSlot(1).build(),
                TInput.builder(17, "вокала").beatEnumSlot("voice").build(),
                TInput.builder(17, "17").beatNumberSlot(17).build(),
                TInput.builder(18, "оружие 1").beatGroupSlot("weapons").beanGroupIndexSlot(1).build(),
                TInput.builder(18, "мечей").beatEnumSlot("blades").build(),
                TInput.builder(18, "18").beatNumberSlot(18).build(),
                TInput.builder(19, "электроника 3").beatGroupSlot("digital").beanGroupIndexSlot(3).build(),
                TInput.builder(19, "киберпанка").beatEnumSlot("cyberpunk").build(),
                TInput.builder(19, "19").beatNumberSlot(19).build(),
                TInput.builder(20, "электроника 4").beatGroupSlot("digital").beanGroupIndexSlot(4).build(),
                TInput.builder(20, "свистка").beatEnumSlot("birdy").build(),
                TInput.builder(20, "20").beatNumberSlot(20).build(),
                TInput.builder(21, "гармоника 1").beatGroupSlot("harmonic").beanGroupIndexSlot(1).build(),
                TInput.builder(21, "губной гармошки").beatEnumSlot("harmonic").build(),
                TInput.builder(21, "21").beatNumberSlot(21).build(),
                TInput.builder(22, "электроника 2").beatGroupSlot("digital").beanGroupIndexSlot(2).build(),
                TInput.builder(22, "приставки").beatEnumSlot("dendi").build(),
                TInput.builder(22, "22").beatNumberSlot(22).build(),
                TInput.builder(23, "бас 1").beatGroupSlot("bass").beanGroupIndexSlot(1).build(),
                TInput.builder(23, "баса").beatEnumSlot("bass").build(),
                TInput.builder(23, "23").beatNumberSlot(23).build(),
                TInput.builder(24, "электроника 1").beatGroupSlot("digital").beanGroupIndexSlot(1).build(),
                TInput.builder(24, "помех").beatEnumSlot("glitch").build(),
                TInput.builder(24, "24").beatNumberSlot(24).build(),
                TInput.builder(25, "орган 3").beatGroupSlot("organ").beanGroupIndexSlot(3).build(),
                TInput.builder(25, "электрооргана").beatEnumSlot("qbite").build(),
                TInput.builder(25, "25").beatNumberSlot(25).build(),
                TInput.builder(26, "гитара 1").beatGroupSlot("guitar").beanGroupIndexSlot(1).build(),
                TInput.builder(26, "гитары").beatEnumSlot("guitar").build(),
                TInput.builder(26, "26").beatNumberSlot(26).build(),
                TInput.builder(27, "колокольчик 2").beatGroupSlot("bells").beanGroupIndexSlot(2).build(),
                TInput.builder(27, "кастрюли").beatEnumSlot("pot").build(),
                TInput.builder(27, "27").beatNumberSlot(27).build(),
                TInput.builder(28, "космос 3").beatGroupSlot("space").beanGroupIndexSlot(3).build(),
                TInput.builder(28, "марса").beatEnumSlot("mars").build(),
                TInput.builder(28, "28").beatNumberSlot(28).build(),
                TInput.builder(29, "космос 1").beatGroupSlot("space").beanGroupIndexSlot(1).build(),
                TInput.builder(29, "космоса").beatEnumSlot("space").build(),
                TInput.builder(29, "29").beatNumberSlot(29).build(),
                TInput.builder(30, "космос 4").beatGroupSlot("space").beanGroupIndexSlot(4).build(),
                TInput.builder(30, "космического пианино").beatEnumSlot("stream").build(),
                TInput.builder(30, "30").beatNumberSlot(30).build(),
                TInput.builder(31, "космос 2").beatGroupSlot("space").beanGroupIndexSlot(2).build(),
                TInput.builder(31, "космического сигнала").beatEnumSlot("toy").build(),
                TInput.builder(31, "31").beatNumberSlot(31).build(),
                TInput.builder(32, "ксилофона").beatEnumSlot("vibraphone").build(),
                TInput.builder(32, "32").beatNumberSlot(32).build(),
                TInput.builder(33, "флейта 1").beatGroupSlot("flute").beanGroupIndexSlot(1).build(),
                TInput.builder(33, "флейты").beatEnumSlot("flute").build(),
                TInput.builder(33, "33").beatNumberSlot(33).build(),
                TInput.builder(34, "барабаны 4").beatGroupSlot("drums").beanGroupIndexSlot(4).build(),
                TInput.builder(34, "глухого барабана").beatEnumSlot("african_percussion").build(),
                TInput.builder(34, "34").beatNumberSlot(34).build(),
                TInput.builder(35, "электроника 5").beatGroupSlot("digital").beanGroupIndexSlot(5).build(),
                TInput.builder(35, "тёрки").beatEnumSlot("dubstep").build(),
                TInput.builder(35, "35").beatNumberSlot(35).build(),
                TInput.builder(36, "гитара 4").beatGroupSlot("guitar").beanGroupIndexSlot(4).build(),
                TInput.builder(36, "русского рока").beatEnumSlot("russian_rock").build(),
                TInput.builder(36, "36").beatNumberSlot(36).build(),
                TInput.builder(37, "гитара 5").beatGroupSlot("guitar").beanGroupIndexSlot(5).build(),
                TInput.builder(37, "акустической гитары").beatEnumSlot("acoustic_guitar").build(),
                TInput.builder(37, "37").beatNumberSlot(37).build(),
                TInput.builder(38, "бас 3").beatGroupSlot("bass").beanGroupIndexSlot(3).build(),
                TInput.builder(38, "клубного бита").beatEnumSlot("club").build(),
                TInput.builder(38, "38").beatNumberSlot(38).build(),
                TInput.builder(39, "колокольчик 3").beatGroupSlot("bells").beanGroupIndexSlot(3).build(),
                TInput.builder(39, "перезвона").beatEnumSlot("campanella").build(),
                TInput.builder(39, "39").beatNumberSlot(39).build(),
                TInput.builder(40, "животные 1").beatGroupSlot("animal").beanGroupIndexSlot(1).build(),
                TInput.builder(40, "кота").beatEnumSlot("cat").build(),
                TInput.builder(40, "40").beatNumberSlot(40).build(),
                TInput.builder(41, "струнные 1").beatGroupSlot("strings").beanGroupIndexSlot(1).build(),
                TInput.builder(41, "барабанов кото").beatEnumSlot("koto").build(),
                TInput.builder(41, "41").beatNumberSlot(41).build(),
                TInput.builder(42, "электроника 6").beatGroupSlot("digital").beanGroupIndexSlot(6).build(),
                TInput.builder(42, "пищалки").beatEnumSlot("toy_synths").build(),
                TInput.builder(42, "42").beatNumberSlot(42).build(),
                TInput.builder(43, "вокал 2").beatGroupSlot("voice").beanGroupIndexSlot(2).build(),
                TInput.builder(43, "мужского голоса").beatEnumSlot("male_voice").build(),
                TInput.builder(43, "43").beatNumberSlot(43).build(),
                TInput.builder(44, "барабаны 5").beatGroupSlot("drums").beanGroupIndexSlot(5).build(),
                TInput.builder(44, "перкуссии").beatEnumSlot("percussion").build(),
                TInput.builder(44, "44").beatNumberSlot(44).build(),
                TInput.builder(45, "животные 2").beatGroupSlot("animal").beanGroupIndexSlot(2).build(),
                TInput.builder(45, "собаки").beatEnumSlot("dog").build(),
                TInput.builder(45, "45").beatNumberSlot(45).build(),
                TInput.builder(46, "электроника 7").beatGroupSlot("digital").beanGroupIndexSlot(7).build(),
                TInput.builder(46, "лазера").beatEnumSlot("sci_fi").build(),
                TInput.builder(46, "46").beatNumberSlot(46).build(),
                TInput.builder(47, "электроника 8").beatGroupSlot("digital").beanGroupIndexSlot(8).build(),
                TInput.builder(47, "стыковки").beatEnumSlot("trap").build(),
                TInput.builder(47, "47").beatNumberSlot(47).build(),
                TInput.builder(48, "электроника 10").beatGroupSlot("digital").beanGroupIndexSlot(10).build(),
                TInput.builder(48, "дроида").beatEnumSlot("r2d2").build(),
                TInput.builder(48, "48").beatNumberSlot(48).build(),
                TInput.builder(49, "колокольчик 4").beatGroupSlot("bells").beanGroupIndexSlot(4).build(),
                TInput.builder(49, "колокола").beatEnumSlot("chime").build(),
                TInput.builder(49, "49").beatNumberSlot(49).build(),
                TInput.builder(50, "струнные 2").beatGroupSlot("strings").beanGroupIndexSlot(2).build(),
                TInput.builder(50, "арфы").beatEnumSlot("harp").build(),
                TInput.builder(50, "50").beatNumberSlot(50).build(),
                TInput.builder(51, "гитара 6").beatGroupSlot("guitar").beanGroupIndexSlot(6).build(),
                TInput.builder(51, "электрогитары").beatEnumSlot("electric_guitar").build(),
                TInput.builder(51, "51").beatNumberSlot(51).build(),
                TInput.builder(52, "гитара 7").beatGroupSlot("guitar").beanGroupIndexSlot(7).build(),
                TInput.builder(52, "соло").beatEnumSlot("solo").build(),
                TInput.builder(52, "52").beatNumberSlot(52).build(),
                TInput.builder(53, "электроника 9").beatGroupSlot("digital").beanGroupIndexSlot(9).build(),
                TInput.builder(53, "фаталити").beatEnumSlot("mk").build(),
                TInput.builder(53, "53").beatNumberSlot(53).build(),
                TInput.builder(54, "гармоника 2").beatGroupSlot("harmonic").beanGroupIndexSlot(2).build(),
                TInput.builder(54, "баяна").beatEnumSlot("bayan").build(),
                TInput.builder(54, "54").beatNumberSlot(54).build(),
                TInput.builder(55, "барабаны 6").beatGroupSlot("drums").beanGroupIndexSlot(6).build(),
                TInput.builder(55, "аркады").beatEnumSlot("arcade").build(),
                TInput.builder(55, "55").beatNumberSlot(55).build(),
                TInput.builder(56, "гитара 8").beatGroupSlot("guitar").beanGroupIndexSlot(8).build(),
                TInput.builder(56, "белый снег").beatEnumSlot("tsoy").build(),
                TInput.builder(56, "56").beatNumberSlot(56).build(),
                TInput.builder(57, "гитара 9").beatGroupSlot("guitar").beanGroupIndexSlot(9).build(),
                TInput.builder(57, "колдуна").beatEnumSlot("kish").build(),
                TInput.builder(57, "57").beatNumberSlot(57).build(),
                TInput.builder(58, "колокольчик 5").beatGroupSlot("bells").beanGroupIndexSlot(5).build(),
                TInput.builder(58, "клавесина").beatEnumSlot("glockenspiel").build(),
                TInput.builder(58, "58").beatNumberSlot(58).build()
            )
        }
    }
}
