package ru.yandex.alice.memento.controller

import com.fasterxml.jackson.databind.ObjectMapper
import com.google.protobuf.Any
import com.google.protobuf.GeneratedMessageV3
import com.google.protobuf.Message
import com.google.protobuf.Struct
import com.google.protobuf.util.JsonFormat
import com.yandex.ydb.ValueProtos
import org.springframework.beans.factory.annotation.Value
import org.springframework.boot.autoconfigure.http.HttpMessageConverters
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.http.MediaType
import org.springframework.http.converter.ByteArrayHttpMessageConverter
import org.springframework.http.converter.HttpMessageConverter
import org.springframework.http.converter.ResourceHttpMessageConverter
import org.springframework.http.converter.ResourceRegionHttpMessageConverter
import org.springframework.http.converter.StringHttpMessageConverter
import org.springframework.http.converter.json.MappingJackson2HttpMessageConverter
import org.springframework.web.servlet.config.annotation.InterceptorRegistry
import org.springframework.web.servlet.config.annotation.WebMvcConfigurer
import ru.yandex.alice.memento.proto.MementoApiProto
import ru.yandex.alice.memento.proto.UserConfigsProto
import ru.yandex.alice.memento.scanner.KeyMappingScanner
import ru.yandex.alice.paskills.common.logging.protoseq.Setrace
import ru.yandex.alice.paskills.common.logging.protoseq.SetraceInterceptor
import ru.yandex.alice.paskills.common.proto.utils.ProtoWarmupper
import ru.yandex.alice.paskills.common.tvm.spring.handler.TvmAuthorizationInterceptor
import ru.yandex.library.svnversion.VcsVersion
import ru.yandex.passport.tvmauth.TvmClient
import java.lang.reflect.Type

@Configuration
internal open class ControllerConfiguration(
    private val tvmClient: TvmClient,
    private val setrace: Setrace,
    scanner: KeyMappingScanner
) : WebMvcConfigurer {

    private val registry = JsonFormat.TypeRegistry.newBuilder()
        .add(UserConfigsProto.TNewsConfig.getDescriptor())
        .build()

    @Value("\${sec.developerTrustedToken:}")
    private val developerTrustedToken: String? = null

    @Value("\${sec.allowXUid:false}")
    private val allowXUid = false

    init {
        val userConfigs = MementoApiProto.EConfigKey.values()
            .mapNotNull {
                scanner.getClassForKey(it)
                    ?.let { clazz -> clazz.getMethod("getDefaultInstance").invoke(null) as GeneratedMessageV3 }
            }
            .toSet()

        val deviceConfigs = MementoApiProto.EDeviceConfigKey.values()
            .mapNotNull {
                scanner.getClassForKey(it)
                    ?.let { clazz -> clazz.getMethod("getDefaultInstance").invoke(null) as GeneratedMessageV3 }
            }
            .toSet()

        ProtoWarmupper.warmupProtoDescriptor(
            setOf(
                MementoApiProto.TReqGetAllObjects.getDefaultInstance(),
                MementoApiProto.TRespGetAllObjects.getDefaultInstance(),
                MementoApiProto.TReqChangeUserObjects.getDefaultInstance(),
                MementoApiProto.TRespChangeUserObjects.getDefaultInstance(),
                MementoApiProto.TReqGetUserObjects.getDefaultInstance(),
                MementoApiProto.TRespGetUserObjects.getDefaultInstance(),
                MementoApiProto.TClearUserData.getDefaultInstance(),
                ValueProtos.ResultSet.getDefaultInstance(),
                Any.getDefaultInstance(),
                Struct.getDefaultInstance(),
            ) + userConfigs + deviceConfigs
        )
    }

    @Bean
    open fun tvmAuthorizationInterceptor(): TvmAuthorizationInterceptor {
        return TvmAuthorizationInterceptor(tvmClient, emptyMap(), developerTrustedToken, allowXUid)
    }

    @Bean
    open fun versionInfo(): MementoApiProto.VersionInfo {
        val vcsVersion = VcsVersion(ControllerConfiguration::class.java)
        return MementoApiProto.VersionInfo.newBuilder()
            .setSvnVersion(vcsVersion.programHash)
            .setSvnRevision(vcsVersion.programSvnRevision)
            .setArcadiaPatchNumber(vcsVersion.arcadiaPatchNumer)
            .setBranch(vcsVersion.branch)
            .setTag(vcsVersion.tag)
            .build()
    }

    override fun addInterceptors(registry: InterceptorRegistry) {
        registry.addInterceptor(SetraceInterceptor(setrace))
        registry.addInterceptor(tvmAuthorizationInterceptor())
    }

    override fun configureMessageConverters(converters: MutableList<HttpMessageConverter<*>?>) {
        converters.add(protobufHttpMessageConverter())
    }

    @Bean
    open fun protobufJsonParser(): JsonFormat.Parser {
        return JsonFormat.parser().usingTypeRegistry(registry)
    }

    @Bean
    open fun protobufJsonPrinter(): JsonFormat.Printer {
        return JsonFormat.printer().usingTypeRegistry(registry)
    }

    @Bean
    open fun protobufHttpMessageConverter(): CustomProtobufHttpMessageConverter {
        return CustomProtobufHttpMessageConverter(protobufJsonParser(), protobufJsonPrinter())
    }

    @Bean
    open fun jsonHttpMessageConverter(objectMapper: ObjectMapper): MappingJackson2HttpMessageConverter {
        val converter: MappingJackson2HttpMessageConverter =
            object : MappingJackson2HttpMessageConverter(objectMapper) {
                // disable jackson as a protobuf classes
                override fun canRead(type: Type, contextClass: Class<*>?, mediaType: MediaType?): Boolean {
                    return if (type is Class<*> && Message::class.java.isAssignableFrom(type)) {
                        false
                    } else super.canRead(type, contextClass, mediaType)
                }

                override fun supports(clazz: Class<*>): Boolean {
                    return !Message::class.java.isAssignableFrom(clazz) && super.supports(clazz)
                }

                override fun canWrite(clazz: Class<*>, mediaType: MediaType?): Boolean {
                    return !Message::class.java.isAssignableFrom(clazz) && super.canWrite(clazz, mediaType)
                }
            }
        return converter
    }

    /*leave only defined in app converters ignoring default spring converters*/
    @Bean
    open fun messageConverters(
        stringHttpConverter: StringHttpMessageConverter,
        jsonConverter: MappingJackson2HttpMessageConverter,
        protobufConverter: CustomProtobufHttpMessageConverter
    ): HttpMessageConverters {
        val list = ArrayList<HttpMessageConverter<*>>()
        list.add(protobufConverter)
        list.add(ByteArrayHttpMessageConverter())
        list.add(stringHttpConverter)
        list.add(ResourceHttpMessageConverter())
        list.add(ResourceRegionHttpMessageConverter())
        list.add(jsonConverter)
        return HttpMessageConverters(false, list)
    }
}
