package ru.yandex.alice.paskill.dialogovo.utils;

import java.util.stream.Stream;

import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;

import ru.yandex.alice.paskill.dialogovo.domain.ImageAlias;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class LogoUtilsTest {
    static Stream<Arguments> arguments() {
        return Stream.of(
                Arguments.of("https://avatars.mds.yandex.net/get-dialogs/758954/1a309e8e7d6781214dc5/orig", "https" +
                        "://avatars.mds.yandex.net/get-dialogs/758954/1a309e8e7d6781214dc5/orig", ImageAlias.ORIG),
                Arguments.of("https://avatars.mds.yandex.net/get-dialogs/758954/1a309e8e7d6781214dc5/mobile-logo-x2",
                        "https://avatars.mds.yandex.net/get-dialogs/758954/1a309e8e7d6781214dc5/orig",
                        ImageAlias.MOBILE_LOGO_X2)
        );
    }

    @ParameterizedTest
    @MethodSource("arguments")
    void makeUrlRetursExpected(String expected, String imageUrl, ImageAlias alias) {
        assertEquals(expected, LogoUtils.makeLogo(imageUrl, alias));
    }
}
