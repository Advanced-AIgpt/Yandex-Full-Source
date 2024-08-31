package ru.yandex.alice.kronstadt.scenarios.tv_channels.indexer

import com.fasterxml.jackson.databind.ObjectMapper
import org.assertj.core.api.Assertions.assertThat
import org.junit.jupiter.api.Test
import org.springframework.boot.test.json.JacksonTester
import org.springframework.core.ResolvableType

internal class MessageTest() {

    @Test
    fun testSerializeModifyMessage() {
        val tester = JacksonTester<ModifyMessage>(
            MessageTest::class.java,
            ResolvableType.forClass(ModifyMessage::class.java),
            ObjectMapper().registerModules(ObjectMapper.findModules())
        )
        val msg = ModifyMessage("asd?q=1")
        msg.addZone("zone1", 123)
        msg.addAttr("a1", "qwe", TypedItemType.PROPERTY, TypedItemType.SEARCH_ATTR_LITERAL)
        msg.addAttr("a1", "qqq", TypedItemType.PROPERTY, TypedItemType.SEARCH_ATTR_LITERAL)

        assertThat(tester.write(msg)).isEqualToJson(
            """{
  "prefix": 1,
  "action": "modify",
  "docs": [
    {
      "url": "asd?q=1",
      "options": {
        "mime_type": "text/plain",
        "charset": "utf8",
        "language": "ru"
      },
      "zone1": {
        "value": 123,
        "type": "#z"
      },
      "a1": [
        {
          "value": "qwe",
          "type": "#pl"
        },
        {
          "value": "qqq",
          "type": "#pl"
        }
      ]
    }
  ]
}"""
        )
    }

    @Test
    fun testSerializeDeleteMessage() {
        val tester = JacksonTester<DeleteMessage>(
            MessageTest::class.java,
            ResolvableType.forClass(DeleteMessage::class.java),
            ObjectMapper().registerModules(ObjectMapper.findModules())
        )
        val msg = DeleteMessage("asd?q=1")


        assertThat(tester.write(msg)).isEqualToJson(
            """{
  "prefix": 1,
  "action": "delete",
  "docs": [
    {
      "url": "asd?q=1"
    }
  ]
}"""
        )
    }
}
