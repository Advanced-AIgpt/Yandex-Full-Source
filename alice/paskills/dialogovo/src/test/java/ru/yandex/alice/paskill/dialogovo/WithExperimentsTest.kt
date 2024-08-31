package ru.yandex.alice.paskill.dialogovo

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test
import ru.yandex.alice.kronstadt.core.WithExperiments

private data class WithExperimentsImpl(
    override val experiments: Set<String>
) : WithExperiments {
    constructor(vararg experiments: String) : this(experiments.toSet())
}

class WithExperimentsTest {

    private val prefix = "accessible_private_skills="

    private fun assertEqualsIgnoreOrder(expected: List<String>, actual: List<String>) {
        assertEquals(expected.toSet(), actual.toSet())
    }

    @Test
    fun testEmptyValuesList() {
        val experiments = WithExperimentsImpl()
        assertEqualsIgnoreOrder(
            experiments.getExperimentsWithCommaSeparatedListValues(prefix, emptyList()),
            listOf<String>()
        )
    }

    @Test
    fun testCommaSeparatedList() {
        val experiments = WithExperimentsImpl(
            "accessible_private_skills=62e46116-0fe6-4373-ba1d-8e96087d02e1,8fbfeaea-6269-46ee-9098-2202e3b8ff64"
        )
        assertEqualsIgnoreOrder(
            experiments.getExperimentsWithCommaSeparatedListValues(prefix, emptyList()),
            listOf("62e46116-0fe6-4373-ba1d-8e96087d02e1", "8fbfeaea-6269-46ee-9098-2202e3b8ff64")
        )
    }

    @Test
    fun testMultipleFlags() {
        val experiments = WithExperimentsImpl(
            "accessible_private_skills=62e46116-0fe6-4373-ba1d-8e96087d02e1",
            "accessible_private_skills=8fbfeaea-6269-46ee-9098-2202e3b8ff64"
        )
        assertEqualsIgnoreOrder(
            experiments.getExperimentsWithCommaSeparatedListValues(prefix, emptyList()),
            listOf("62e46116-0fe6-4373-ba1d-8e96087d02e1", "8fbfeaea-6269-46ee-9098-2202e3b8ff64")
        )
    }

    @Test
    fun testMultipleCommaSeparatedFlags() {
        val experiments = WithExperimentsImpl(
            "accessible_private_skills=62e46116-0fe6-4373-ba1d-8e96087d02e1,8fbfeaea-6269-46ee-9098-2202e3b8ff64",
            "accessible_private_skills=a147164a-f7bb-4756-8dd3-29053af4428f,520067f9-3ccf-4ca0-9c9c-a7b46e9cacae"
        )
        assertEqualsIgnoreOrder(
            experiments.getExperimentsWithCommaSeparatedListValues(prefix, emptyList()),
            listOf(
                "62e46116-0fe6-4373-ba1d-8e96087d02e1",
                "8fbfeaea-6269-46ee-9098-2202e3b8ff64",
                "a147164a-f7bb-4756-8dd3-29053af4428f",
                "520067f9-3ccf-4ca0-9c9c-a7b46e9cacae"
            )
        )
    }
}
