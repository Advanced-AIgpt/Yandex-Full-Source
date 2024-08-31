package ru.yandex.alice.divktrenderer.projects.example

import com.yandex.div.dsl.context.*
import com.yandex.div.dsl.model.*
import com.yandex.div.dsl.reference
import com.yandex.div.dsl.type.color
import com.yandex.div.dsl.value
import ru.yandex.alice.divkit.IdempotentDivKitTemplate
import ru.yandex.alice.divkit.StaticContext
import ru.yandex.alice.protos.data.scenario.example.Data.TExampleScenarioData
import kotlin.reflect.KClass

class ExampleDiv() : IdempotentDivKitTemplate<TExampleScenarioData> {
    override val templateDataClass: KClass<TExampleScenarioData>
        get() = TExampleScenarioData::class

    private val greetingsTextRef = reference<String>("greetings_text")

    override fun render(ctx: StaticContext, data: TExampleScenarioData): CardWithTemplates {
        val templates: TemplateContext<Div> = templates {
            define(
                "greetings",
                divText(
                    text = greetingsTextRef,
                    textColor = value("#ff0".color)
                )
            )
        }
        val card = card {
            divData(
                logId = "example_card",
                states = listOf(
                    state(
                        stateId = 0,
                        div = divContainer(
                            items = listOf(
                                greetings(data)
                            )
                        )
                    )
                )
            )
        }
        return CardWithTemplates(card, templates)
    }

    private fun CardContext.greetings(data: TExampleScenarioData) = template(
        "greetings",
        resolve(greetingsTextRef, data.hello ?: " ")
    )
}
