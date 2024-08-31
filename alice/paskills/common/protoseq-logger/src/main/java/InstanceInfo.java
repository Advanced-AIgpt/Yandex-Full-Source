package ru.yandex.alice.paskills.common.logging.protoseq;

import java.net.InetAddress;
import java.net.UnknownHostException;

class InstanceInfo {
    public static final InstanceInfo INSTANCE = new InstanceInfo();

    private final long pid = ProcessHandle.current().pid();
    private final String hostname;

    InstanceInfo() {
        String osHostname;
        try {
            osHostname = InetAddress.getLocalHost().getHostName();
        } catch (UnknownHostException e) {
            osHostname = "unknown_host";
        }
        this.hostname = osHostname;
    }

    public long getPid() {
        return pid;
    }

    public String getHostname() {
        return hostname;
    }
}
