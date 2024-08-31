package ru.yandex.quasar.billing.providers;

/**
 * Headers for OTT to support 4k videos
 */
public class XDeviceHeaders {
    public static final String X_DEVICE_VIDEO_CODECS = "X-Device-Video-Codecs";
    public static final String X_DEVICE_AUDIO_CODECS = "X-Device-Audio-Codecs";
    public static final String SUPPORTS_CURRENT_HDCP_LEVEL = "supportsCurrentHDCPLevel";
    public static final String X_DEVICE_DYNAMIC_RANGES = "X-Device-Dynamic-Ranges";
    public static final String X_DEVICE_VIDEO_FORMATS = "X-Device-Video-Formats";

    private XDeviceHeaders() {
    }
}
