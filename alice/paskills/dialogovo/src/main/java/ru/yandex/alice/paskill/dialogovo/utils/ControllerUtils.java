package ru.yandex.alice.paskill.dialogovo.utils;

import java.net.InetAddress;
import java.net.UnknownHostException;

import javax.servlet.http.HttpServletRequest;

public class ControllerUtils {

    private ControllerUtils() {
        throw new UnsupportedOperationException();
    }

    public static boolean external(HttpServletRequest req) throws UnknownHostException {
        InetAddress addr = InetAddress.getByName(req.getLocalAddr());
        return !addr.isLoopbackAddress();
    }

}
