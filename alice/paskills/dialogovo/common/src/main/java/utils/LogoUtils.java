package ru.yandex.alice.paskill.dialogovo.utils;

import java.util.Objects;

import ru.yandex.alice.paskill.dialogovo.domain.ImageAlias;

public class LogoUtils {
    private LogoUtils() {
        throw new UnsupportedOperationException();
    }

    /**
     * @param imageUrl full image url (https://avatars.mds.yandex.net/get-dialogs/758954/1a309e8e7d6781214dc5/orig)
     * @return logoUrl
     */
    public static String makeLogo(String imageUrl, ImageAlias size) {
        Objects.requireNonNull(imageUrl, "imageUrl required");

        var lastIndex = imageUrl.lastIndexOf("/");
        if (lastIndex == -1) {
            return imageUrl;
        }

        return imageUrl.substring(0, lastIndex) + "/" + size.getCode();
    }
}
