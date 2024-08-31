package ru.yandex.alice.kronstadt.server.http.middleware;

import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.List;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.protobuf.Descriptors;
import com.google.protobuf.Message;
import com.google.protobuf.util.JsonFormat;
import org.springframework.boot.autoconfigure.condition.ConditionalOnMissingBean;
import org.springframework.boot.autoconfigure.http.HttpMessageConverters;
import org.springframework.context.annotation.Bean;
import org.springframework.http.MediaType;
import org.springframework.http.converter.ByteArrayHttpMessageConverter;
import org.springframework.http.converter.HttpMessageConverter;
import org.springframework.http.converter.ResourceHttpMessageConverter;
import org.springframework.http.converter.ResourceRegionHttpMessageConverter;
import org.springframework.http.converter.StringHttpMessageConverter;
import org.springframework.http.converter.json.MappingJackson2HttpMessageConverter;
import org.springframework.lang.Nullable;
import org.springframework.stereotype.Component;
import org.springframework.web.servlet.config.annotation.InterceptorRegistry;
import org.springframework.web.servlet.config.annotation.WebMvcConfigurer;

import ru.yandex.alice.kronstadt.proto.ApplyArgsProto;
import ru.yandex.alice.kronstadt.server.http.CustomProtobufHttpMessageConverter;
import ru.yandex.alice.paskills.common.logging.protoseq.Setrace;
import ru.yandex.alice.paskills.common.logging.protoseq.SetraceInterceptor;
import ru.yandex.alice.paskills.common.tvm.spring.handler.TvmAuthorizationInterceptor;

@Component
class ControllerConfigurer implements WebMvcConfigurer {
    private final RequestContextInterceptor requestContextInterceptor;
    private final LoggingInterceptor loggingInterceptor;
    private final SolomonInterceptor solomonInterceptor;
    private final TvmAuthorizationInterceptor tvmAuthorizationInterceptor;
    private final SetraceInterceptor setraceInterceptor;

    ControllerConfigurer(
            RequestContextInterceptor requestContextInterceptor,
            LoggingInterceptor loggingInterceptor,
            SolomonInterceptor solomonInterceptor,
            TvmAuthorizationInterceptor tvmAuthorizationInterceptor,
            Setrace setrace) {
        this.requestContextInterceptor = requestContextInterceptor;
        this.loggingInterceptor = loggingInterceptor;
        this.solomonInterceptor = solomonInterceptor;
        this.tvmAuthorizationInterceptor = tvmAuthorizationInterceptor;
        this.setraceInterceptor = new SetraceInterceptor(setrace);
    }

    @Bean
    @ConditionalOnMissingBean(JsonFormat.TypeRegistry.class)
    public JsonFormat.TypeRegistry protobufTypeRegistry(List<Descriptors.Descriptor> registryDescriptors) {
        return JsonFormat.TypeRegistry.newBuilder()
                .add(registryDescriptors)
                .add(ApplyArgsProto.TSceneArguments.getDescriptor())
                .build();
    }

    @Bean
    @ConditionalOnMissingBean(JsonFormat.Parser.class)
    JsonFormat.Parser protobufJsonParser(JsonFormat.TypeRegistry registry) {
        return JsonFormat.parser().usingTypeRegistry(registry);
    }

    @Bean
    @ConditionalOnMissingBean(JsonFormat.Printer.class)
    JsonFormat.Printer protobufJsonPrinter(JsonFormat.TypeRegistry registry) {
        return JsonFormat.printer().usingTypeRegistry(registry);
    }

    // Proto to json converter
    @Bean
    CustomProtobufHttpMessageConverter protobufHttpMessageConverter(JsonFormat.Printer printer,
                                                                    JsonFormat.Parser parser) {
        return new CustomProtobufHttpMessageConverter(parser, printer);
    }

    @Bean
    MappingJackson2HttpMessageConverter jsonHttpMessageConverter(ObjectMapper objectMapper) {
        var converter = new MappingJackson2HttpMessageConverter(objectMapper) {


            // disable jackson as a protobuf classes
            @Override
            public boolean canRead(Type type, @Nullable Class<?> contextClass, @Nullable MediaType mediaType) {
                if (type instanceof Class<?> && Message.class.isAssignableFrom((Class<?>) type)) {
                    return false;
                }
                return super.canRead(type, contextClass, mediaType);
            }

            @Override
            protected boolean supports(Class<?> clazz) {
                return !Message.class.isAssignableFrom(clazz) && super.supports(clazz);
            }

            @Override
            public boolean canWrite(Class<?> clazz, @Nullable MediaType mediaType) {
                return !Message.class.isAssignableFrom(clazz) && super.canWrite(clazz, mediaType);
            }
        };
        return converter;
    }

    /*leave only defined in app converters ignoring default spring converters*/
    @Bean
    public HttpMessageConverters messageConverters(
            StringHttpMessageConverter stringHttpConverter,
            MappingJackson2HttpMessageConverter jsonConverter,
            CustomProtobufHttpMessageConverter protobufConverter
    ) {
        ArrayList<HttpMessageConverter<?>> list = new ArrayList<>();
        list.add(protobufConverter);
        list.add(new ByteArrayHttpMessageConverter());
        list.add(stringHttpConverter);
        list.add(new ResourceHttpMessageConverter());
        list.add(new ResourceRegionHttpMessageConverter());
        list.add(jsonConverter);

        return new HttpMessageConverters(false, list);
    }

    @Override
    public void addInterceptors(InterceptorRegistry registry) {
        registry.addInterceptor(solomonInterceptor);
        registry.addInterceptor(setraceInterceptor);
        registry.addInterceptor(tvmAuthorizationInterceptor);
        registry.addInterceptor(requestContextInterceptor);
        registry.addInterceptor(loggingInterceptor);
    }
}
