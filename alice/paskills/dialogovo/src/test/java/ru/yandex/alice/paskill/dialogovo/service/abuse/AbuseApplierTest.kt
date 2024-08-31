package ru.yandex.alice.paskill.dialogovo.service.abuse

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.readValue
import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Test
import org.skyscreamer.jsonassert.JSONAssert
import org.skyscreamer.jsonassert.JSONCompareMode
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig
import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider
import ru.yandex.alice.paskill.dialogovo.domain.Censored
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse
import ru.yandex.alice.paskill.dialogovo.utils.AnnotationWalker
import ru.yandex.alice.paskill.dialogovo.utils.ResourceUtils
import ru.yandex.alice.paskill.dialogovo.utils.ValuePathWrapper
import java.util.Optional
import java.util.function.Supplier

@SpringJUnitConfig(classes = [TestConfigProvider::class])
class AbuseApplierTest {
    @Autowired
    private lateinit var objectMapper: ObjectMapper

    private val applier = AbuseApplierImpl(object : AbuseService {
        override fun checkAbuse(docs: List<AbuseDocument>): Map<String, String> {
            val res = HashMap<String, String>()
            for (doc in docs) {
                res[doc.id] =
                    if (doc.value == "блять ты че совсем охуел") "<censored>блять</censored> ты че совсем <censored>охуел</censored>" else doc.value
            }
            return res
        }
    })

    @Test
    fun bigImageCard() {
        testImpl("big_image_card")
    }

    @Test
    fun itemsListCard() {
        testImpl("items_list_card")
    }

    @Test
    fun simpleButtons() {
        testImpl("simple_buttons")
    }

    private fun testImpl(fileName: String) {
        val response = ResourceUtils.getStringResource("abuse/$fileName.json")
        val expected = ResourceUtils.getStringResource("abuse/" + fileName + "_expected.json")
        val obj = objectMapper.readValue<WebhookResponse>(response)
        applier.apply(obj)
        JSONAssert.assertEquals(expected, objectMapper.writeValueAsString(obj), JSONCompareMode.LENIENT)
    }

    @Test
    fun traverseObject() {
        val root = AbuseTestObject(
            "text1",
            AbuseTestInner("text2"),
            null,
            listOf(
                AbuseTestInner("text2-0"),
                AbuseTestInner("text2-1"),
                AbuseTestInner("text2-2")
            ),
            Optional.of("opt-text1"),
            Optional.of(AbuseTestInner("opt-text2")),
            Optional.of(
                listOf(AbuseTestInner("opt-iter-1"))
            )
        )
        val walker = AnnotationWalker(Censored::class.java)
        val items = walker.traverseObject(root)
        Assertions.assertEquals(8, items.size)
        assertValuePathWrapper(items, "censored1", "text1", "text1-mod") { root.censored1 }
        assertValuePathWrapper(items, "inner.censored2", "text2", "text2-mod") { root.inner.censored2 }
        assertValuePathWrapper(items, "optCensored", "opt-text1", "opt-text1") { root.optCensored.get() }
        assertValuePathWrapper(
            items, "optInner.censored2", "opt-text2", "opt-text2-mod"
        ) { root.optInner.get().censored2 }
        assertValuePathWrapper(
            items, "optInnerIterable.[0].censored2", "opt-iter-1", "opt-iter-1-mod"
        ) { root.optInnerIterable.get().get(0).censored2 }
        assertValuePathWrapper(
            items, "innerIterable.[0].censored2", "text2-0", "text2-0-mod"
        ) { root.innerIterable.get(0).censored2 }
        assertValuePathWrapper(
            items, "innerIterable.[1].censored2", "text2-1", "text2-1-mod"
        ) { root.innerIterable.get(1).censored2 }
        Assertions.assertTrue(items.firstOrNull { it.path == "innerNull" } == null)
    }

    @Throws(IllegalAccessException::class)
    fun assertValuePathWrapper(
        items: List<ValuePathWrapper<Censored>>,
        path: String,
        value: String?,
        afterApply: String,
        supplier: Supplier<String>
    ) {
        val optItem = items.stream().filter { x: ValuePathWrapper<Censored> -> x.path == path }.findFirst()
        Assertions.assertTrue(optItem.isPresent)
        val item = optItem.get()
        Assertions.assertEquals(item.value, value)
        item.apply(afterApply)
        Assertions.assertEquals(afterApply, supplier.get())
    }

    @Censored
    data class AbuseTestObject(
        @Censored
        var censored1: String,
        var inner: AbuseTestInner,
        var innerNull: AbuseTestInner?,
        var innerIterable: List<AbuseTestInner>,

        @Censored
        var optCensored: Optional<String>,
        var optInner: Optional<AbuseTestInner>,
        var optInnerIterable: Optional<List<AbuseTestInner>>,
    )

    @Censored
    data class AbuseTestInner(
        @Censored
        var censored2: String
    )
}
