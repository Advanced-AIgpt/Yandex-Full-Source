package ru.yandex.alice.kronstadt.core.convert

import com.google.protobuf.Message
import ru.yandex.alice.kronstadt.core.convert.request.FromProtoConverter
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.megamind.protos.scenarios.RequestProto

interface StateConverter<ScenarioState> : ToProtoConverter<ScenarioState, Message>,
    FromProtoConverter<RequestProto.TScenarioBaseRequest, ScenarioState?>

