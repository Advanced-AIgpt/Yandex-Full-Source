package ru.yandex.alice.paskills.skills.bilenko

import com.fasterxml.jackson.databind.ObjectMapper
import org.junit.jupiter.api.Test
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.boot.test.autoconfigure.web.reactive.WebFluxTest
import org.springframework.test.web.reactive.server.WebTestClient
import java.nio.charset.StandardCharsets

//@SpringBootTest
@WebFluxTest
class TestHttpCall {
    @Autowired
    lateinit var client: WebTestClient

    @Autowired
    lateinit var objectMapper: ObjectMapper

    @Test
    fun testPing() {
        client.get()
            .uri("/ping")
            .exchange()
            .expectStatus().is2xxSuccessful
    }

    @Test
    fun testCallParse() {
        client.post()
            .uri("/webhook")
            .header("Content-Type", "application/json")
            .bodyValue(req)
            .exchange()
            .expectStatus().is2xxSuccessful
            .expectBody().consumeWith {
               println(String(it.responseBody, StandardCharsets.UTF_8))
            }
    }

    @Test
    fun testCallParse2() {
        client.post()
            .uri("/webhook")
            .header("Content-Type", "application/json")
            .bodyValue(req2)
            .exchange()
            .expectStatus().is2xxSuccessful
            .expectBody().consumeWith {
                println(String(it.responseBody, StandardCharsets.UTF_8))
            }
    }

    val req = """
{
"meta": {
"locale": "ru-RU",
"timezone": "UTC",
"client_id": "ru.yandex.searchplugin/7.16 (none none; android 4.4.2)",
"interfaces": {
  "screen": {},
  "payments": {},
  "account_linking": {}
}
},
"session": {
"message_id": 0,
"session_id": "12ae9199-92d0-4d10-9356-e0582f33146e",
"skill_id": "2ec0f6f8-52e2-4978-981b-c664a9e0842e",
"user": {
  "user_id": "CD4A9DB2DB0273FEE7E5AF576457CDDF7433B871297763C37F811A181E3D5A28"
},
"application": {
  "application_id": "E5914E2407F03AEBF2B0F856A7D1B1F83D5D44CCE543E22CB61667498B1DD28F"
},
"user_id": "E5914E2407F03AEBF2B0F856A7D1B1F83D5D44CCE543E22CB61667498B1DD28F",
"new": true
},
"request": {
"command": "",
"original_utterance": "",
"nlu": {
  "tokens": [],
  "entities": [],
  "intents": {}
},
"markup": {
  "dangerous_context": false
},
"type": "SimpleUtterance"
},
"state": {
"session": {},
"user": {},
"application": {}
},
"version": "1.0"
}
    """.trimIndent().trim()

    val req2 = """
        {
          "meta": {
            "locale": "ru-RU",
            "timezone": "UTC",
            "client_id": "ru.yandex.searchplugin/7.16 (none none; android 4.4.2)",
            "interfaces": {
              "screen": {},
              "payments": {},
              "account_linking": {}
            }
          },
          "session": {
            "message_id": 1,
            "session_id": "e461a575-db59-4c42-b976-b8c0f23083b8",
            "skill_id": "2ec0f6f8-52e2-4978-981b-c664a9e0842e",
            "user": {
              "user_id": "CD4A9DB2DB0273FEE7E5AF576457CDDF7433B871297763C37F811A181E3D5A28"
            },
            "application": {
              "application_id": "E5914E2407F03AEBF2B0F856A7D1B1F83D5D44CCE543E22CB61667498B1DD28F"
            },
            "user_id": "E5914E2407F03AEBF2B0F856A7D1B1F83D5D44CCE543E22CB61667498B1DD28F",
            "new": false
          },
          "request": {
            "command": "давай",
            "original_utterance": "давай",
            "nlu": {
              "tokens": [
                "давай"
              ],
              "entities": [],
              "intents": {
                "YANDEX.CONFIRM": {
                  "slots": {}
                }
              }
            },
            "markup": {
              "dangerous_context": false
            },
            "type": "SimpleUtterance"
          },
          "state": {
            "session": {
              "shown": [],
              "@type": "OfferMoreState"
            },
            "user": {},
            "application": {}
          },
          "version": "1.0"
        }
    """.trimIndent().trim()
}
