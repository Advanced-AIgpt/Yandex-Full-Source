package ru.yandex.alice.paskills.my_alice.blackbox;

import java.io.IOException;
import java.math.BigInteger;
import java.util.List;
import java.util.Map;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.GsonTester;

import static org.junit.jupiter.api.Assertions.assertEquals;

@JsonTest
class SessionIdTest {

    @Autowired
    private GsonTester<SessionId.RawResponse> gsonTester;

    @Test
    void yandexoidRaw() throws IOException {
        SessionId.RawResponse expected = SessionId.RawResponse.builder()
                .age(87925)
                .expiresIn(7688075)
                .ttl(5)
                .error("OK")
                .status(new SessionId.RawResponse.AuthStatus("VALID", 0))
                .uid(new SessionId.RawResponse.Uid(BigInteger.valueOf(14824612), false, null))
                .login("ra6fpb")
                .aliases(Map.of("13", "jock"))
                .karma(new SessionId.RawResponse.Karma(0, null))
                .karmaStatus(new SessionId.RawResponse.KarmaStatus(6000))
                .displayName(new SessionId.RawResponse.DisplayName(
                        "Евгений",
                        null,
                        new User.Avatar("15298/enc-92d865871", false)
                ))
                .dbFields(Map.of(
                        "subscription.suid.2", "227379366",
                        "subscription.suid.668", "1",
                        "subscription.suid.669", "1",
                        "userinfo.firstname.uid", "Евгений",
                        "userinfo.lastname.uid", "Королев"
                ))
                .attributes(Map.of(
                        "1008", "ra6fpb",
                        "1015", "1",
                        "1016", "YA_PLUS"
                ))
                .addressList(List.of(
                        new SessionId.RawResponse.Address("ra6fpb@yandex.ru", true, true, true, "2005-11-10 22:00:21")
                ))
                .phones(List.of(
                        new SessionId.RawResponse.Phone("15075359", Map.of(
                                "102", "+79250000000",
                                "107", "1",
                                "108", "1"
                        ))
                ))
                .userTicket("3:user:COtoEJar")
                .build();

        gsonTester.parse("{\"age\":87925,\"expires_in\":7688075,\"ttl\":\"5\",\"error\":\"OK\"," +
                "\"status\":{\"value\":\"VALID\",\"id\":0},\"uid\":{\"value\":\"14824612\",\"lite\":false," +
                "\"hosted\":false},\"login\":\"ra6fpb\",\"have_password\":true,\"have_hint\":true," +
                "\"aliases\":{\"13\":\"jock\"},\"karma\":{\"value\":0},\"karma_status\":{\"value\":6000}," +
                "\"regname\":\"ra6fpb\",\"display_name\":{\"name\":\"\\u0415\\u0432\\u0433\\u0435\\u043D\\u0438" +
                "\\u0439\",\"avatar\":{\"default\":\"15298\\/enc-92d865871\"," +
                "\"empty\":false}},\"dbfields\":{\"subscription.suid.2\":\"227379366\",\"subscription.suid" +
                ".668\":\"1\",\"subscription.suid.669\":\"1\",\"userinfo.firstname" +
                ".uid\":\"\\u0415\\u0432\\u0433\\u0435\\u043D\\u0438\\u0439\",\"userinfo.lastname" +
                ".uid\":\"\\u041A\\u043E\\u0440\\u043E\\u043B\\u0435\\u0432\"},\"attributes\":{\"1008\":\"ra6fpb\"," +
                "\"1015\":\"1\",\"1016\":\"YA_PLUS\"},\"address-list\":[{\"address\":\"ra6fpb@yandex.ru\"," +
                "\"validated\":true,\"default\":true,\"rpop\":false,\"silent\":false,\"unsafe\":false," +
                "\"native\":true,\"born-date\":\"2005-11-10 22:00:21\"}],\"phones\":[{\"id\":\"15075359\"," +
                "\"attributes\":{\"102\":\"+79250000000\",\"107\":\"1\",\"108\":\"1\"}}]," +
                "\"auth\":{\"password_verification_age\":7522085,\"have_password\":true,\"secure\":true," +
                "\"partner_pdd_token\":false},\"connection_id\":\"s:1574677870852:oB-j9M0jhOgMBAAAuAYCKg:1c\"," +
                "\"user_ticket\":\"3:user:COtoEJar\"}")
                .assertThat().isEqualToComparingFieldByField(expected);
    }

    @Test
    void yandexoid() {
        SessionId.RawResponse raw = SessionId.RawResponse.builder()
                .age(87925)
                .expiresIn(7688075)
                .ttl(5)
                .error("OK")
                .status(new SessionId.RawResponse.AuthStatus("VALID", 0))
                .uid(new SessionId.RawResponse.Uid(BigInteger.valueOf(14824612), false, null))
                .login("ra6fpb")
                .aliases(Map.of("13", "jock"))
                .karma(new SessionId.RawResponse.Karma(0, null))
                .karmaStatus(new SessionId.RawResponse.KarmaStatus(6000))
                .displayName(new SessionId.RawResponse.DisplayName(
                        "Евгений",
                        null,
                        new User.Avatar("15298/enc-92d865871", false)
                ))
                .dbFields(Map.of(
                        "subscription.suid.2", "227379366",
                        "subscription.suid.668", "1",
                        "subscription.suid.669", "1",
                        "userinfo.firstname.uid", "Евгений",
                        "userinfo.lastname.uid", "Королев"
                ))
                .attributes(Map.of(
                        "1008", "ra6fpb",
                        "1015", "1",
                        "1016", "YA_PLUS"
                ))
                .addressList(List.of(
                        new SessionId.RawResponse.Address("ra6fpb@yandex.ru", true, true, true, "2005" +
                                "-11-10 22:00:21")
                ))
                .phones(List.of(
                        new SessionId.RawResponse.Phone("15075359", Map.of(
                                "102", "+79250000000",
                                "107", "1",
                                "108", "1"
                        ))
                ))
                .userTicket("3:user:COtoEJar")
                .build();

        SessionId.Response expected = new SessionId.Response(
                SessionId.Response.Status.VALID,
                null,
                new User(
                        BigInteger.valueOf(14824612),
                        "ra6fpb",
                        "Евгений",
                        new User.Avatar("15298/enc-92d865871", false),
                        new User.Attributes(
                                true,
                                true,
                                "jock"
                        )
                ),
                "3:user:COtoEJar",
                raw
        );

        assertEquals(SessionId.Response.fromRaw(raw), expected);
    }

    @Test
    void pddRaw() throws IOException {
        SessionId.RawResponse expected = SessionId.RawResponse.builder()
                .age(42)
                .expiresIn(7775958)
                .ttl(5)
                .error("OK")
                .status(new SessionId.RawResponse.AuthStatus("VALID", 0))
                .uid(new SessionId.RawResponse.Uid(
                        BigInteger.valueOf(1130000043768933L),
                        true,
                        "alice-b2b-priemka.yaconnect.com"
                ))
                .login("yndx.bus.012@alice-b2b-priemka.yaconnect.com")
                .aliases(Map.of())
                .karma(new SessionId.RawResponse.Karma(0, null))
                .karmaStatus(new SessionId.RawResponse.KarmaStatus(0))
                .displayName(new SessionId.RawResponse.DisplayName(
                        "yndx.bus.012@alice-b2b-priemka.yaconnect.com",
                        null,
                        new User.Avatar("0/0-0", true)
                ))
                .dbFields(Map.of(
                        "subscription.suid.2", "1130000060971445",
                        "subscription.suid.668", "",
                        "subscription.suid.669", "",
                        "userinfo.firstname.uid", "Технарь",
                        "userinfo.lastname.uid", "Человек"
                ))
                .attributes(Map.of(
                        "1008", "yndx.bus.012@alice-b2b-priemka.yaconnect.com"
                ))
                .addressList(List.of(
                        new SessionId.RawResponse.Address(
                                "yndx.bus.012@alice-b2b-priemka.yaconnect.com",
                                true,
                                true,
                                true,
                                "2020-03-13 12:15:07"
                        )
                ))
                .phones(List.of())
                .userTicket("3:user:COtoELnB6PcFGigKCQjl")
                .build();

        gsonTester.parse("{\"age\":42,\"expires_in\":7775958,\"ttl\":\"5\",\"error\":\"OK\"," +
                "\"status\":{\"value\":\"VALID\",\"id\":0},\"uid\":{\"value\":\"1130000043768933\",\"lite\":false," +
                "\"hosted\":true,\"domid\":\"4159661\",\"domain\":\"alice-b2b-priemka.yaconnect.com\",\"mx\":\"0\"," +
                "\"domain_ena\":\"1\",\"catch_all\":false},\"login\":\"yndx.bus.012@alice-b2b-priemka.yaconnect" +
                ".com\",\"have_password\":true,\"have_hint\":false,\"aliases\":{},\"karma\":{\"value\":0}," +
                "\"karma_status\":{\"value\":0},\"regname\":\"yndx.bus.012@alice-b2b-priemka.yaconnect.com\"," +
                "\"display_name\":{\"name\":\"yndx.bus.012@alice-b2b-priemka.yaconnect.com\"," +
                "\"avatar\":{\"default\":\"0\\/0-0\",\"empty\":true}},\"dbfields\":{\"subscription.suid" +
                ".2\":\"1130000060971445\",\"subscription.suid.668\":\"\",\"subscription.suid.669\":\"\",\"userinfo" +
                ".firstname.uid\":\"\\u0422\\u0435\\u0445\\u043D\\u0430\\u0440\\u044C\",\"userinfo.lastname" +
                ".uid\":\"\\u0427\\u0435\\u043B\\u043E\\u0432\\u0435\\u043A\"},\"attributes\":{\"1008\":\"yndx.bus" +
                ".012@alice-b2b-priemka.yaconnect.com\"},\"address-list\":[{\"address\":\"yndx.bus" +
                ".012@alice-b2b-priemka.yaconnect.com\",\"validated\":true,\"default\":true,\"rpop\":false," +
                "\"silent\":false,\"unsafe\":false,\"native\":true,\"born-date\":\"2020-03-13 12:15:07\"}]," +
                "\"phones\":[],\"auth\":{\"password_verification_age\":9356722,\"have_password\":true," +
                "\"secure\":true,\"partner_pdd_token\":false}," +
                "\"connection_id\":\"s:1574677870852:oB-j9M0jhOgMBAAAuAYCKg:1c\"," +
                "\"user_ticket\":\"3:user:COtoELnB6PcFGigKCQjl\"}\n")
                .assertThat().isEqualToComparingFieldByField(expected);
    }

    @Test
    void pdd() {
        SessionId.RawResponse raw = SessionId.RawResponse.builder()
                .age(42)
                .expiresIn(7775958)
                .ttl(5)
                .error("OK")
                .status(new SessionId.RawResponse.AuthStatus("VALID", 0))
                .uid(new SessionId.RawResponse.Uid(
                        BigInteger.valueOf(1130000043768933L),
                        true,
                        "alice-b2b-priemka.yaconnect.com"
                ))
                .login("yndx.bus.012@alice-b2b-priemka.yaconnect.com")
                .aliases(Map.of())
                .karma(new SessionId.RawResponse.Karma(0, null))
                .karmaStatus(new SessionId.RawResponse.KarmaStatus(0))
                .displayName(new SessionId.RawResponse.DisplayName(
                        "yndx.bus.012@alice-b2b-priemka.yaconnect.com",
                        null,
                        new User.Avatar("0/0-0", true)
                ))
                .dbFields(Map.of(
                        "subscription.suid.2", "1130000060971445",
                        "subscription.suid.668", "",
                        "subscription.suid.669", "",
                        "userinfo.firstname.uid", "Технарь",
                        "userinfo.lastname.uid", "Человек"
                ))
                .attributes(Map.of(
                        "1008", "yndx.bus.012@alice-b2b-priemka.yaconnect.com"
                ))
                .addressList(List.of(
                        new SessionId.RawResponse.Address(
                                "yndx.bus.012@alice-b2b-priemka.yaconnect.com",
                                true,
                                true,
                                true,
                                "2020-03-13 12:15:07"
                        )
                ))
                .phones(List.of())
                .userTicket("3:user:COtoELnB6PcFGigKCQjl")
                .build();

        SessionId.Response expected = new SessionId.Response(
                SessionId.Response.Status.VALID,
                null,
                new User(
                        BigInteger.valueOf(1130000043768933L),
                        "yndx.bus.012@alice-b2b-priemka.yaconnect.com",
                        "yndx.bus.012@alice-b2b-priemka.yaconnect.com",
                        new User.Avatar("0/0-0", true),
                        new User.Attributes(
                                false,
                                false,
                                null
                        )
                ),
                "3:user:COtoELnB6PcFGigKCQjl",
                raw
        );

        assertEquals(SessionId.Response.fromRaw(raw), expected);
    }
}
