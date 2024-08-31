package ru.yandex.alice.paskills.common.apphost.spring;

import javax.annotation.Nullable;

import com.google.protobuf.Struct;

public record DataSourceApphostContainer(
        @Nullable @ApphostKey("datasource_USER_LOCATION") Struct userLocation,
        @Nullable @ApphostKey("datasource_BEGEMOT_EXTERNAL_MARKUP") Struct begemotExternalMarkup,
        @Nullable @ApphostKey("datasource_BLACK_BOX") Struct userInfo
) implements ApphostKeysContainer {
}
