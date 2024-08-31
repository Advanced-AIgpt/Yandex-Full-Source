package ru.yandex.alice.social.sharing.document

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.registerKotlinModule

internal val objectMapper = ObjectMapper().registerKotlinModule()
