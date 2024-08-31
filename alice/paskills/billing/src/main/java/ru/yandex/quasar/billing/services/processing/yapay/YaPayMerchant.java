package ru.yandex.quasar.billing.services.processing.yapay;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.time.Instant;
import java.time.LocalDate;
import java.util.Map;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

@Data
@Builder
public class YaPayMerchant {
    private final Map<PersonType, Person> persons;

    @JsonFormat(shape = JsonFormat.Shape.STRING)
    private final Instant created;
    @JsonFormat(shape = JsonFormat.Shape.STRING)
    private final Instant updated;
    private final Moderation moderation;
    private final Organization organization;
    @JsonProperty("parent_uid")
    private final BigDecimal parentUid;
    private final Map<AddressType, Address> addresses;
    private final BigInteger uid;
    private final BigInteger revision;
    private final String status;
    private final Bank bank;
    private final String name;
    private final Billing billing;


    public enum AddressType {
        legal,
        post
    }

    public enum PersonType {
        ceo,
        contact,
        signer
    }

    @Data
    @Builder
    public static class Person {
        private final String surname;
        @JsonProperty("birthDate")
        private final LocalDate birthDate;
        private final String patronymic;
        private final String phone;
        private final String email;
        private final String name;
    }

    @Data
    public static class Moderation {
        //private final List<BigDecimal> reasons;
        @JsonProperty("hasOngoing")
        private final boolean hasOngoing;
        private final boolean approved;
    }

    @Data
    @Builder
    public static class Address {
        private final String street;
        private final String home;
        private final String city;
        private final String country;
        private final String zip;
    }

    @Data
    @Builder
    public static class Bank {
        @JsonProperty("correspondentAccount")
        private final String correspondentAccount;
        private final String name;
        private final String bik;
        private final String account;
    }

    @Data
    @Builder
    public static class Billing {
        @JsonProperty("trust_partner_id")
        private final String trustPartnerId;
        @JsonProperty("trust_submerchant_id")
        private final String trustSubmerchantId;
        @JsonProperty("contract_id")
        private final String contractId;
        @JsonProperty("client_id")
        private final String clientId;
        @JsonProperty("person_id")
        private final String personId;
    }

    @Data
    static class Wrapper {
        private final YaPayMerchant data;
        private final int code;
        private final String status;
    }

}

