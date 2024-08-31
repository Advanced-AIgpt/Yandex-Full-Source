package ru.yandex.alice.kronstadt.scenarios.alice4business

import org.springframework.beans.factory.annotation.Value
import org.springframework.stereotype.Service
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse
import ru.yandex.alice.kronstadt.core.IrrelevantResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.directive.ModroviaCallbackDirective
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoStateScenario
import ru.yandex.alice.kronstadt.core.scenario.SelectedScene
import ru.yandex.alice.kronstadt.scenarios.alice4business.SayActivationCodeScene.Arg

internal const val ACTIVATION_CODE_FALLBACK = "0000"
internal const val WEBVIEW_COMMAND_GET_CODE = "alice4business:get_code"
internal const val WEBVIEW_COMMAND_SET_CODE = "alice4business:set_code"
internal const val WEBVIEW_EXPECTED_SCREEN_NAME = "device_lock"
internal const val WEBVIEW_VIEW_KEY = "alice4business:device_lock"

internal const val ALICE4BUSINESS_DEVICE_LOCK_IRRELEVANT = "alice4business.device_lock.irrelevant"

// Semantic frames
internal const val ALICE4BUSINESS_DEVICE_LOCK_REPEAT_CODE = "alice.4business.device_lock.repeat_code"

// Experiments
internal const val DO_NOT_CACHE_UNLOCKED_B2B_DEVICE_STATE = "do_not_cache_unlocked_b2b_device_state"

@Service
open class Alice4BusinessScenario(
    private val alice4BusinessService: Alice4BusinessService,
    @Value("\${alice4-business-config.cacheUnlockedDeviceState}")
    private val cacheUnlockedDeviceState: Boolean
) : AbstractNoStateScenario(ScenarioMeta("alice4Business", "Alice4Business", "alice_for_business", "alice4business")) {

    override val irrelevantResponseFactory: IrrelevantResponse.Factory<Any> =
        DefaultIrrelevantResponse.Factory(ALICE4BUSINESS_DEVICE_LOCK_IRRELEVANT)

    override fun selectScene(request: MegaMindRequest<Any>): SelectedScene.Running<Any, *>? = request.handle {
        onClient(ClientInfo::isYaSmartDevice) {
            onCondition({ alice4BusinessService.isBusinessDevice(clientInfo.deviceId) }) {
                val deviceIdentifier = clientInfo.deviceId!!
                val deviceLockState = getDeviceLockState(request, deviceIdentifier)

                onCallback<ModroviaCallbackDirective> { directive ->
                    onCondition({ directive.command == WEBVIEW_COMMAND_GET_CODE }) {

                        if (deviceLockState == null) {
                            sceneWithArgs(LockFailureScene::class, true)
                        } else if (!deviceLockState.locked) {
                            scene<StationUnlockedScene>()
                        } else {
                            //scene(GetCodeScene, deviceLockState)
                            sceneWithArgs(GetCodeScene::class, deviceLockState)
                        }
                    }
                }

                if (deviceLockState == null) {
                    if (isWebviewOpen(request)) {
                        sceneWithArgs(LockFailureScene::class, false)
                    } else {
                        // skip processing so another scenario answers
                        null
                    }
                } else if (!deviceLockState.locked) {
                    if (isWebviewOpen(request)) {
                        scene<StationUnlockedScene>()
                    } else {
                        // skip processing so another scenario answers
                        null
                    }
                } else {
                    onFrame(ALICE4BUSINESS_DEVICE_LOCK_REPEAT_CODE) {
                        sceneWithArgs(
                            SayActivationCodeScene::class,
                            Arg(true, deviceLockState.code, deviceLockState.stationUrl)
                        )
                    }
                    sceneWithArgs(
                        SayActivationCodeScene::class,
                        Arg(false, deviceLockState.code, deviceLockState.stationUrl)
                    )
                }
            }
        }
    }

    private fun getDeviceLockState(request: MegaMindRequest<*>, deviceIdentifier: String): DeviceLockState? {
        return if (cacheUnlockedDeviceState &&
            !request.hasExperiment(DO_NOT_CACHE_UNLOCKED_B2B_DEVICE_STATE) &&
            request.options.quasarAuxiliaryConfig.alice4Business?.unlocked == true
        ) {
            UNLOCKED_DEVICE_STATE
        } else {
            alice4BusinessService.getDeviceLockState(deviceIdentifier, request.serverTime)
        }
    }
}

internal fun isWebviewOpen(request: MegaMindRequest<*>): Boolean {
    return request.mordoviaState["alice4business"] == true &&
        request.mordoviaState["currentScreen"] == WEBVIEW_EXPECTED_SCREEN_NAME
}
