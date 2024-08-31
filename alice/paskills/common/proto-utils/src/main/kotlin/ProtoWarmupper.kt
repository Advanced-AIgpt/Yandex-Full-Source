package ru.yandex.alice.paskills.common.proto.utils

import com.fasterxml.jackson.databind.ObjectMapper
import com.google.protobuf.Descriptors
import com.google.protobuf.GeneratedMessageV3
import org.apache.logging.log4j.LogManager

object ProtoWarmupper {
    private val logger = LogManager.getLogger()

    fun warmup(objectMapper: ObjectMapper, protoMessages: Set<GeneratedMessageV3>) {
        warmupJackson(objectMapper, protoMessages.map { it::class.java }.toSet())
        warmupProtoDescriptor(protoMessages)
    }

    fun warmupJackson(objectMapper: ObjectMapper, classes: Set<Class<*>>) {
        logger.info("Starting jackson warmup for proto")
        for (clazz in classes) {
            objectMapper.canSerialize(clazz)
            objectMapper.canDeserialize(objectMapper.constructType(clazz))
        }
        logger.info("Finished jackson warmup for proto")
    }

    fun warmupProtoDescriptor(protoMessages: Set<GeneratedMessageV3>) {
        val map = hashSetOf<Descriptors.Descriptor>()
        logger.info("Starting proto warmup")
        for (msg in protoMessages) {
            scan(map, msg)
        }
        logger.info("Finished proto warmup")
    }

    private fun scan(map: MutableSet<Descriptors.Descriptor>, msg: GeneratedMessageV3) {
        if (msg.descriptorForType !in map) {
            map.add(msg.descriptorForType)
            val ignored = msg.toBuilder()
            for (field in msg.descriptorForType.fields) {
                if (field.javaType == Descriptors.FieldDescriptor.JavaType.MESSAGE
                    && field.messageType !in map
                ) {
                    val obj = msg.getField(field) as? GeneratedMessageV3
                    if (obj != null) {
                        scan(map, obj)
                    }
                }
            }
            for (oneof in msg.descriptorForType.realOneofs) {
                for (field in oneof.fields) {
                    if (field.javaType == Descriptors.FieldDescriptor.JavaType.MESSAGE
                        && field.messageType !in map
                    ) {
                        val obj = msg.getField(field) as? GeneratedMessageV3
                        if (obj != null) {
                            scan(map, obj)
                        }
                    }
                }
            }
        }
    }
}
