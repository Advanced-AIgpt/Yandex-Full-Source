package ru.yandex.alice.kronstadt.scenarios.contacts

import com.google.protobuf.Message
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArguments
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoStateScenario
import ru.yandex.alice.kronstadt.core.scenario.SelectedScene
import ru.yandex.kronstadt.alice.scenarios.contacts.proto.TUploadContactsApplyArgs

val CONTACTS = ScenarioMeta("contacts", "Contacts", "contacts")

// Intents
const val UPLOAD_CONTACTS_INTENT = "alice_scenarios.upload_contact_request"
const val UPDATE_CONTACTS_INTENT = "alice_scenarios.update_contact_request"

// Frames
const val UPLOAD_CONTACTS_REQUEST_FRAME = "alice.upload_contact_request"
const val UPDATE_CONTACTS_REQUEST_FRAME = "alice.update_contact_request"

object UploadContactsApplyArgs : ApplyArguments

@Component
class ContactsScenario : AbstractNoStateScenario(CONTACTS) {

    override fun applyArgumentsToProto(applyArguments: ApplyArguments): Message {
        return TUploadContactsApplyArgs.getDefaultInstance()
    }

    override fun applyArgumentsFromProto(arg: com.google.protobuf.Any): ApplyArguments {
        return UploadContactsApplyArgs
    }

    override fun selectScene(request: MegaMindRequest<Any>): SelectedScene.Running<Any, *>? = request.handle {
        onFrame(UPDATE_CONTACTS_REQUEST_FRAME) { frame ->
            onCondition({
                frame.typedSemanticFrame != null
                    && frame.typedSemanticFrame!!.hasUpdateContactsRequestSemanticFrame()
            })
            {
                sceneWithArgs(
                    UploadContactsScene::class,
                    UploadContactsArgs(fullUpload = false)
                )
            }
        }
        onFrame(UPLOAD_CONTACTS_REQUEST_FRAME) { frame ->
            onCondition({
                frame.typedSemanticFrame != null
                    && frame.typedSemanticFrame!!.hasUploadContactsRequestSemanticFrame()
            })
            {
                sceneWithArgs(
                    UploadContactsScene::class,
                    UploadContactsArgs(fullUpload = true)
                )
            }
        }
    }
}
