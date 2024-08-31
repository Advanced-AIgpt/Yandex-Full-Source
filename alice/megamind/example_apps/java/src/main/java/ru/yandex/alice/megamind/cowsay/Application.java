package ru.yandex.alice.megamind.cowsay;

import com.google.protobuf.util.JsonFormat;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.context.annotation.Bean;

@SpringBootApplication
public class Application {

    // custom HttpMessageConverter for mm protobuf content-type headers
    @Bean
    CustomProtobufHttpMessageConverter protobufHttpMessageConverter() {
        return new CustomProtobufHttpMessageConverter(JsonFormat.parser(), JsonFormat.printer());
    }

    public static void main(String[] args) {
        SpringApplication.run(Application.class, args);
    }

}
