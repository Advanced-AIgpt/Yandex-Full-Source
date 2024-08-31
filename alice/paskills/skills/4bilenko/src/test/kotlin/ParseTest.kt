package ru.yandex.alice.paskills.skills.bilenko

import com.fasterxml.jackson.databind.node.ObjectNode
import org.junit.jupiter.api.Test
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.boot.test.autoconfigure.json.JsonTest
import org.springframework.boot.test.json.JacksonTester
import ru.yandex.alice.paskills.skills.bilenko.model.WebhookRequest

@JsonTest
class ParseTest {
    @Autowired
    lateinit var tester: JacksonTester<WebhookRequest<SessionState, ObjectNode>>

    @Autowired
    lateinit var stateTester: JacksonTester<SessionState>

    @Test
    internal fun testParse() {
        val o = tester.parse(req).`object`
    }    @Test
    internal fun testParse2() {
        val o = tester.parse(req2).`object`
    }

    @Test
    internal fun testSerialize() {
        val w = stateTester.write(OfferMoreState(listOf()))
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
            "session_id": "8bce0ee7-52f3-4ae5-a710-523d0b1d6914",
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
            "command": "1",
            "original_utterance": "1",
            "nlu": {
              "tokens": [
                "1"
              ],
              "entities": [
                {
                  "type": "YANDEX.NUMBER",
                  "tokens": {
                    "start": 0,
                    "end": 1
                  },
                  "value": 1
                }
              ],
              "intents": {}
            },
            "markup": {
              "dangerous_context": false
            },
            "type": "SimpleUtterance"
          },
          "state": {
            "session": {
              "shown": [],
              "@class": "ru.yandex.alice.paskills.skills.bilenko.SessionState${"$"}OfferMoreState"
            },
            "user": {},
            "application": {}
          },
          "version": "1.0"
        }
    """.trimIndent().trim()
}
