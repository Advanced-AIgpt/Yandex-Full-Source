package ru.yandex.alice.kronstadt.server.http

import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Test
import org.springframework.boot.test.context.SpringBootTest
import org.springframework.boot.test.context.TestComponent
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoStateScenario
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.core.scenario.SelectedScene

@SpringBootTest(
    classes = [TestApp::class, TestStartup.TestScenario::class, TestStartup.TestScene::class],
    properties = ["tvm.mode=disabled"]
)
class TestStartup {

    @Test
    internal fun test() {
        Assertions.assertTrue(true)
    }

    @TestComponent
    class TestScenario : AbstractNoStateScenario(ScenarioMeta("test_scenario")) {

        override fun selectScene(request: MegaMindRequest<Any>): SelectedScene.Running<Any, *>? {
            throw RuntimeException("failure")
        }
    }

    @TestComponent
    class TestScene : AbstractNoargScene<Any>("test_scene") {
        override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
            throw RuntimeException("failure")
        }
    }
}
