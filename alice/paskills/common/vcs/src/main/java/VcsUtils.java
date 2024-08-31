package ru.yandex.alice.paskills.common.vcs;

import java.io.IOException;
import java.net.URL;
import java.util.Enumeration;
import java.util.Objects;
import java.util.jar.Attributes;
import java.util.jar.Manifest;

public class VcsUtils {

    private final String version;
    public static final VcsUtils INSTANCE = new VcsUtils();

    private VcsUtils() {
        String ver = null;
        try {
            Enumeration<URL> resources = getClass().getClassLoader()
                    .getResources("META-INF/MANIFEST.MF");
            while (resources.hasMoreElements()) {
                Manifest manifest = new Manifest(resources.nextElement().openStream());

                Attributes attributes = manifest.getMainAttributes();
                String value = attributes.getValue("Arcadia-Source-Last-Change");
                if (value != null && !value.isEmpty()) {
                    ver = value;
                    break;
                }

            }
        } catch (IOException ignored) {

        }

        this.version = Objects.requireNonNullElse(ver, "");
    }

    public String getVersion() {
        return version;
    }
}
