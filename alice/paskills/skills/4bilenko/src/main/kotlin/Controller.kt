package ru.yandex.alice.paskills.skills.bilenko

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.databind.node.ObjectNode
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.web.bind.annotation.PostMapping
import org.springframework.web.bind.annotation.RequestBody
import org.springframework.web.bind.annotation.RequestMapping
import org.springframework.web.bind.annotation.RestController
import ru.yandex.alice.paskills.skills.bilenko.model.WebhookRequest
import ru.yandex.alice.paskills.skills.bilenko.model.WebhookResponse

@RestController
class Controller {

    private val dispatcher = dispatcher()

    @Autowired
    private lateinit var objectMapper: ObjectMapper

    @RequestMapping("/ping")
    fun ping() = "ok"

    @PostMapping(path = ["/", "/webhook"])
    suspend fun webhook(@RequestBody body: WebhookRequest<SessionState, ObjectNode>): WebhookResponse<out SessionState, out ObjectNode> {
        if (body.request.command != "ping") {
            println("command: {${body.request.command}} nlu: {${body.request.nlu}} session_state: {${body.state?.session}} " +
                "uuid: {${body.session.application.applicationId}} user_id: {${body.session.user?.userId}}")
        }
        val response = dispatcher.handle(body)
        val s = objectMapper.writeValueAsString(response)
        return response
    }

}
