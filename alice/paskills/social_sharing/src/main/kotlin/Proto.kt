package ru.yandex.alice.social.sharing

import com.google.protobuf.Message

interface ProtoSerializable<MESSAGE: Message> {
    fun toProto(): MESSAGE
}

interface ProtoDeserializer<MESSAGE, DATA_CLASS> {
    fun fromProto(proto: MESSAGE): DATA_CLASS
}
