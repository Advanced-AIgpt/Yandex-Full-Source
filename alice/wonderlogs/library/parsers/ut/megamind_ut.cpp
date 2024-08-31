#include <alice/wonderlogs/library/parsers/megamind.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/yson/node/node_io.h>

namespace {

constexpr TStringBuf MEGAMIND_ANALYTICS_LOG = R"-({
    "_logfeller_index_bucket" = "//home/logfeller/index/megamind/analytics-log/1800-1800/1658137200/1658136600";
    "_logfeller_timestamp" = 1658136787u;
    "_stbx" = "rt3.sas--megamind--analytics-log:27@@28398323@@2022-07-17T10-39-40-120911Z_4fb3c513-1f73194f-29b6e54a-e5550669_0@@1658136787355@@1658136788@@analytics-log@@910@@1658136788368";
    "analytics_info" = {};
    "contains_sensitive_data" = %false;
    "content_type" = "application/json";
    "environment" = "megamind_standalone_ci";
    "hypothesis_number" = #;
    "iso_eventtime" = "2022-07-18 12:33:07";
    "provider" = "megamind";
    "quality_storage" = {};
    "request" = {
        "enrollment_headers" = {
            "headers" = [
                {
                    "version" = "speaker-0.1.3";
                    "person_id" = "PersId-e0907a68-1c64921a-c7bf8aa9-343c6143";
                    "user_type" = "GUEST";
                };
                {
                    "person_id" = "PersId-c8d05f95-ba52f4ad-fc1c9854-645ad9dd";
                    "user_type" = "__SYSTEM_OWNER_DO_NOT_USE_AFTER_2021";
                };
            ];
        };
        "request" = {
            "additional_options" = {};
            "environment_state" = {
                "devices" = [];
                "endpoints" = [
                    {
                        "capabilities" = [
                            {
                                "parameters" = {};
                                "meta" = {
                                    "reportable" = %false;
                                    "retrievable" = %false;
                                    "supported_directives" = [
                                        "SetFixedEqualizerBandsDirectiveType";
                                    ];
                                };
                                "@type" = "type.googleapis.com/NAlice.TEqualizerCapability";
                                "state" = {
                                    "preset_mode" = "Default";
                                };
                            };
                            {
                                "parameters" = {};
                                "meta" = {
                                    "reportable" = %false;
                                    "retrievable" = %false;
                                };
                                "@type" = "type.googleapis.com/NAlice.TBioCapability";
                                "state" = {};
                            };
                        ];
                        "status" = {};
                        "meta" = {
                            "type" = "SpeakerEndpointType";
                            "device_info" = {};
                        };
                        "id" = "XK0000000000000241800000a04c979e";
                    };
                ];
            };
            "test_ids" = [
                "348361";
                "409426";
                "408529";
                "416051";
                "412802";
                "430607";
                "419255";
                "439312";
                "432016";
                "444881";
                "418101";
                "446307";
                "456972";
                "451937";
                "448077";
                "445637";
                "474060";
                "466623";
                "479025";
                "484874";
                "493549";
                "474756";
                "499740";
                "511530";
                "518927";
                "495038";
                "514529";
                "503144";
                "546002";
                "548373";
                "543186";
                "532774";
                "555265";
                "552768";
                "607587";
                "605932";
                "610815";
                "570998";
                "571336";
                "613372";
                "341892";
                "341898";
                "341905";
                "341907";
                "341916";
                "341923";
                "341933";
                "341939";
                "341944";
                "341926";
                "524639";
                "524767";
                "598966";
                "567655";
                "583794";
                "600505";
                "586889";
                "613123";
                "611210";
                "545694";
                "591514";
                "457894";
                "286638";
                "614428";
            ];
            "voice_session" = %true;
            "smart_home" = {
                "payload" = {
                    "devices" = [
                        {
                            "name" = "\xD0\x9F\xD1\x83\xD0\xBB\xD1\x8C\xD1\x82";
                            "created" = 1610123278;
                            "type" = "devices.types.hub";
                            "id" = "08f8e2c2-8df0-4b6d-9e4c-bd3d14f16169";
                            "room_id" = "924d7bec-b13c-469e-a324-5bd2f16a09fe";
                        };
                        {
                            "name" = "\xD0\xAF\xD0\xBD\xD0\xB4\xD0\xB5\xD0\xBA\xD1\x81 \xD0\x9B\xD0\xB0\xD0\xB9\xD1\x82 8653";
                            "created" = 1657011332;
                            "type" = "devices.types.smart_speaker.yandex.station.micro";
                            "quasar_info" = {
                                "device_id" = "LY0000000000000000130000c3c98653";
                                "platform" = "yandexmicro";
                            };
                            "id" = "2124c424-9f53-459a-8ffe-c1655de5c5ca";
                        };
                        {
                            "name" = "\xD0\x9C\xD0\xBE\xD0\xB4\xD1\x83\xD0\xBB\xD1\x8C";
                            "created" = 1651047316;
                            "type" = "devices.types.media_device.dongle.yandex.module_2";
                            "quasar_info" = {
                                "device_id" = "D003TS0003XWKW";
                                "platform" = "yandexmodule_2";
                            };
                            "id" = "22054bd8-92f5-43cf-8c78-75c1b9b61558";
                            "room_id" = "924d7bec-b13c-469e-a324-5bd2f16a09fe";
                        };
                        {
                            "capabilities" = [
                                {
                                    "instance" = "brightness";
                                    "parameters" = {
                                        "instance" = "brightness";
                                    };
                                    "type" = "devices.capabilities.range";
                                };
                                {
                                    "instance" = "color";
                                    "parameters" = {};
                                    "type" = "devices.capabilities.color_setting";
                                };
                                {
                                    "instance" = "temperature_k";
                                    "parameters" = {};
                                    "type" = "devices.capabilities.color_setting";
                                };
                                {
                                    "instance" = "on";
                                    "parameters" = {};
                                    "type" = "devices.capabilities.on_off";
                                };
                            ];
                            "name" = "\xD0\x9B\xD0\xB0\xD0\xBC\xD0\xBF\xD0\xB0";
                            "created" = 1635530979;
                            "type" = "devices.types.light";
                            "id" = "2afbc4a8-06ed-4268-b260-f123ce861e26";
                            "room_id" = "a26514ec-ab32-43a4-8cf6-b2ff03b4fdb6";
                        };
                        {
                            "name" = "\xD0\xA1\xD1\x82\xD0\xB0\xD0\xBD\xD1\x86\xD0\xB8\xD1\x8F \xD0\x9C\xD0\xB8\xD0\xBD\xD0\xB8 new 68214";
                            "created" = 1652886046;
                            "type" = "devices.types.smart_speaker.yandex.station.mini_2";
                            "quasar_info" = {
                                "device_id" = "MG0000000000000424440000359c6821";
                                "platform" = "yandexmini_2";
                            };
                            "id" = "2b7b5702-24ac-417d-809d-e8902e229d05";
                            "room_id" = "00d7a268-587e-42dd-8897-e372bf5b0ec8";
                        };
                        {
                            "capabilities" = [
                                {
                                    "instance" = "1610154291468297243";
                                    "parameters" = {
                                        "instance" = "1610154291468297243";
                                        "instance_names" = [
                                            "\xD0\xA5\xD0\xB4\xD0\xBC\xD0\xB0 \xD0\xB8\xD1\x81\xD1\x82\xD0\xBE\xD1\x87\xD0\xBD\xD0\xB8\xD0\xBA \xD0\xB2\xD0\xB2\xD0\xB5\xD1\x80\xD1\x85";
                                        ];
                                    };
                                    "instance_names" = [
                                        "\xD0\xA5\xD0\xB4\xD0\xBC\xD0\xB0 \xD0\xB8\xD1\x81\xD1\x82\xD0\xBE\xD1\x87\xD0\xBD\xD0\xB8\xD0\xBA \xD0\xB2\xD0\xB2\xD0\xB5\xD1\x80\xD1\x85";
                                    ];
                                    "type" = "devices.capabilities.custom.button";
                                };
                                {
                                    "instance" = "1610154306015054941";
                                    "parameters" = {
                                        "instance" = "1610154306015054941";
                                        "instance_names" = [
                                            "\xD0\xA5\xD0\xB4\xD0\xBC\xD0\xB0 \xD0\xB8\xD1\x81\xD1\x82\xD0\xBE\xD1\x87\xD0\xBD\xD0\xB8\xD0\xBA \xD0\xB2\xD0\xBD\xD0\xB8\xD0\xB7";
                                        ];
                                    };
                                    "instance_names" = [
                                        "\xD0\xA5\xD0\xB4\xD0\xBC\xD0\xB0 \xD0\xB8\xD1\x81\xD1\x82\xD0\xBE\xD1\x87\xD0\xBD\xD0\xB8\xD0\xBA \xD0\xB2\xD0\xBD\xD0\xB8\xD0\xB7";
                                    ];
                                    "type" = "devices.capabilities.custom.button";
                                };
                                {
                                    "instance" = "1610154320267627000";
                                    "parameters" = {
                                        "instance" = "1610154320267627000";
                                        "instance_names" = [
                                            "\xD0\xA5\xD0\xB4\xD0\xBC\xD0\xB0 \xD0\xB8\xD1\x81\xD1\x82\xD0\xBE\xD1\x87\xD0\xBD\xD0\xB8\xD0\xBA \xD0\xBE\xD0\xBA";
                                        ];
                                    };
                                    "instance_names" = [
                                        "\xD0\xA5\xD0\xB4\xD0\xBC\xD0\xB0 \xD0\xB8\xD1\x81\xD1\x82\xD0\xBE\xD1\x87\xD0\xBD\xD0\xB8\xD0\xBA \xD0\xBE\xD0\xBA";
                                    ];
                                    "type" = "devices.capabilities.custom.button";
                                };
                            ];
                            "name" = "\xD0\xA5\xD0\xB4\xD0\xBC\xD0\xB0";
                            "created" = 1610154292;
                            "type" = "devices.types.other";
                            "id" = "38cfab9c-b62f-4c27-be6c-97f8933bf58c";
                            "room_id" = "924d7bec-b13c-469e-a324-5bd2f16a09fe";
                        };
                        {
                            "name" = "\xD0\xAF\xD0\xBD\xD0\xB4\xD0\xB5\xD0\xBA\xD1\x81 \xD0\x9B\xD0\xB0\xD0\xB9\xD1\x82 c5de";
                            "created" = 1650360768;
                            "type" = "devices.types.smart_speaker.yandex.station.micro";
                            "quasar_info" = {
                                "device_id" = "LG000000000000000454000087c5c5de";
                                "platform" = "yandexmicro";
                            };
                            "id" = "50f2a0b1-863f-4ac9-aa33-59b8057c2d05";
                        };
                        {
                            "name" = "\xD0\xAF\xD0\xBD\xD0\xB4\xD0\xB5\xD0\xBA\xD1\x81 \xD0\xA1\xD1\x82\xD0\xB0\xD0\xBD\xD1\x86\xD0\xB8\xD1\x8F";
                            "created" = 1648733601;
                            "type" = "devices.types.smart_speaker.yandex.station";
                            "quasar_info" = {
                                "device_id" = "74005034440c0821058e";
                                "platform" = "yandexstation";
                            };
                            "id" = "8696f4c4-ed1d-4b31-a8aa-30fb0d023ac4";
                        };
                        {
                            "capabilities" = [
                                {
                                    "instance" = "on";
                                    "parameters" = {};
                                    "type" = "devices.capabilities.on_off";
                                };
                                {
                                    "instance" = "work_speed";
                                    "values" = [
                                        "quiet";
                                        "normal";
                                        "fast";
                                        "turbo";
                                    ];
                                    "parameters" = {
                                        "instance" = "work_speed";
                                    };
                                    "type" = "devices.capabilities.mode";
                                };
                                {
                                    "instance" = "pause";
                                    "parameters" = {
                                        "instance" = "pause";
                                    };
                                    "type" = "devices.capabilities.toggle";
                                };
                            ];
                            "name" = "\xD0\x9F\xD0\xBE\xD0\xB4\xD0\xBA\xD1\x83\xD1\x81\xD1\x82\xD0\xBE\xD0\xB2\xD1\x8B\xD0\xB9 \xD0\x9A\xD0\xBB\xD0\xB0\xD0\xB2\xD0\xB4\xD0\xB5\xD0\xB7\xD1\x8C";
                            "created" = 1650358068;
                            "type" = "devices.types.vacuum_cleaner";
                            "id" = "86f5faf4-9317-48ef-b0dc-dbdd6923e3fe";
                            "room_id" = "924d7bec-b13c-469e-a324-5bd2f16a09fe";
                        };
                        {
                            "name" = "\xD0\xAF\xD0\xBD\xD0\xB4\xD0\xB5\xD0\xBA\xD1\x81 \xD0\xA2\xD0\x92";
                            "created" = 1647617087;
                            "type" = "devices.types.media_device.tv";
                            "quasar_info" = {
                                "device_id" = "cea2f65a462b3a6fad0e";
                                "platform" = "yandex_tv_mt9632_cv";
                            };
                            "id" = "9eaaa87c-c06c-4c2d-946f-ce8bdef8d5d0";
                            "room_id" = "924d7bec-b13c-469e-a324-5bd2f16a09fe";
                        };
                        {
                            "capabilities" = [
                                {
                                    "instance" = "on";
                                    "parameters" = {};
                                    "type" = "devices.capabilities.on_off";
                                };
                            ];
                            "name" = "\xD0\xA2\xD0\xBE\xD1\x80\xD1\x88\xD0\xB5\xD1\x80";
                            "created" = 1609878941;
                            "properties" = [
                                {
                                    "instance" = "voltage";
                                    "type" = "devices.properties.float";
                                };
                                {
                                    "instance" = "power";
                                    "type" = "devices.properties.float";
                                };
                                {
                                    "instance" = "amperage";
                                    "type" = "devices.properties.float";
                                };
                            ];
                            "type" = "devices.types.light";
                            "id" = "9f29bc06-a950-4218-8050-5ce4345f6089";
                            "room_id" = "924d7bec-b13c-469e-a324-5bd2f16a09fe";
                        };
                        {
                            "capabilities" = [
                                {
                                    "instance" = "1610192468009362459";
                                    "parameters" = {
                                        "instance" = "1610192468009362459";
                                        "instance_names" = [
                                            "\xD0\x92\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C \xD1\x81\xD0\xB2\xD0\xB8\xD0\xBD\xD0\xB3";
                                        ];
                                    };
                                    "instance_names" = [
                                        "\xD0\x92\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C \xD1\x81\xD0\xB2\xD0\xB8\xD0\xBD\xD0\xB3";
                                    ];
                                    "type" = "devices.capabilities.custom.button";
                                };
                            ];
                            "name" = "\xD0\xA1\xD0\xB2\xD0\xB8\xD0\xBD\xD0\xB3\xD0\xB5\xD1\x80\xD0\xBE\xD1\x84\xD1\x84";
                            "created" = 1610192469;
                            "type" = "devices.types.other";
                            "id" = "9f8ff4bd-3e35-47c1-b24b-0f70cbee5079";
                            "room_id" = "924d7bec-b13c-469e-a324-5bd2f16a09fe";
                        };
                        {
                            "capabilities" = [
                                {
                                    "instance" = "1617058074052901744";
                                    "parameters" = {
                                        "instance" = "1617058074052901744";
                                        "instance_names" = [
                                            "\xD0\xA1\xD0\xB2\xD0\xB8\xD0\xBD\xD0\xB3";
                                        ];
                                    };
                                    "instance_names" = [
                                        "\xD0\xA1\xD0\xB2\xD0\xB8\xD0\xBD\xD0\xB3";
                                    ];
                                    "type" = "devices.capabilities.custom.button";
                                };
                            ];
                            "name" = "\xD0\xA1\xD0\xB2\xD0\xB8\xD0\xBD\xD0\xB3\xD0\xB5\xD1\x80\xD0\xBE\xD0\xBD";
                            "created" = 1617058075;
                            "type" = "devices.types.other";
                            "id" = "a1ca82f7-c2a9-4d14-ba35-261f7f275215";
                            "room_id" = "924d7bec-b13c-469e-a324-5bd2f16a09fe";
                        };
                        {
                            "name" = "\xD0\x9C\xD0\xBE\xD0\xB4\xD1\x83\xD0\xBB\xD1\x91\xD0\xBA";
                            "created" = 1651139304;
                            "type" = "devices.types.media_device.dongle.yandex.module_2";
                            "quasar_info" = {
                                "device_id" = "D00YY0000BJZ7W";
                                "platform" = "yandexmodule_2";
                            };
                            "id" = "acbfa725-cd8d-4481-aa55-edaf5ef9c397";
                        };
                        {
                            "name" = "\xD0\xAF\xD0\xBD\xD0\xB4\xD0\xB5\xD0\xBA\xD1\x81 \xD0\xA1\xD1\x82\xD0\xB0\xD0\xBD\xD1\x86\xD0\xB8\xD1\x8F \xD0\x9C\xD0\xB0\xD0\xBA\xD1\x81 979e";
                            "created" = 1651150342;
                            "type" = "devices.types.smart_speaker.yandex.station_2";
                            "quasar_info" = {
                                "device_id" = "XK0000000000000241800000a04c979e";
                                "platform" = "yandexstation_2";
                            };
                            "id" = "adea9cd4-984b-42c3-955e-e7c63ce66eb6";
                        };
                        {
                            "name" = "\xD0\x94\xD0\xB0\xD1\x82\xD1\x87\xD0\xB8\xD0\xBA";
                            "created" = 1648974748;
                            "properties" = [
                                {
                                    "instance" = "temperature";
                                    "type" = "devices.properties.float";
                                };
                                {
                                    "instance" = "pressure";
                                    "type" = "devices.properties.float";
                                };
                                {
                                    "instance" = "humidity";
                                    "type" = "devices.properties.float";
                                };
                            ];
                            "type" = "devices.types.sensor";
                            "id" = "b27f4161-3405-457f-a464-2e50e7d50bfa";
                            "room_id" = "a26514ec-ab32-43a4-8cf6-b2ff03b4fdb6";
                        };
                        {
                            "capabilities" = [
                                {
                                    "instance" = "on";
                                    "parameters" = {};
                                    "type" = "devices.capabilities.on_off";
                                };
                            ];
                            "name" = "\xD0\xA6\xD0\xB2\xD0\xB5\xD1\x82\xD1\x8B";
                            "created" = 1609936203;
                            "properties" = [
                                {
                                    "instance" = "voltage";
                                    "type" = "devices.properties.float";
                                };
                                {
                                    "instance" = "power";
                                    "type" = "devices.properties.float";
                                };
                                {
                                    "instance" = "amperage";
                                    "type" = "devices.properties.float";
                                };
                            ];
                            "type" = "devices.types.socket";
                            "id" = "b7bdce17-ed2b-4d28-9718-cf20f47f0139";
                            "room_id" = "924d7bec-b13c-469e-a324-5bd2f16a09fe";
                        };
                        {
                            "capabilities" = [
                                {
                                    "instance" = "on";
                                    "parameters" = {};
                                    "type" = "devices.capabilities.on_off";
                                };
                                {
                                    "instance" = "temperature";
                                    "parameters" = {
                                        "instance" = "temperature";
                                    };
                                    "type" = "devices.capabilities.range";
                                };
                                {
                                    "instance" = "thermostat";
                                    "values" = [
                                        "cool";
                                        "heat";
                                        "auto";
                                        "fan_only";
                                        "dry";
                                    ];
                                    "parameters" = {
                                        "instance" = "thermostat";
                                    };
                                    "type" = "devices.capabilities.mode";
                                };
                                {
                                    "instance" = "fan_speed";
                                    "values" = [
                                        "medium";
                                        "auto";
                                        "high";
                                        "low";
                                    ];
                                    "parameters" = {
                                        "instance" = "fan_speed";
                                    };
                                    "type" = "devices.capabilities.mode";
                                };
                            ];
                            "name" = "\xD0\x9A\xD0\xBE\xD0\xBD\xD0\xB4\xD0\xB8\xD1\x86\xD0\xB8\xD0\xBE\xD0\xBD\xD0\xB5\xD1\x80";
                            "created" = 1610124512;
                            "type" = "devices.types.thermostat.ac";
                            "id" = "b8fc7726-90b6-48c4-912f-267acb43eeab";
                            "room_id" = "924d7bec-b13c-469e-a324-5bd2f16a09fe";
                        };
                        {
                            "capabilities" = [
                                {
                                    "instance" = "on";
                                    "parameters" = {};
                                    "type" = "devices.capabilities.on_off";
                                };
                                {
                                    "instance" = "mute";
                                    "parameters" = {
                                        "instance" = "mute";
                                    };
                                    "type" = "devices.capabilities.toggle";
                                };
                                {
                                    "instance" = "input_source";
                                    "values" = [
                                        "one";
                                    ];
                                    "parameters" = {
                                        "instance" = "input_source";
                                    };
                                    "type" = "devices.capabilities.mode";
                                };
                                {
                                    "instance" = "volume";
                                    "parameters" = {
                                        "instance" = "volume";
                                    };
                                    "type" = "devices.capabilities.range";
                                };
                                {
                                    "instance" = "channel";
                                    "parameters" = {
                                        "instance" = "channel";
                                    };
                                    "type" = "devices.capabilities.range";
                                };
                            ];
                            "name" = "\xD0\xA2\xD0\xB5\xD0\xBB\xD0\xB5\xD0\xB2\xD0\xB8\xD0\xB7\xD0\xBE\xD1\x80";
                            "created" = 1610123549;
                            "type" = "devices.types.media_device.tv";
                            "id" = "b9d940c3-8dca-486a-ab0b-82a7d41b2c9c";
                            "room_id" = "924d7bec-b13c-469e-a324-5bd2f16a09fe";
                        };
                        {
                            "capabilities" = [
                                {
                                    "instance" = "color";
                                    "parameters" = {};
                                    "type" = "devices.capabilities.color_setting";
                                };
                            ];
                            "name" = "\xD0\xAF\xD0\xBD\xD0\xB4\xD0\xB5\xD0\xBA\xD1\x81 \xD0\xA1\xD1\x82\xD0\xB0\xD0\xBD\xD1\x86\xD0\xB8\xD1\x8F 2 SKXB";
                            "created" = 1648657364;
                            "type" = "devices.types.smart_speaker.yandex.station.midi";
                            "quasar_info" = {
                                "device_id" = "U00T000004SKXB";
                                "platform" = "yandexmidi";
                            };
                            "id" = "c74e6549-8957-4e3c-9cab-5a12913bfdf1";
                            "room_id" = "a26514ec-ab32-43a4-8cf6-b2ff03b4fdb6";
                        };
                        {
                            "capabilities" = [
                                {
                                    "instance" = "on";
                                    "parameters" = {};
                                    "type" = "devices.capabilities.on_off";
                                };
                            ];
                            "name" = "\xD0\xA3\xD0\xB2\xD0\xBB\xD0\xB0\xD0\xB6\xD0\xBD\xD0\xB8\xD1\x82\xD0\xB5\xD0\xBB\xD1\x8C";
                            "created" = 1645878834;
                            "properties" = [
                                {
                                    "instance" = "humidity";
                                    "type" = "devices.properties.float";
                                };
                                {
                                    "instance" = "temperature";
                                    "type" = "devices.properties.float";
                                };
                            ];
                            "type" = "devices.types.humidifier";
                            "id" = "c797cd45-16d3-4605-9b57-d68b1d2774e7";
                            "room_id" = "924d7bec-b13c-469e-a324-5bd2f16a09fe";
                        };
                        {
                            "capabilities" = [
                                {
                                    "instance" = "1610154913220104932";
                                    "parameters" = {
                                        "instance" = "1610154913220104932";
                                        "instance_names" = [
                                            "\xD0\x92\xD0\xB2\xD0\xB5\xD1\x80\xD1\x85";
                                        ];
                                    };
                                    "instance_names" = [
                                        "\xD0\x92\xD0\xB2\xD0\xB5\xD1\x80\xD1\x85";
                                    ];
                                    "type" = "devices.capabilities.custom.button";
                                };
                                {
                                    "instance" = "1610154922864284753";
                                    "parameters" = {
                                        "instance" = "1610154922864284753";
                                        "instance_names" = [
                                            "\xD0\x92\xD0\xBD\xD0\xB8\xD0\xB7";
                                        ];
                                    };
                                    "instance_names" = [
                                        "\xD0\x92\xD0\xBD\xD0\xB8\xD0\xB7";
                                    ];
                                    "type" = "devices.capabilities.custom.button";
                                };
                                {
                                    "instance" = "1610154934855186462";
                                    "parameters" = {
                                        "instance" = "1610154934855186462";
                                        "instance_names" = [
                                            "\xD0\x92\xD0\xBB\xD0\xB5\xD0\xB2\xD0\xBE";
                                        ];
                                    };
                                    "instance_names" = [
                                        "\xD0\x92\xD0\xBB\xD0\xB5\xD0\xB2\xD0\xBE";
                                    ];
                                    "type" = "devices.capabilities.custom.button";
                                };
                                {
                                    "instance" = "1610154942854510307";
                                    "parameters" = {
                                        "instance" = "1610154942854510307";
                                        "instance_names" = [
                                            "\xD0\x92\xD0\xBF\xD1\x80\xD0\xB0\xD0\xB2\xD0\xBE";
                                        ];
                                    };
                                    "instance_names" = [
                                        "\xD0\x92\xD0\xBF\xD1\x80\xD0\xB0\xD0\xB2\xD0\xBE";
                                    ];
                                    "type" = "devices.capabilities.custom.button";
                                };
                                {
                                    "instance" = "1610154952956137657";
                                    "parameters" = {
                                        "instance" = "1610154952956137657";
                                        "instance_names" = [
                                            "\xD0\x9E\xD0\xBA";
                                        ];
                                    };
                                    "instance_names" = [
                                        "\xD0\x9E\xD0\xBA";
                                    ];
                                    "type" = "devices.capabilities.custom.button";
                                };
                            ];
                            "name" = "\xD0\x9F\xD1\x83\xD0\xBB\xD1\x8C\xD1\x82";
                            "created" = 1610154914;
                            "type" = "devices.types.other";
                            "id" = "cf2ae2af-ebae-4be5-a1fd-966700111676";
                            "room_id" = "924d7bec-b13c-469e-a324-5bd2f16a09fe";
                        };
                        {
                            "name" = "\xD0\xAF\xD0\xBD\xD0\xB4\xD0\xB5\xD0\xBA\xD1\x81 \xD0\xA1\xD1\x82\xD0\xB0\xD0\xBD\xD1\x86\xD0\xB8\xD1\x8F \xD0\x9C\xD0\xB0\xD0\xBA\xD1\x81 \xD0\x94\xD0\xBE\xD0\x9C";
                            "created" = 1649206316;
                            "type" = "devices.types.smart_speaker.yandex.station_2";
                            "quasar_info" = {
                                "device_id" = "XW00000000000001020400005e5c472a";
                                "platform" = "yandexstation_2";
                            };
                            "id" = "dc9bbfde-b47f-4601-a190-29bca7d73efe";
                        };
                        {
                            "name" = "\xD0\xA1\xD1\x82\xD0\xB0\xD0\xBD\xD1\x86\xD0\xB8\xD1\x8F \xD0\x9C\xD0\xB8\xD0\xBD\xD0\xB8 new 8cab";
                            "created" = 1657791471;
                            "type" = "devices.types.smart_speaker.yandex.station.mini_2";
                            "quasar_info" = {
                                "device_id" = "MR0000000000000001900000e2988cab";
                                "platform" = "yandexmini_2";
                            };
                            "id" = "e0ace96c-1c37-433f-9f5e-c76f225b5128";
                        };
                        {
                            "name" = "\xD0\xA1\xD1\x82\xD0\xB0\xD0\xBD\xD1\x86\xD0\xB8\xD1\x8F \xD0\x9C\xD0\xB8\xD0\xBD\xD0\xB8 new 58d2";
                            "created" = 1651070224;
                            "type" = "devices.types.smart_speaker.yandex.station.mini_2";
                            "quasar_info" = {
                                "device_id" = "MK00000000000000051700004cf158d2";
                                "platform" = "yandexmini_2";
                            };
                            "id" = "efb1c881-ab4a-4bc8-b2c6-68f6efaa8a81";
                            "room_id" = "924d7bec-b13c-469e-a324-5bd2f16a09fe";
                        };
                    ];
                    "colors" = [
                        {
                            "name" = "\xD0\x9E\xD0\xB3\xD0\xBD\xD0\xB5\xD0\xBD\xD0\xBD\xD1\x8B\xD0\xB9 \xD0\xB1\xD0\xB5\xD0\xBB\xD1\x8B\xD0\xB9";
                            "id" = "fiery_white";
                        };
                        {
                            "name" = "\xD0\x97\xD0\xB5\xD0\xBB\xD0\xB5\xD0\xBD\xD1\x8B\xD0\xB9";
                            "id" = "green";
                        };
                        {
                            "name" = "\xD0\x9E\xD1\x80\xD1\x85\xD0\xB8\xD0\xB4\xD0\xB5\xD1\x8F";
                            "id" = "orchid";
                        };
                        {
                            "name" = "\xD0\xA4\xD0\xB8\xD0\xBE\xD0\xBB\xD0\xB5\xD1\x82\xD0\xBE\xD0\xB2\xD1\x8B\xD0\xB9";
                            "id" = "violet";
                        };
                        {
                            "name" = "\xD0\xA5\xD0\xBE\xD0\xBB\xD0\xBE\xD0\xB4\xD0\xBD\xD1\x8B\xD0\xB9 \xD0\xB1\xD0\xB5\xD0\xBB\xD1\x8B\xD0\xB9";
                            "id" = "cold_white";
                        };
                        {
                            "name" = "\xD0\x96\xD0\xB5\xD0\xBB\xD1\x82\xD1\x8B\xD0\xB9";
                            "id" = "yellow";
                        };
                        {
                            "name" = "\xD0\x9C\xD1\x8F\xD0\xB3\xD0\xBA\xD0\xB8\xD0\xB9 \xD0\xB1\xD0\xB5\xD0\xBB\xD1\x8B\xD0\xB9";
                            "id" = "soft_white";
                        };
                        {
                            "name" = "\xD0\x98\xD0\xB7\xD1\x83\xD0\xBC\xD1\x80\xD1\x83\xD0\xB4\xD0\xBD\xD1\x8B\xD0\xB9";
                            "id" = "emerald";
                        };
                        {
                            "name" = "\xD0\x9B\xD1\x83\xD0\xBD\xD0\xBD\xD1\x8B\xD0\xB9";
                            "id" = "moonlight";
                        };
                        {
                            "name" = "\xD0\xA0\xD0\xBE\xD0\xB7\xD0\xBE\xD0\xB2\xD1\x8B\xD0\xB9";
                            "id" = "orchid";
                        };
                        {
                            "name" = "\xD0\xA2\xD0\xB5\xD0\xBF\xD0\xBB\xD1\x8B\xD0\xB9 \xD0\xB1\xD0\xB5\xD0\xBB\xD1\x8B\xD0\xB9";
                            "id" = "warm_white";
                        };
                        {
                            "name" = "\xD0\x9D\xD0\xBE\xD1\x80\xD0\xBC\xD0\xB0\xD0\xBB\xD1\x8C\xD0\xBD\xD1\x8B\xD0\xB9";
                            "id" = "white";
                        };
                        {
                            "name" = "\xD0\x94\xD0\xBD\xD0\xB5\xD0\xB2\xD0\xBD\xD0\xBE\xD0\xB9 \xD0\xB1\xD0\xB5\xD0\xBB\xD1\x8B\xD0\xB9";
                            "id" = "daylight";
                        };
                        {
                            "name" = "\xD0\x9E\xD1\x80\xD0\xB0\xD0\xBD\xD0\xB6\xD0\xB5\xD0\xB2\xD1\x8B\xD0\xB9";
                            "id" = "orange";
                        };
                        {
                            "name" = "\xD0\x9F\xD1\x83\xD1\x80\xD0\xBF\xD1\x83\xD1\x80\xD0\xBD\xD1\x8B\xD0\xB9";
                            "id" = "purple";
                        };
                        {
                            "name" = "\xD0\x93\xD0\xBE\xD0\xBB\xD1\x83\xD0\xB1\xD0\xBE\xD0\xB9";
                            "id" = "cyan";
                        };
                        {
                            "name" = "\xD0\xA1\xD0\xB8\xD0\xBD\xD0\xB8\xD0\xB9";
                            "id" = "blue";
                        };
                        {
                            "name" = "\xD0\x9C\xD0\xB0\xD0\xBB\xD0\xB8\xD0\xBD\xD0\xBE\xD0\xB2\xD1\x8B\xD0\xB9";
                            "id" = "raspberry";
                        };
                        {
                            "name" = "\xD0\x9E\xD0\xB3\xD0\xBD\xD0\xB5\xD0\xBD\xD0\xBD\xD1\x8B\xD0\xB9";
                            "id" = "fiery_white";
                        };
                        {
                            "name" = "\xD0\x91\xD0\xB5\xD0\xBB\xD1\x8B\xD0\xB9";
                            "id" = "white";
                        };
                        {
                            "name" = "\xD0\x9E\xD0\xB1\xD1\x8B\xD1\x87\xD0\xBD\xD1\x8B\xD0\xB9";
                            "id" = "white";
                        };
                        {
                            "name" = "\xD0\x9A\xD1\x80\xD0\xB0\xD1\x81\xD0\xBD\xD1\x8B\xD0\xB9";
                            "id" = "red";
                        };
                        {
                            "name" = "\xD0\x9A\xD0\xBE\xD1\x80\xD0\xB0\xD0\xBB\xD0\xBB\xD0\xBE\xD0\xB2\xD1\x8B\xD0\xB9";
                            "id" = "coral";
                        };
                        {
                            "name" = "\xD0\x9B\xD0\xB8\xD0\xBB\xD0\xBE\xD0\xB2\xD1\x8B\xD0\xB9";
                            "id" = "mauve";
                        };
                        {
                            "name" = "\xD0\xA1\xD0\xB0\xD0\xBB\xD0\xB0\xD1\x82\xD0\xBE\xD0\xB2\xD1\x8B\xD0\xB9";
                            "id" = "lime";
                        };
                        {
                            "name" = "\xD0\x91\xD0\xB8\xD1\x80\xD1\x8E\xD0\xB7\xD0\xBE\xD0\xB2\xD1\x8B\xD0\xB9";
                            "id" = "turquoise";
                        };
                        {
                            "name" = "\xD0\xA1\xD0\xB8\xD1\x80\xD0\xB5\xD0\xBD\xD0\xB5\xD0\xB2\xD1\x8B\xD0\xB9";
                            "id" = "lavender";
                        };
                        {
                            "name" = "\xD0\x9C\xD0\xB0\xD0\xBB\xD0\xB8\xD0\xBD\xD0\xB0";
                            "id" = "raspberry";
                        };
                    ];
                    "rooms" = [
                        {
                            "name" = "\xD0\x91\xD0\xB0\xD0\xBB\xD0\xBA\xD0\xBE\xD0\xBD";
                            "id" = "00d7a268-587e-42dd-8897-e372bf5b0ec8";
                        };
                        {
                            "name" = "\xD0\xA1\xD0\xBF\xD0\xB0\xD0\xBB\xD1\x8C\xD0\xBD\xD1\x8F";
                            "id" = "924d7bec-b13c-469e-a324-5bd2f16a09fe";
                        };
                        {
                            "name" = "\xD0\x97\xD0\xB0\xD0\xBB";
                            "id" = "a26514ec-ab32-43a4-8cf6-b2ff03b4fdb6";
                        };
                    ];
                    "scenarios" = [
                        {
                            "name" = "\xD0\xA1\xD1\x83\xD1\x85\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C \xD0\xBF\xD0\xBE\xD0\xB1\xD0\xB5\xD0\xB6\xD0\xB4\xD0\xB5\xD0\xBD\xD0\xB0";
                            "id" = "3a6a78a9-3bbb-4807-92e4-5aaefce27dfa";
                        };
                        {
                            "name" = "\xD0\x92\xD0\xBB\xD0\xB0\xD0\xB6\xD0\xBD\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C";
                            "id" = "46a881f3-f19d-44e0-96ba-77eb1ed0fbee";
                        };
                        {
                            "name" = "\xD0\x94\xD1\x83\xD1\x88\xD0\xBD\xD0\xBE";
                            "triggers" = [
                                {
                                    "type" = "scenario.trigger.voice";
                                    "value" = "\xD0\x94\xD1\x83\xD1\x88\xD0\xBD\xD0\xBE";
                                };
                            ];
                            "id" = "51229857-ffe7-4cc6-af92-1b5acc57f77b";
                        };
                        {
                            "name" = "\xD0\x92\xD1\x8B\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8 \xD1\x82\xD0\xBE\xD1\x80\xD1\x88\xD0\xB5\xD1\x80";
                            "triggers" = [
                                {
                                    "type" = "scenario.trigger.voice";
                                    "value" = "\xD0\x92\xD1\x8B\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8 \xD1\x82\xD0\xBE\xD1\x80\xD1\x88\xD0\xB5\xD1\x80";
                                };
                            ];
                            "id" = "5a05b568-8e33-4bcd-bc04-2ec63e3e791a";
                        };
                        {
                            "name" = "\xD0\x92\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8 \xD1\x82\xD0\xBE\xD1\x80\xD1\x88\xD0\xB5\xD1\x80";
                            "triggers" = [
                                {
                                    "type" = "scenario.trigger.voice";
                                    "value" = "\xD0\x92\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8 \xD1\x82\xD0\xBE\xD1\x80\xD1\x88\xD0\xB5\xD1\x80";
                                };
                            ];
                            "id" = "6f648f57-2716-46df-9d2c-2640b30162d3";
                        };
                        {
                            "name" = "\xD0\xA1\xD0\xB2\xD0\xB8\xD0\xBD\xD0\xB3\xD0\xB5\xD1\x80 \xD0\xB2\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C";
                            "triggers" = [
                                {
                                    "type" = "scenario.trigger.voice";
                                    "value" = "\xD0\xA1\xD0\xB2\xD0\xB8\xD0\xBD\xD0\xB3\xD0\xB5\xD1\x80 \xD0\xB2\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C";
                                };
                            ];
                            "id" = "750d2f78-0dc5-4509-baf8-e443594dbec2";
                        };
                        {
                            "name" = "\xD0\x9F\xD0\xA1 4";
                            "triggers" = [
                                {
                                    "type" = "scenario.trigger.voice";
                                    "value" = "\xD0\x9F\xD0\xB5\xD1\x80\xD0\xB5\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8 \xD0\xB8\xD1\x81\xD1\x82\xD0\xBE\xD1\x87\xD0\xBD\xD0\xB8\xD0\xBA \xD0\xBD\xD0\xB0 \xD0\xBA\xD0\xBE\xD0\xBD\xD1\x81\xD0\xBE\xD0\xBB\xD1\x8C";
                                };
                            ];
                            "id" = "82c38ead-c4d3-45cd-8dd0-0e2968618593";
                        };
                        {
                            "name" = "\xD0\xA6\xD0\xB2\xD0\xB5\xD1\x82\xD1\x8B";
                            "id" = "8da5e759-5c43-477c-93eb-6026ddcddf27";
                        };
                        {
                            "name" = "\xD0\xA6\xD0\xB2\xD0\xB5\xD1\x82\xD1\x8B \xD0\xB2\xD1\x8B\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB0\xD1\x8E\xD1\x82\xD1\x81\xD1\x8F";
                            "id" = "8dc75895-eff4-40d8-bc18-80c408a4635a";
                        };
                        {
                            "name" = "\xD0\x96\xD0\xB0\xD1\x80\xD0\xBA\xD0\xBE";
                            "triggers" = [
                                {
                                    "type" = "scenario.trigger.voice";
                                    "value" = "\xD0\x96\xD0\xB0\xD1\x80\xD0\xBA\xD0\xBE";
                                };
                            ];
                            "id" = "abfb80fe-e7e7-4f3b-90af-7407268801b8";
                        };
                        {
                            "name" = "\xD0\xA5\xD0\xBE\xD0\xBB\xD0\xBE\xD0\xB4\xD0\xBD\xD0\xBE";
                            "triggers" = [
                                {
                                    "type" = "scenario.trigger.voice";
                                    "value" = "\xD0\xA5\xD0\xBE\xD0\xBB\xD0\xBE\xD0\xB4\xD0\xBD\xD0\xBE";
                                };
                            ];
                            "id" = "c345737e-1efc-43af-b2d4-d6e111e04983";
                        };
                        {
                            "name" = "\xD0\x91\xD0\xB0\xD1\x87\xD0\xBE\xD0\xBA \xD0\xBF\xD1\x80\xD0\xBE\xD1\x82\xD0\xB8\xD0\xBA";
                            "triggers" = [
                                {
                                    "type" = "scenario.trigger.voice";
                                    "value" = "\xD0\x91\xD0\xB0\xD1\x87\xD0\xBE\xD0\xBA \xD0\xBF\xD1\x80\xD0\xBE\xD1\x82\xD0\xB8\xD0\xBA";
                                };
                            ];
                            "id" = "d1dd296b-800d-4e35-83ca-c5ba6b3d21bf";
                        };
                        {
                            "name" = "\xD0\x9A\xD0\xBE\xD0\xB4\xD0\xB8";
                            "triggers" = [
                                {
                                    "type" = "scenario.trigger.voice";
                                    "value" = "\xD0\xA2\xD0\xB5\xD0\xBB\xD0\xB5\xD0\xB2\xD0\xB8\xD0\xB7\xD0\xBE\xD1\x80 \xD0\xB8\xD1\x81\xD1\x82\xD0\xBE\xD1\x87\xD0\xBD\xD0\xB8\xD0\xBA \xD0\xBA\xD0\xBE\xD0\xB4\xD0\xB8";
                                };
                            ];
                            "id" = "d80b5c42-2c6d-4df9-8f73-c0116e84326c";
                        };
                        {
                            "name" = "\xD0\xA1\xD0\xB2\xD0\xB8\xD0\xBD\xD0\xB3\xD0\xB5\xD1\x80 \xD0\xB2\xD1\x8B\xD0\xBA\xD0\xBB\xD1\x83\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C";
                            "triggers" = [
                                {
                                    "type" = "scenario.trigger.voice";
                                    "value" = "\xD0\xA1\xD0\xB2\xD0\xB8\xD0\xBD\xD0\xB3\xD0\xB5\xD1\x80 \xD0\xB2\xD1\x8B\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C";
                                };
                            ];
                            "id" = "f09f2837-1cbc-4707-87a9-bcf976a331df";
                        };
                    ];
                };
            };
            "activation_type" = "directive";
            "raw_personal_data" = "{\"/v1/personality/profile/alisa/kv/alice_children_biometry\":\"enabled\",\"/v1/personality/profile/alisa/kv/alice_proactivity\":\"enabled\",\"/v1/personality/profile/alisa/kv/gender\":\"female\",\"/v1/personality/profile/alisa/kv/guest_uid\":\"1644741096\",\"/v1/personality/profile/alisa/kv/morning_show\":\"{\\\"last_push_timestamp\\\":1640589885,\\\"pushes_sent\\\":3}\",\"/v1/personality/profile/alisa/kv/proactivity_history\":\"{}\",\"/v1/personality/profile/alisa/kv/taxi_history\":\"{\\\"iPhone\\\":null,\\\"iPhone5F366176-FDB3-48D7-8EF1-C31A7FB08880\\\":null,\\\"iPhone65EDDF57-FCE5-4CFE-BE48-3351BC1CDB1D\\\":null,\\\"last_card\\\":null,\\\"last_payment_method\\\":{\\\"id\\\":\\\"cash\\\",\\\"type\\\":\\\"cash\\\"},\\\"previous_tariff\\\":\\\"business\\\",\\\"user_already_get_offer_in_smart_speaker\\\":true,\\\"user_already_made_orders_in_any_app\\\":true,\\\"yandexstation4410789684084c1a0210\\\":{\\\"last_taxi_address\\\":{\\\"address_line\\\":\\\"\xD0\xA0\xD0\xBE\xD1\x81\xD1\x81\xD0\xB8\xD1\x8F, \xD0\x9C\xD0\xBE\xD1\x81\xD0\xBA\xD0\xB2\xD0\xB0, \xD0\x91\xD0\xB0\xD0\xBA\xD0\xB8\xD0\xBD\xD1\x81\xD0\xBA\xD0\xB0\xD1\x8F \xD1\x83\xD0\xBB\xD0\xB8\xD1\x86\xD0\xB0, 31\\\",\\\"city\\\":\\\"\xD0\x9C\xD0\xBE\xD1\x81\xD0\xBA\xD0\xB2\xD0\xB0\\\",\\\"city_cases\\\":{\\\"dative\\\":\\\"\xD0\x9C\xD0\xBE\xD1\x81\xD0\xBA\xD0\xB2\xD0\xB5\\\",\\\"genitive\\\":\\\"\xD0\x9C\xD0\xBE\xD1\x81\xD0\xBA\xD0\xB2\xD1\x8B\\\",\\\"nominative\\\":\\\"\xD0\x9C\xD0\xBE\xD1\x81\xD0\xBA\xD0\xB2\xD0\xB0\\\",\\\"preposition\\\":\\\"\xD0\xB2\\\",\\\"prepositional\\\":\\\"\xD0\x9C\xD0\xBE\xD1\x81\xD0\xBA\xD0\xB2\xD0\xB5\\\"},\\\"city_prepcase\\\":\\\"\xD0\xB2 \xD0\x9C\xD0\xBE\xD1\x81\xD0\xBA\xD0\xB2\xD0\xB5\\\",\\\"country\\\":\\\"\xD0\xA0\xD0\xBE\xD1\x81\xD1\x81\xD0\xB8\xD1\x8F\\\",\\\"geo_uri\\\":\\\"https://yandex.ru/maps?ll=37.660251%2C55.617022&ol=geo&oll=37.660251%2C55.617022&text=%D0%A0%D0%BE%D1%81%D1%81%D0%B8%D1%8F%2C%20%D0%9C%D0%BE%D1%81%D0%BA%D0%B2%D0%B0%2C%20%D0%91%D0%B0%D0%BA%D0%B8%D0%BD%D1%81%D0%BA%D0%B0%D1%8F%20%D1%83%D0%BB%D0%B8%D1%86%D0%B0%2C%2031\\\",\\\"geoid\\\":213,\\\"house\\\":\\\"31\\\",\\\"in_user_city\\\":false,\\\"level\\\":\\\"inside_city\\\",\\\"location\\\":{\\\"lat\\\":55.61702201,\\\"lon\\\":37.66025132},\\\"street\\\":\\\"\xD0\x91\xD0\xB0\xD0\xBA\xD0\xB8\xD0\xBD\xD1\x81\xD0\xBA\xD0\xB0\xD1\x8F \xD1\x83\xD0\xBB\xD0\xB8\xD1\x86\xD0\xB0\\\"}}}\",\"/v1/personality/profile/alisa/kv/user_name\":\"\xD0\xBD\xD0\xBE\xD0\xB2\xD0\xB0\xD1\x8F \xD1\x85\xD0\xBE\xD0\xB7\xD1\x8F\xD0\xB9\xD0\xBA\xD0\xB0\",\"/v1/personality/profile/alisa/kv/video_rater\":\"[{\\\"kinopoisk_id\\\":\\\"film/1303058\\\",\\\"score\\\":4,\\\"text_score\\\":\\\"\xD0\xBB\xD0\xB0\xD0\xB9\xD0\xBA\\\",\\\"timestamp\\\":1628585208,\\\"timezone\\\":\\\"Europe/Moscow\\\"},{\\\"kinopoisk_id\\\":\\\"film/718222\\\",\\\"score\\\":1,\\\"text_score\\\":\\\"\xD0\xB4\xD0\xB8\xD0\xB7\xD0\xBB\xD0\xB0\xD0\xB9\xD0\xBA\\\",\\\"timestamp\\\":1628585208,\\\"timezone\\\":\\\"Europe/Moscow\\\"},{\\\"kinopoisk_id\\\":\\\"film/1200179\\\",\\\"score\\\":3,\\\"text_score\\\":\\\"\xD0\xBE\xD0\xB1\xD1\x8B\xD1\x87\xD0\xBD\xD1\x8B\xD0\xB9\\\",\\\"timestamp\\\":1628585208,\\\"timezone\\\":\\\"Europe/Moscow\\\"},{\\\"kinopoisk_id\\\":\\\"film/1405843\\\",\\\"score\\\":4,\\\"text_score\\\":\\\"\xD0\xBB\xD0\xB0\xD0\xB9\xD0\xBA\\\",\\\"timestamp\\\":1628585208,\\\"timezone\\\":\\\"Europe/Moscow\\\"},{\\\"kinopoisk_id\\\":\\\"film/1224067\\\",\\\"score\\\":4,\\\"text_score\\\":\\\"\xD0\xBB\xD0\xB0\xD0\xB9\xD0\xBA\\\",\\\"timestamp\\\":1628585208,\\\"timezone\\\":\\\"Europe/Moscow\\\"}]\",\"/v1/personality/profile/alisa/kv/yandexstation_4410789684084c1a0210_location\":\"{\\\"location\\\":\\\"home\\\"}\\n\"}";
            "event" = {
                "name" = "@@mm_semantic_frame";
                "payload" = {
                    "typed_semantic_frame" = {
                        "guest_enrollment_start_semantic_frame" = {
                            "puid" = {
                                "string_value" = "526276160";
                            };
                        };
                    };
                    "analytics" = {
                        "origin" = "SmartSpeaker";
                        "purpose" = "voiceprint_enroll";
                    };
                };
                "type" = "server_action";
            };
            "notification_state" = {
                "subscriptions" = [
                    {
                        "name" = "\xD0\x91\xD0\xBE\xD0\xBB\xD1\x8C\xD1\x88\xD0\xB5 \xD0\xBC\xD1\x83\xD0\xB7\xD1\x8B\xD0\xBA\xD0\xB8";
                        "timestamp" = "1632145343944840";
                        "id" = "3";
                    };
                ];
            };
            "laas_region" = {
                "suspected_longitude" = 37.587093;
                "is_yandex_staff" = %false;
                "country_id_by_ip" = 225;
                "probable_regions" = [
                    {
                        "region_id" = 21641;
                        "weight" = 0.5;
                    };
                ];
                "is_hosting" = %false;
                "region_home" = 21641;
                "region_id" = 120542;
                "latitude" = 55.733974;
                "location_accuracy" = 1;
                "is_serp_trusted_net" = %false;
                "precision" = 2;
                "suspected_location_accuracy" = 1;
                "suspected_location_unixtime" = 1658136787;
                "is_yandex_net" = %true;
                "is_public_proxy" = %false;
                "is_mobile" = %false;
                "suspected_region_city" = 213;
                "regular_coordinates" = [
                    {
                        "lat" = 56.017732;
                        "lon" = 37.471355;
                        "type" = 1;
                    };
                    {
                        "lat" = 56.014584;
                        "lon" = 37.463452;
                        "type" = 0;
                    };
                    {
                        "lat" = 56.016896;
                        "lon" = 37.470207;
                        "type" = 0;
                    };
                ];
                "is_tor" = %false;
                "region_by_ip" = 192;
                "is_user_choice" = %false;
                "is_gdpr" = %false;
                "longitude" = 37.587093;
                "probable_regions_reliability" = 1;
                "is_anonymous_vpn" = %false;
                "suspected_precision" = 2;
                "location_unixtime" = 1658136787;
                "should_update_cookie" = %false;
                "suspected_region_id" = 120542;
                "city_id" = 213;
                "suspected_latitude" = 55.733974;
            };
            "device_state" = {
                "screen" = {
                    "supported_screen_resolutions" = [
                        "video_format_SD";
                        "video_format_HD";
                    ];
                    "hdcp_level" = "current_HDCP_level_1X";
                    "dynamic_ranges" = [
                        "dynamic_range_SDR";
                    ];
                };
                "device_id" = "XK0000000000000241800000a04c979e";
                "timers" = {};
                "video" = {
                    "view_state" = {
                        "currentScreen" = "morda/home";
                        "user_subscription_type" = "YA_PREMIUM";
                        "sections" = [
                            {
                                "active" = %true;
                                "type" = "main";
                                "items" = [
                                    {
                                        "metaforback" = {
                                            "serial_id" = "4dbe3685655d2e7e99ea665f4b23ab29";
                                            "url" = "https://strm.yandex.ru/vod/vh-ottenc-converted/vod-content/4a5865cf87875175b5de7234dd1dbe79/9550321x1656156750x3c827e19-1d08-47a5-8b30-55368fa9330f/kaltura/dash_drm_sdr_hd_avc_aac_25cfc48a1c48372c92605bd44638196a/4a5865cf87875175b5de7234dd1dbe79/ysign1=d194c2c5067e61d69dd3b1d267a3d66bb2f949c67111e8dafc74fc3ee6ae4520,abcID=3386,from=ya-station,pfx,region=225,sfx,ts=62e254d0/manifest.mpd";
                                            "season_id" = "48b0a8f7c73daa138f13bca0d11a2a7e";
                                            "subscriptions" = [
                                                "YA_PLUS_SUPER";
                                                "YA_PREMIUM";
                                                "YA_PLUS_3M";
                                                "YA_PLUS";
                                                "KP_BASIC";
                                                "YA_PLUS_KP";
                                            ];
                                            "title" = "\xD0\x95\xD1\x80\xD0\xB0\xD0\xBB\xD0\xB0\xD1\x88";
                                            "uuid" = "4a5865cf87875175b5de7234dd1dbe79";
                                        };
                                        "number" = 1;
                                        "active" = %true;
                                        "type" = "video";
                                        "title" = "\xD0\x95\xD1\x80\xD0\xB0\xD0\xBB\xD0\xB0\xD1\x88";
                                        "metaforlog" = {
                                            "source_carousel_id" = "ChFoaG9nd3dra3h5dWF1c3RoaBIIc3RhdGlvbjIaDGVudGl0eV9taXhlZCABKAA=";
                                            "supertag_title" = "\xD0\xA1\xD0\xB5\xD1\x80\xD0\xB8\xD0\xB0\xD0\xBB";
                                            "source_carousel" = "recommendation";
                                            "is_promo" = 0;
                                            "supertag" = "series";
                                            "onto_otype" = "Film/Series@on";
                                            "restriction_age" = 0;
                                            "genres" = "\xD0\xB4\xD0\xB5\xD1\x82\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9, \xD0\xBA\xD0\xBE\xD0\xBC\xD0\xB5\xD0\xB4\xD0\xB8\xD1\x8F";
                                            "is_special_project" = 0;
                                            "source_carousel_position" = "0";
                                            "release_year" = 1974;
                                            "onto_category" = "series";
                                            "rating_kp" = 7.239999771;
                                            "content_type_name" = "vod-episode";
                                            "can_play_on_station" = 1;
                                        };
                                    };
                                    {
                                        "metaforback" = {
                                            "serial_id" = "4fafdc0d3088cff68b711839855235f3";
                                            "url" = "https://strm.yandex.ru/vod/vh-ottenc-converted/vod-content/4585a429002f35498ac25bdb25d242ce/9550321x1655973237x2a18193c-9c70-421e-93d9-b2981534d063/kaltura/dash_drm_sdr_hd_avc_aac_1f2e2f6114b968db5aa9f1de2999ccd0/4585a429002f35498ac25bdb25d242ce/ysign1=477124eaf3d164ea36d17dbc1f44e8909abdecf880a7b2ba2a8493a4bc66bb30,abcID=3386,from=ya-station,pfx,region=225,sfx,ts=62e254d0/manifest.mpd";
                                            "season_id" = "491619c511adede48dff39fd7f3caa51";
                                            "subscriptions" = [
                                                "KP_BASIC";
                                                "YA_PLUS";
                                                "YA_PLUS_SUPER";
                                                "YA_PLUS_3M";
                                                "YA_PREMIUM";
                                                "YA_PLUS_KP";
                                            ];
                                            "title" = "\xD0\xA2\xD1\x80\xD0\xB8\xD0\xBD\xD0\xB0\xD0\xB4\xD1\x86\xD0\xB0\xD1\x82\xD1\x8C";
                                            "uuid" = "4585a429002f35498ac25bdb25d242ce";
                                        };
                                        "number" = 2;
                                        "active" = %true;
                                        "type" = "video";
                                        "title" = "\xD0\xA2\xD1\x80\xD0\xB8\xD0\xBD\xD0\xB0\xD0\xB4\xD1\x86\xD0\xB0\xD1\x82\xD1\x8C";
                                        "metaforlog" = {
                                            "source_carousel_id" = "ChFoaG9nd3dra3h5dWF1c3RoaBIIc3RhdGlvbjIaDGVudGl0eV9taXhlZCABKAA=";
                                            "supertag_title" = "\xD0\x9A\xD0\xB8\xD0\xBD\xD0\xBE\xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA HD";
                                            "source_carousel" = "recommendation";
                                            "is_promo" = 0;
                                            "supertag" = "subscription";
                                            "onto_otype" = "Film/Series@on";
                                            "restriction_age" = 16;
                                            "genres" = "\xD1\x82\xD1\x80\xD0\xB8\xD0\xBB\xD0\xBB\xD0\xB5\xD1\x80, \xD0\xB4\xD1\x80\xD0\xB0\xD0\xBC\xD0\xB0, \xD0\xBA\xD1\x80\xD0\xB8\xD0\xBC\xD0\xB8\xD0\xBD\xD0\xB0\xD0\xBB, \xD0\xB4\xD0\xB5\xD1\x82\xD0\xB5\xD0\xBA\xD1\x82\xD0\xB8\xD0\xB2";
                                            "is_special_project" = 0;
                                            "source_carousel_position" = "0";
                                            "release_year" = 2016;
                                            "onto_category" = "series";
                                            "rating_kp" = 6.96999979;
                                            "content_type_name" = "vod-episode";
                                            "can_play_on_station" = 1;
                                        };
                                    };
                                    {
                                        "metaforback" = {
                                            "duration" = 6436;
                                            "url" = "https://strm.yandex.ru/vod/vh-ottenc-converted/vod-content/4a8cb0bd88b0aea394c270954b6d84b5/9401751x1651149352x8cb52898-dbe7-4ae4-aa7b-b0483a0e8499/kaltura/dash_drm_sdr_hd_avc_ec3_4887f9394e06ea50dc0ee33947affe5ce629ed05c14950a9af8b52d424dedb3c/4a8cb0bd88b0aea394c270954b6d84b5/ysign1=62ec1c022041bd2c94bc08cfa055c8f6855f0bf685a8a65b7ea6f6d67fd30329,abcID=3386,from=ya-station,pfx,region=225,sfx,ts=62e254d0/manifest.mpd";
                                            "subscriptions" = [
                                                "YA_PLUS_SUPER";
                                                "YA_PREMIUM";
                                                "YA_PLUS_KP";
                                                "KP_BASIC";
                                                "YA_PLUS";
                                                "YA_PLUS_3M";
                                            ];
                                            "title" = "\xD0\x91\xD0\xBE\xD1\x81\xD1\x81-\xD0\xBC\xD0\xBE\xD0\xBB\xD0\xBE\xD0\xBA\xD0\xBE\xD1\x81\xD0\xBE\xD1\x81 2";
                                            "uuid" = "4a8cb0bd88b0aea394c270954b6d84b5";
                                        };
                                        "number" = 3;
                                        "active" = %true;
                                        "type" = "video";
                                        "title" = "\xD0\x91\xD0\xBE\xD1\x81\xD1\x81-\xD0\xBC\xD0\xBE\xD0\xBB\xD0\xBE\xD0\xBA\xD0\xBE\xD1\x81\xD0\xBE\xD1\x81 2";
                                        "metaforlog" = {
                                            "source_carousel_id" = "ChFoaG9nd3dra3h5dWF1c3RoaBIIc3RhdGlvbjIaDGVudGl0eV9taXhlZCABKAA=";
                                            "supertag_title" = "\xD0\x9C\xD1\x83\xD0\xBB\xD1\x8C\xD1\x82\xD1\x84\xD0\xB8\xD0\xBB\xD1\x8C\xD0\xBC";
                                            "source_carousel" = "recommendation";
                                            "countries" = "\xD0\xA1\xD0\xA8\xD0\x90";
                                            "is_promo" = 0;
                                            "supertag" = "kids";
                                            "onto_otype" = "Film/Film";
                                            "restriction_age" = 6;
                                            "genres" = "\xD1\x81\xD0\xB5\xD0\xBC\xD0\xB5\xD0\xB9\xD0\xBD\xD1\x8B\xD0\xB9, \xD0\xBF\xD1\x80\xD0\xB8\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x8F, \xD0\xBA\xD0\xBE\xD0\xBC\xD0\xB5\xD0\xB4\xD0\xB8\xD1\x8F, \xD0\xBC\xD1\x83\xD0\xBB\xD1\x8C\xD1\x82\xD1\x84\xD0\xB8\xD0\xBB\xD1\x8C\xD0\xBC";
                                            "views_count" = 2758705;
                                            "is_special_project" = 0;
                                            "source_carousel_position" = "0";
                                            "release_year" = 2021;
                                            "onto_category" = "anim_film";
                                            "rating_kp" = 7.260000229;
                                            "content_type_name" = "vod-episode";
                                            "can_play_on_station" = 0;
                                        };
                                    };
                                    {
                                        "metaforback" = {
                                            "duration" = 2171;
                                            "serial_id" = "47fe3cfb204e50f49e21fb220bed77e3";
                                            "url" = "https://strm.yandex.ru/vod/vh-ottenc-converted/vod-content/d25c776d9efb468e8d5decf797495364/9550321x1657711071x19d13514-6c9e-4796-98bb-08b554522f8e/kaltura/dash_drm_sdr_hd_avc_aac_055339cd4905cdaace782b613b45e9e3/d25c776d9efb468e8d5decf797495364/ysign1=270340f2bc5e25f23912cbf6fbe13f3c87ca24469ce9064de2deacb576ed1e11,abcID=3386,from=ya-station,pfx,region=225,sfx,ts=62e254d0/manifest.mpd";
                                            "subscriptions" = [
                                                "YA_PLUS_SUPER";
                                                "YA_PLUS";
                                                "YA_PREMIUM";
                                                "YA_PLUS_KP";
                                                "YA_PLUS_3M";
                                                "KP_BASIC";
                                            ];
                                            "title" = "\xD0\x9D\xD1\x83\xD0\xBB\xD0\xB5\xD0\xB2\xD0\xBE\xD0\xB9 \xD0\xBF\xD0\xB0\xD1\x86\xD0\xB8\xD0\xB5\xD0\xBD\xD1\x82 - \xD0\xA1\xD0\xB5\xD0\xB7\xD0\xBE\xD0\xBD 1 - \xD0\xA1\xD0\xB5\xD1\x80\xD0\xB8\xD1\x8F 8 - \xD0\xA4\xD0\xB8\xD0\xBB\xD1\x8C\xD0\xBC \xD0\xBE \xD1\x81\xD0\xB5\xD1\x80\xD0\xB8\xD0\xB0\xD0\xBB\xD0\xB5";
                                            "uuid" = "d25c776d9efb468e8d5decf797495364";
                                        };
                                        "number" = 4;
                                        "active" = %true;
                                        "type" = "video";
                                        "title" = "\xD0\x9D\xD1\x83\xD0\xBB\xD0\xB5\xD0\xB2\xD0\xBE\xD0\xB9 \xD0\xBF\xD0\xB0\xD1\x86\xD0\xB8\xD0\xB5\xD0\xBD\xD1\x82 - \xD0\xA1\xD0\xB5\xD0\xB7\xD0\xBE\xD0\xBD 1 - \xD0\xA1\xD0\xB5\xD1\x80\xD0\xB8\xD1\x8F 8 - \xD0\xA4\xD0\xB8\xD0\xBB\xD1\x8C\xD0\xBC \xD0\xBE \xD1\x81\xD0\xB5\xD1\x80\xD0\xB8\xD0\xB0\xD0\xBB\xD0\xB5";
                                        "metaforlog" = {
                                            "source_carousel_id" = "delayed_tvo";
                                            "source_carousel" = "delayed_view";
                                            "is_promo" = 0;
                                            "onto_otype" = "Film/Series@on";
                                            "restriction_age" = 18;
                                            "genres" = "\xD0\xB4\xD0\xB5\xD1\x82\xD0\xB5\xD0\xBA\xD1\x82\xD0\xB8\xD0\xB2, \xD0\xB4\xD1\x80\xD0\xB0\xD0\xBC\xD0\xB0";
                                            "views_count" = 785615;
                                            "is_special_project" = 0;
                                            "source_carousel_position" = "0";
                                            "release_year" = 2022;
                                            "onto_category" = "series";
                                            "rating_kp" = 8.380000114;
                                            "content_type_name" = "vod-episode";
                                            "can_play_on_station" = 0;
                                        };
                                    };
                                    {
                                        "metaforback" = {
                                            "duration" = 1344;
                                            "serial_id" = "46c5df252dc1a790b82d1a00fcf44812";
                                            "url" = "https://strm.yandex.ru/vod/vh-ottenc-converted/vod-content/4c7cb3aaaec6062f8866771032c0b4bb/9401751x1651176409x713b9ed2-6859-4c53-9dfb-e3d2e1009b8c/kaltura/dash_drm_sdr_hd_avc_aac_eb99f653654809d359c7af91a7a8a40addd6ad87a988fac526850630c78efd35/4c7cb3aaaec6062f8866771032c0b4bb/ysign1=808fce3a47a2c54337e983706e23753abdc15f490673b64ad3fbacc2c1e77f20,abcID=3386,from=ya-station,pfx,region=225,sfx,ts=62e254d0/manifest.mpd";
                                            "subscriptions" = [
                                                "YA_PLUS";
                                                "YA_PLUS_3M";
                                                "YA_PLUS_SUPER";
                                                "YA_PREMIUM";
                                                "YA_PLUS_KP";
                                                "KP_BASIC";
                                            ];
                                            "title" = "\xD0\xA0\xD0\xB8\xD0\xBA \xD0\xB8 \xD0\x9C\xD0\xBE\xD1\x80\xD1\x82\xD0\xB8 - \xD0\xA1\xD0\xB5\xD0\xB7\xD0\xBE\xD0\xBD 3 - \xD0\xA1\xD0\xB5\xD1\x80\xD0\xB8\xD1\x8F 1 - \xD0\xA0\xD0\xB8\xD0\xBA\xD0\xB1\xD0\xB5\xD0\xB3 \xD0\xB8\xD0\xB7 \xD0\xA0\xD0\xB8\xD0\xBA\xD1\x88\xD0\xB5\xD0\xBD\xD0\xBA\xD0\xB0";
                                            "uuid" = "4c7cb3aaaec6062f8866771032c0b4bb";
                                        };
                                        "number" = 5;
                                        "active" = %true;
                                        "type" = "video";
                                        "title" = "\xD0\xA0\xD0\xB8\xD0\xBA \xD0\xB8 \xD0\x9C\xD0\xBE\xD1\x80\xD1\x82\xD0\xB8 - \xD0\xA1\xD0\xB5\xD0\xB7\xD0\xBE\xD0\xBD 3 - \xD0\xA1\xD0\xB5\xD1\x80\xD0\xB8\xD1\x8F 1 - \xD0\xA0\xD0\xB8\xD0\xBA\xD0\xB1\xD0\xB5\xD0\xB3 \xD0\xB8\xD0\xB7 \xD0\xA0\xD0\xB8\xD0\xBA\xD1\x88\xD0\xB5\xD0\xBD\xD0\xBA\xD0\xB0";
                                        "metaforlog" = {
                                            "source_carousel_id" = "delayed_tvo";
                                            "source_carousel" = "delayed_view";
                                            "is_promo" = 0;
                                            "onto_otype" = "Film/Series@on";
                                            "restriction_age" = 18;
                                            "genres" = "\xD0\xBC\xD1\x83\xD0\xBB\xD1\x8C\xD1\x82\xD1\x84\xD0\xB8\xD0\xBB\xD1\x8C\xD0\xBC, \xD0\xBA\xD0\xBE\xD0\xBC\xD0\xB5\xD0\xB4\xD0\xB8\xD1\x8F, \xD1\x84\xD0\xB0\xD0\xBD\xD1\x82\xD0\xB0\xD1\x81\xD1\x82\xD0\xB8\xD0\xBA\xD0\xB0, \xD0\xBF\xD1\x80\xD0\xB8\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x8F";
                                            "views_count" = 1246096;
                                            "is_special_project" = 0;
                                            "source_carousel_position" = "0";
                                            "release_year" = 2013;
                                            "onto_category" = "anim_series";
                                            "rating_kp" = 9;
                                            "content_type_name" = "vod-episode";
                                            "can_play_on_station" = 1;
                                        };
                                    };
                                    {
                                        "metaforback" = {
                                            "serial_id" = "4c96c73c44bb5f079e347191f6818d69";
                                            "url" = "https://strm.yandex.ru/vod/vh-ottenc-converted/vod-content/42e8b40947b01ca69f2d8c405fe25146/9550321x1656228001x1dc85f30-fc7c-4f62-ad8a-8de97fe05a7d/kaltura/dash_drm_sdr_hd_avc_aac_3eb1fd877c90773208471c037d63d3cb/42e8b40947b01ca69f2d8c405fe25146/ysign1=0a899fec61c49b83fd2c39c385e56e486c5a7e5f04510ce88c46316e64c52925,abcID=3386,from=ya-station,pfx,region=225,sfx,ts=62e254d0/manifest.mpd";
                                            "season_id" = "478490667b8616d495051936112e83e7";
                                            "subscriptions" = [
                                                "YA_PLUS_SUPER";
                                                "YA_PLUS_3M";
                                                "YA_PLUS";
                                                "YA_PLUS_KP";
                                                "KP_BASIC";
                                                "YA_PREMIUM";
                                            ];
                                            "title" = "\xD0\x97\xD0\xB0 \xD0\xB6\xD0\xB8\xD0\xB7\xD0\xBD\xD1\x8C";
                                            "uuid" = "42e8b40947b01ca69f2d8c405fe25146";
                                        };
                                        "number" = 6;
                                        "active" = %true;
                                        "type" = "video";
                                        "title" = "\xD0\x97\xD0\xB0 \xD0\xB6\xD0\xB8\xD0\xB7\xD0\xBD\xD1\x8C";
                                        "metaforlog" = {
                                            "source_carousel_id" = "ChFoaG9nd3dra3h5dWF1c3RoaBIIc3RhdGlvbjIaDGVudGl0eV9taXhlZCABKAA=";
                                            "supertag_title" = "\xD0\xA1\xD0\xB5\xD1\x80\xD0\xB8\xD0\xB0\xD0\xBB";
                                            "source_carousel" = "recommendation";
                                            "is_promo" = 0;
                                            "supertag" = "series";
                                            "onto_otype" = "Film/Series@on";
                                            "restriction_age" = 16;
                                            "genres" = "\xD0\xB4\xD1\x80\xD0\xB0\xD0\xBC\xD0\xB0, \xD0\xBA\xD1\x80\xD0\xB8\xD0\xBC\xD0\xB8\xD0\xBD\xD0\xB0\xD0\xBB, \xD0\xB1\xD0\xB8\xD0\xBE\xD0\xB3\xD1\x80\xD0\xB0\xD1\x84\xD0\xB8\xD1\x8F";
                                            "is_special_project" = 0;
                                            "source_carousel_position" = "0";
                                            "release_year" = 2020;
                                            "onto_category" = "series";
                                            "rating_kp" = 8.010000229;
                                            "content_type_name" = "vod-episode";
                                            "can_play_on_station" = 1;
                                        };
                                    };
                                    {
                                        "metaforback" = {
                                            "serial_id" = "45739cf595e83bdfbc6debd703ad6555";
                                            "url" = "https://strm.yandex.ru/vod/vh-ottenc-converted/vod-content/49fb793cd2971fd7addaac8c43a558fc/9550321x1655666797x9275ac32-f616-4b92-898c-55aea9e4637c/kaltura/dash_drm_sdr_hd_avc_aac_bc16e0b3ed11cccf5ffe0c47a2301ccf/49fb793cd2971fd7addaac8c43a558fc/ysign1=5b64113ca7c66b62229c2a06a383860ffe5c751556605f549da2f5b54ffd16dc,abcID=3386,from=ya-station,pfx,region=225,sfx,ts=62e254d0/manifest.mpd";
                                            "season_id" = "4e3c067dbf208d849c390fba1eeafab3";
                                            "subscriptions" = [
                                                "YA_PLUS_KP";
                                                "YA_PLUS";
                                                "YA_PLUS_SUPER";
                                                "YA_PREMIUM";
                                                "YA_PLUS_3M";
                                                "KP_BASIC";
                                            ];
                                            "title" = "\xD0\xA2\xD0\xBE\xD0\xBB\xD1\x8C\xD0\xBA\xD0\xBE \xD0\xB4\xD0\xBB\xD1\x8F \xD0\xB2\xD0\xB7\xD1\x80\xD0\xBE\xD1\x81\xD0\xBB\xD1\x8B\xD1\x85";
                                            "uuid" = "49fb793cd2971fd7addaac8c43a558fc";
                                        };
                                        "number" = 7;
                                        "active" = %true;
                                        "type" = "video";
                                        "title" = "\xD0\xA2\xD0\xBE\xD0\xBB\xD1\x8C\xD0\xBA\xD0\xBE \xD0\xB4\xD0\xBB\xD1\x8F \xD0\xB2\xD0\xB7\xD1\x80\xD0\xBE\xD1\x81\xD0\xBB\xD1\x8B\xD1\x85";
                                        "metaforlog" = {
                                            "source_carousel_id" = "ChFoaG9nd3dra3h5dWF1c3RoaBIIc3RhdGlvbjIaDGVudGl0eV9taXhlZCABKAA=";
                                            "supertag_title" = "\xD0\x9A\xD0\xB8\xD0\xBD\xD0\xBE\xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA HD";
                                            "source_carousel" = "recommendation";
                                            "is_promo" = 0;
                                            "supertag" = "subscription";
                                            "onto_otype" = "Film/Series@on";
                                            "restriction_age" = 18;
                                            "genres" = "\xD0\xBA\xD0\xBE\xD0\xBC\xD0\xB5\xD0\xB4\xD0\xB8\xD1\x8F, \xD0\xBC\xD0\xB5\xD0\xBB\xD0\xBE\xD0\xB4\xD1\x80\xD0\xB0\xD0\xBC\xD0\xB0, \xD0\xB4\xD1\x80\xD0\xB0\xD0\xBC\xD0\xB0";
                                            "is_special_project" = 0;
                                            "source_carousel_position" = "0";
                                            "release_year" = 2020;
                                            "onto_category" = "series";
                                            "rating_kp" = 6.400000095;
                                            "content_type_name" = "vod-episode";
                                            "can_play_on_station" = 1;
                                        };
                                    };
                                ];
                                "id" = "main";
                            };
                        ];
                    };
                    "current_screen" = "mordovia_webview";
                    "last_play_timestamp" = 0;
                    "screen_state" = {
                        "view_key" = "VideoStationSPA:main";
                        "scenario" = "VideoStationSPA:main";
                    };
                };
                "clock_display_state" = {
                    "clock_enabled" = %true;
                };
                "sound_level" = 2;
                "is_tv_plugged_in" = %true;
                "alarm_state" = {
                    "icalendar" = "BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//Yandex LTD//NONSGML Quasar//EN\r\nEND:VCALENDAR\r\n";
                    "max_sound_level" = 7;
                    "currently_playing" = %false;
                };
                "multiroom" = {
                    "mode" = "Unknown";
                    "visible_peers" = [
                        "74005034440c0821058e";
                    ];
                };
                "alarms_state" = "BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//Yandex LTD//NONSGML Quasar//EN\r\nEND:VCALENDAR\r\n";
                "internet_connection" = {
                    "neighbours" = [
                        {
                            "bssid" = "52:ff:20:5e:4b:99";
                            "channel" = 4;
                            "ssid" = "y_ytvroom";
                        };
                        {
                            "bssid" = "52:ff:20:9e:4b:99";
                            "channel" = 4;
                            "ssid" = "";
                        };
                        {
                            "bssid" = "fa:8f:ca:55:e7:ee";
                            "channel" = 6;
                            "ssid" = "\xD0\x97\xD0\xB0\xD0\xB4\xD0\xBD\xD0\xB8\xD0\xB9 \xD0\xB4\xD0\xB2\xD0\xBE\xD1\x80.ynb,";
                        };
                        {
                            "bssid" = "04:d4:c4:c2:d7:88";
                            "channel" = 4;
                            "ssid" = "Vqe witnesses";
                        };
                        {
                            "bssid" = "50:ff:20:3e:4b:99";
                            "channel" = 36;
                            "ssid" = "y_ytvroom_5g";
                        };
                        {
                            "bssid" = "08:96:ad:e3:ac:f9";
                            "channel" = 64;
                            "ssid" = "MobDevInternet";
                        };
                        {
                            "bssid" = "08:96:ad:e3:ac:ff";
                            "channel" = 64;
                            "ssid" = "Yandex";
                        };
                        {
                            "bssid" = "08:96:ad:e3:ac:fb";
                            "channel" = 64;
                            "ssid" = "Guests";
                        };
                        {
                            "bssid" = "08:96:ad:e3:ac:fe";
                            "channel" = 64;
                            "ssid" = "PDAS";
                        };
                        {
                            "bssid" = "08:96:ad:e3:ac:fa";
                            "channel" = 64;
                            "ssid" = "MobCert";
                        };
                        {
                            "bssid" = "08:96:ad:e3:ac:f8";
                            "channel" = 64;
                            "ssid" = "MobTest";
                        };
                    ];
                    "type" = "Wifi_2_4GHz";
                    "current" = {
                        "bssid" = "52:ff:20:5e:4b:99";
                        "channel" = 4;
                        "ssid" = "y_ytvroom";
                    };
                };
                "mics_muted" = %false;
                "sound_muted" = %false;
                "sound_max_level" = 10;
                "active_actions" = {};
                "last_watched" = {};
                "bluetooth" = {
                    "player" = {
                        "pause" = %true;
                    };
                };
                "rcu" = {
                    "is_rcu_connected" = %false;
                };
                "device_config" = {
                    "content_settings" = "without";
                    "child_content_settings" = "safe";
                    "spotter" = "alisa";
                };
            };
            "Experiments" = {
                "Storage" = {
                    "biometry_remove" = {
                        "String" = "1";
                    };
                    "use_app_host_pure_Vins_scenario" = {
                        "String" = "1";
                    };
                    "allow_subtitles_and_audio_in_mordovia" = {
                        "String" = "1";
                    };
                    "podcasts" = {
                        "String" = "1";
                    };
                    "hw_voiceprint_enable_fallback_to_server_biometry" = {
                        "String" = "1";
                    };
                    "nlg_short_timer_cancel_exp" = {
                        "String" = "1";
                    };
                    "hw_enable_morning_show" = {
                        "String" = "1";
                    };
                    "hw_music_onboarding_tracks_reask_count=2" = {
                        "String" = "1";
                    };
                    "hw_music_thin_client_generative" = {
                        "String" = "1";
                    };
                    "music_show_first_track" = {
                        "String" = "1";
                    };
                    "enable_tts_gpu" = {
                        "String" = "1";
                    };
                    "video_new_seasons_path" = {
                        "String" = "1";
                    };
                    "iot" = {
                        "String" = "1";
                    };
                    "hw_music_thin_client_fairy_tale_playlists" = {
                        "String" = "1";
                    };
                    "hw_voiceprint_enable_remove" = {
                        "String" = "1";
                    };
                    "kv_saas_activation_experiment" = {
                        "String" = "1";
                    };
                    "change_alarm_sound_music" = {
                        "String" = "1";
                    };
                    "music_partials" = {
                        "String" = "1";
                    };
                    "hw_alarm_megamind_2906_fix" = {
                        "String" = "1";
                    };
                    "alarm_how_long" = {
                        "String" = "1";
                    };
                    "hw_music_thin_client_playlist" = {
                        "String" = "1";
                    };
                    "enable_reminders_todos" = {
                        "String" = "1";
                    };
                    "mm_enable_protocol_scenario=Settings" = {
                        "String" = "1";
                    };
                    "bg_beggins_voiceprint_what_is_my_name" = {
                        "String" = "1";
                    };
                    "bg_beggins_qr_code" = {
                        "String" = "1";
                    };
                    "change_track" = {
                        "String" = "1";
                    };
                    "new_fairytale_quasar" = {
                        "String" = "1";
                    };
                    "radio_play_onboarding" = {
                        "String" = "1";
                    };
                    "mm_enable_begemot_contacts" = {
                        "String" = "1";
                    };
                    "change_alarm_sound" = {
                        "String" = "1";
                    };
                    "mm_enable_protocol_scenario=MordoviaVideoSelection" = {
                        "String" = "1";
                    };
                    "use_coordinates_from_iot_in_laas_request" = {
                        "String" = "1";
                    };
                    "ether" = {
                        "String" = "https://yandex.ru/video/quasar/home/?audio_codec=AAC%2CAC3%2CEAC3%2CVORBIS%2COPUS&current_hdcp_level=None&dynamic_range=SDR%2CHDR10&video_codec=AVC%2CHEVC%2CVP9&video_format=SD%2CHD%2CUHD";
                    };
                    "hw_music_announce" = {
                        "String" = "1";
                    };
                    "change_alarm_sound_radio" = {
                        "String" = "1";
                    };
                    "drm_tv_stream" = {
                        "String" = "1";
                    };
                    "ignore_trash_classified_results" = {
                        "String" = "1";
                    };
                    "bg_fresh_granet_prefix=alice.generative_tale" = {
                        "String" = "1";
                    };
                    "video_webview_video_entity" = {
                        "String" = "1";
                    };
                    "taxi_nlu" = {
                        "String" = "1";
                    };
                    "ugc_enabled" = {
                        "String" = "1";
                    };
                    "uniproxy_vins_sessions" = {
                        "String" = "1";
                    };
                    "mm_enable_protocol_scenario=Goods" = {
                        "String" = "1";
                    };
                    "mordovia_films_gallery_splash" = {
                        "String" = "{\"card\":{\"log_id\":\"station_home_carousel\",\"states\":[{\"state_id\":0,\"div\":{\"type\":\"body\"}}]},\"templates\":{\"body\":{\"type\":\"container\",\"orientation\":\"vertical\",\"background\":[{\"color\":\"#151517\",\"type\":\"solid\"}],\"paddings\":{\"top\":140,\"left\":80},\"width\":{\"type\":\"match_parent\",\"value\":1920},\"height\":{\"type\":\"fixed\",\"value\":1080},\"items\":[{\"type\":\"cards\"}]},\"cards\":{\"type\":\"container\",\"orientation\":\"horizontal\",\"height\":{\"type\":\"fixed\",\"value\":614},\"margins\":{\"bottom\":245},\"items\":[{\"type\":\"card\"},{\"type\":\"card\"},{\"type\":\"card\"},{\"type\":\"card\"},{\"type\":\"card\"}]},\"card\":{\"type\":\"container\",\"margins\":{\"right\":40},\"width\":{\"type\":\"fixed\",\"value\":410},\"height\":{\"type\":\"match_parent\"},\"items\":[{\"type\":\"placeholder-image\"}]},\"placeholder-image\":{\"type\":\"placeholder\",\"border\":{\"corner_radius\":6},\"height\":{\"type\":\"match_parent\",\"value\":24}},\"placeholder\":{\"type\":\"separator\",\"delimiter_style\":{\"color\":\"#212123\"},\"background\":[{\"color\":\"#212123\",\"type\":\"solid\"}]}}}";
                    };
                    "hw_alarm_relocation_exp__alarm_show" = {
                        "String" = "1";
                    };
                    "cachalot_mm_context_save" = {
                        "String" = "1";
                    };
                    "enable_memento_reminders" = {
                        "String" = "1";
                    };
                    "fairytale_search_text_noprefix" = {
                        "String" = "1";
                    };
                    "market_beru_disable" = {
                        "String" = "1";
                    };
                    "general_conversation" = {
                        "String" = "1";
                    };
                    "hw_voiceprint_enable_bio_capability" = {
                        "String" = "1";
                    };
                    "hw_enable_children_morning_show" = {
                        "String" = "1";
                    };
                    "bg_alice_music_onboarding_tracks" = {
                        "String" = "1";
                    };
                    "hw_music_thin_client" = {
                        "String" = "1";
                    };
                    "tv" = {
                        "String" = "1";
                    };
                    "music_sing_song" = {
                        "String" = "1";
                    };
                    "tv_stream" = {
                        "String" = "1";
                    };
                    "use_memento" = {
                        "String" = "1";
                    };
                    "enable_biometry_scoring" = {
                        "String" = "1";
                    };
                    "hw_music_enable_prefetch_get_next_correction" = {
                        "String" = "1";
                    };
                    "bg_fresh_alice_prefix=alice.goods.best_prices_reask" = {
                        "String" = "1";
                    };
                    "username_auto_insert" = {
                        "String" = "1";
                    };
                    "alarm_semantic_frame" = {
                        "String" = "1";
                    };
                    "weather_precipitation_type" = {
                        "String" = "1";
                    };
                    "supress_multi_activation" = {
                        "String" = "1";
                    };
                    "weather_precipitation" = {
                        "String" = "1";
                    };
                    "enable_timers_alarms" = {
                        "String" = "1";
                    };
                    "quasar_gc_instead_of_search" = {
                        "String" = "1";
                    };
                    "hw_enable_alice_show_without_music" = {
                        "String" = "1";
                    };
                    "enable_tts_timings" = {
                        "String" = "1";
                    };
                    "new_music_radio_nlg" = {
                        "String" = "1";
                    };
                    "video_not_use_native_youtube_api" = {
                        "String" = "1";
                    };
                    "new_nlg" = {
                        "String" = "1";
                    };
                    "clock_face_control_turn_off" = {
                        "String" = "1";
                    };
                    "enable_full_rtlog" = {
                        "String" = "1";
                    };
                    "video_enable_telemetry" = {
                        "String" = "1";
                    };
                    "music_session" = {
                        "String" = "1";
                    };
                    "translate" = {
                        "String" = "1";
                    };
                    "hw_music_fairy_tales_enable_ondemand" = {
                        "String" = "1";
                    };
                    "bg_fresh_granet_form=alice.general_conversation.force_exit.ifexp.bg_enable_gc_force_exit" = {
                        "String" = "1";
                    };
                    "clock_face_control_unsupported_operation_nlg_response" = {
                        "String" = "1";
                    };
                    "hw_enable_phone_calls" = {
                        "String" = "1";
                    };
                    "personal_tv_channel" = {
                        "String" = "1";
                    };
                    "bg_enable_player_next_track_v2" = {
                        "String" = "1";
                    };
                    "bg_enable_generative_tale" = {
                        "String" = "1";
                    };
                    "bg_alice_music_onboarding" = {
                        "String" = "1";
                    };
                    "dialog_4178_newcards" = {
                        "String" = "1";
                    };
                    "kinopoisk_a3m_multi_exp" = {
                        "String" = "1";
                    };
                    "personalization" = {
                        "String" = "1";
                    };
                    "how_much" = {
                        "String" = "1";
                    };
                    "bg_fresh_granet_experiment=bg_enable_player_next_track_v2" = {
                        "String" = "1";
                    };
                    "weather_precipitation_starts_ends" = {
                        "String" = "1";
                    };
                    "use_trash_talk_classifier" = {
                        "String" = "1";
                    };
                    "use_app_host_pure_Video_scenario" = {
                        "String" = "1";
                    };
                    "market_disable" = {
                        "String" = "1";
                    };
                    "hw_music_what_album_is_this_song_from" = {
                        "String" = "1";
                    };
                    "mm_enable_player_features" = {
                        "String" = "1";
                    };
                    "radio_fixes" = {
                        "String" = "1";
                    };
                    "video_webview_video_entity_seasons" = {
                        "String" = "1";
                    };
                    "bg_exp_alice_show_day_part_and_age" = {
                        "String" = "1";
                    };
                    "bg_fresh_granet_prefix=alice.goods.best_prices" = {
                        "String" = "1";
                    };
                    "mordovia_main_splash" = {
                        "String" = "{\"card\":{\"log_id\":\"station_informers\",\"states\":[{\"state_id\":0,\"div\":{\"type\":\"body\"}}]},\"templates\":{\"body\":{\"type\":\"container\",\"orientation\":\"horizontal\",\"background\":[{\"color\":\"#151517\",\"type\":\"solid\"}],\"paddings\":{\"top\":140,\"left\":80},\"width\":{\"type\":\"match_parent\",\"value\":1920},\"height\":{\"type\":\"fixed\",\"value\":1080},\"items\":[{\"type\":\"container\",\"width\":{\"type\":\"fixed\",\"value\":1860},\"items\":[{\"type\":\"cards-big\"},{\"type\":\"cards-small\",\"margins\":{\"top\":40}}]}]},\"cards-big\":{\"type\":\"container\",\"orientation\":\"horizontal\",\"items\":[{\"type\":\"card-big\"},{\"type\":\"card-big\"},{\"type\":\"card-big\"}]},\"card-big\":{\"type\":\"container\",\"margins\":{\"right\":40},\"width\":{\"type\":\"fixed\",\"value\":560},\"height\":{\"type\":\"fixed\",\"value\":410},\"items\":[{\"type\":\"placeholder-image\",\"height\":{\"type\":\"fixed\",\"value\":315}},{\"type\":\"placeholder-text\",\"width\":{\"type\":\"fixed\",\"value\":500},\"height\":{\"type\":\"fixed\",\"value\":32},\"margins\":{\"top\":20}},{\"type\":\"placeholder-text\",\"width\":{\"type\":\"fixed\",\"value\":310},\"height\":{\"type\":\"fixed\",\"value\":28},\"margins\":{\"top\":14}}]},\"cards-small\":{\"type\":\"container\",\"orientation\":\"horizontal\",\"items\":[{\"type\":\"card\"},{\"type\":\"card\"},{\"type\":\"card\"},{\"type\":\"card\"}]},\"card\":{\"type\":\"container\",\"margins\":{\"right\":40},\"width\":{\"type\":\"fixed\",\"value\":410},\"height\":{\"type\":\"fixed\",\"value\":400},\"items\":[{\"type\":\"placeholder-image\",\"height\":{\"type\":\"fixed\",\"value\":234}},{\"type\":\"placeholder-text\",\"width\":{\"type\":\"fixed\",\"value\":286},\"height\":{\"type\":\"fixed\",\"value\":32},\"margins\":{\"top\":22}},{\"type\":\"placeholder-text\",\"width\":{\"type\":\"fixed\",\"value\":190},\"height\":{\"type\":\"fixed\",\"value\":28},\"margins\":{\"top\":10}}]},\"placeholder-image\":{\"type\":\"placeholder\",\"border\":{\"corner_radius\":6},\"height\":{\"type\":\"match_parent\"}},\"placeholder-text\":{\"type\":\"placeholder\",\"margins\":{\"top\":24},\"border\":{\"corner_radius\":4},\"height\":{\"type\":\"fixed\",\"value\":24}},\"placeholder\":{\"type\":\"separator\",\"delimiter_style\":{\"color\":\"#212123\"},\"background\":[{\"color\":\"#212123\",\"type\":\"solid\"}]}}}";
                    };
                    "radio_play_in_quasar" = {
                        "String" = "1";
                    };
                    "dj_service_for_games_onboarding" = {
                        "String" = "1";
                    };
                    "change_alarm_with_sound" = {
                        "String" = "1";
                    };
                    "hw_enrollment_directives" = {
                        "String" = "1";
                    };
                    "mm_enable_apphost_apply_scenarios" = {
                        "String" = "1";
                    };
                    "music_exp__dj_testid@613372" = {
                        "String" = "1";
                    };
                    "music" = {
                        "String" = "1";
                    };
                    "bg_fairy_tale_text_slot" = {
                        "String" = "1";
                    };
                    "hw_music_onboarding_tracks_reask" = {
                        "String" = "1";
                    };
                    "k_schastiyu_dlya_companii" = {
                        "String" = "1";
                    };
                    "hw_gc_enable_generative_tale" = {
                        "String" = "1";
                    };
                    "bg_time_capsule_v2" = {
                        "String" = "1";
                    };
                    "mm_enable_protocol_scenario=CecCommands" = {
                        "String" = "1";
                    };
                    "hw_enable_alice_pillow_show" = {
                        "String" = "1";
                    };
                    "hw_enable_morning_show_good_morning" = {
                        "String" = "1";
                    };
                    "hw_music_thin_client_radio" = {
                        "String" = "1";
                    };
                    "hw_music_onboarding" = {
                        "String" = "1";
                    };
                    "personal_tv_help" = {
                        "String" = "1";
                    };
                    "mm_proactivity_disable_answer" = {
                        "String" = "1";
                    };
                    "hw_music_what_year_is_this_song" = {
                        "String" = "1";
                    };
                    "tv_without_channel_status_check" = {
                        "String" = "1";
                    };
                    "debug_mode" = {
                        "String" = "1";
                    };
                    "mm_protocol_priority_scenario_early_win" = {
                        "String" = "1";
                    };
                    "pure_general_conversation" = {
                        "String" = "1";
                    };
                    "mm_disable_music" = {
                        "String" = "1";
                    };
                    "read_factoid_source" = {
                        "String" = "1";
                    };
                    "hw_enable_good_night_show" = {
                        "String" = "1";
                    };
                    "hw_music_thin_client_use_save_progress" = {
                        "String" = "1";
                    };
                    "mm_enable_protocol_scenario=VideoCommand" = {
                        "String" = "1";
                    };
                    "music_exp__dj_program@alice_reverse_experiment" = {
                        "String" = "1";
                    };
                    "quasar_tv" = {
                        "String" = "1";
                    };
                    "bg_alice_music_like" = {
                        "String" = "1";
                    };
                    "hw_disable_device_call_shortcut" = {
                        "String" = "1";
                    };
                    "mordovia" = {
                        "String" = "1";
                    };
                    "recurring_purchase" = {
                        "String" = "1";
                    };
                    "bg_beggins_zeliboba_qr_code" = {
                        "String" = "1";
                    };
                    "mm_music_fairy_tale_prefer_music_over_vins_on_smart_speakers" = {
                        "String" = "1";
                    };
                    "vins_music_play_force_irrelevant" = {
                        "String" = "1";
                    };
                    "fairytale_fallback" = {
                        "String" = "1";
                    };
                    "weather_tomorrow_significance_threshold=0.7" = {
                        "String" = "1";
                    };
                    "analytics_info" = {
                        "String" = "1";
                    };
                    "mm_enable_protocol_scenario=HollywoodHardcodedMusic" = {
                        "String" = "1";
                    };
                    "mm_enable_parallel_continue" = {
                        "String" = "1";
                    };
                    "clock_face_control_turn_on" = {
                        "String" = "1";
                    };
                    "hw_enable_good_night_show_by_time" = {
                        "String" = "1";
                    };
                    "tv_channels_webview" = {
                        "String" = "1";
                    };
                    "sleep_timers" = {
                        "String" = "1";
                    };
                    "mm_enable_logging_node" = {
                        "String" = "1";
                    };
                    "fm_radio_recommend" = {
                        "String" = "1";
                    };
                    "bg_fresh_alice_prefix=alice.music.announce" = {
                        "String" = "1";
                    };
                    "music_check_plus_promo" = {
                        "String" = "1";
                    };
                    "vins_e2e_partials" = {
                        "String" = "1";
                    };
                    "mm_enable_protocol_scenario=AliceShow" = {
                        "String" = "1";
                    };
                    "weather_tomorrow_forecast_warning" = {
                        "String" = "1";
                    };
                    "mm_enable_apphost_modifiers_in_apply" = {
                        "String" = "1";
                    };
                    "bg_fresh_granet_prefix=alice.time_capsule" = {
                        "String" = "1";
                    };
                    "taxi" = {
                        "String" = "1";
                    };
                    "video_pure_hw_content_details" = {
                        "String" = "1";
                    };
                    "hw_music_thin_client_fairy_tale_ondemand" = {
                        "String" = "1";
                    };
                    "music_recognizer" = {
                        "String" = "1";
                    };
                    "bg_fresh_alice_prefix=alice.messenger_call" = {
                        "String" = "1";
                    };
                    "show_video_settings" = {
                        "String" = "1";
                    };
                    "new_special_playlists" = {
                        "String" = "1";
                    };
                    "music_biometry" = {
                        "String" = "1";
                    };
                    "bg_exp_enable_clock_face_turn_on_granet" = {
                        "String" = "1";
                    };
                    "market_orders_status_disable" = {
                        "String" = "1";
                    };
                    "boost_down_vins_open_or_continue_on_video_screens" = {
                        "String" = "1";
                    };
                    "hw_music_onboarding_discovery_radio" = {
                        "String" = "1";
                    };
                    "enable_partials" = {
                        "String" = "1";
                    };
                    "skillrec_settings_dj" = {
                        "String" = "1";
                    };
                    "enable_outgoing_device_to_device_calls" = {
                        "String" = "1";
                    };
                    "ambient_sound" = {
                        "String" = "1";
                    };
                    "video_render_node" = {
                        "String" = "1";
                    };
                    "music_personalization" = {
                        "String" = "1";
                    };
                    "biometry_like" = {
                        "String" = "1";
                    };
                    "tts_domain_music" = {
                        "String" = "1";
                    };
                    "medium_ru_explicit_content" = {
                        "String" = "1";
                    };
                    "mm_enable_protocol_scenario=TimeCapsule" = {
                        "String" = "1";
                    };
                    "bg_exp_enable_clock_face_turn_off_granet" = {
                        "String" = "1";
                    };
                    "random_number_2node" = {
                        "String" = "1";
                    };
                    "film_gallery" = {
                        "String" = "1";
                    };
                    "hw_music_change_track_version" = {
                        "String" = "1";
                    };
                    "quasar" = {
                        "String" = "1";
                    };
                    "tv_vod_translation" = {
                        "String" = "1";
                    };
                    "bg_fresh_granet_form=alice.mordovia_video_selection" = {
                        "String" = "1";
                    };
                    "alarm_snooze" = {
                        "String" = "1";
                    };
                    "hw_music_onboarding_genre_radio" = {
                        "String" = "1";
                    };
                    "hw_enable_evening_show" = {
                        "String" = "1";
                    };
                    "mm_enable_protocol_scenario=LinkARemote" = {
                        "String" = "1";
                    };
                    "plus30_multi_exp" = {
                        "String" = "1";
                    };
                    "bg_enable_merger" = {
                        "String" = "1";
                    };
                    "disable_personal_tv_channel" = {
                        "String" = "1";
                    };
                    "bg_goods_best_prices" = {
                        "String" = "1";
                    };
                    "hw_music_onboarding_silence" = {
                        "String" = "1";
                    };
                    "bg_beggins_goods_best_prices_reask_web_ecom_500" = {
                        "String" = "1";
                    };
                    "hw_alarm_relocation_exp__alarm_set" = {
                        "String" = "1";
                    };
                    "mm_setup_begemot_beggins_request" = {
                        "String" = "1";
                    };
                    "enable_outgoing_device_calls" = {
                        "String" = "1";
                    };
                    "video_omit_youtube_restriction" = {
                        "String" = "1";
                    };
                    "shopping_list" = {
                        "String" = "1";
                    };
                    "music_use_websearch" = {
                        "String" = "1";
                    };
                    "quasar_biometry_limit_users" = {
                        "String" = "1";
                    };
                    "video_qproxy_players" = {
                        "String" = "1";
                    };
                    "mm_enable_session_reset" = {
                        "String" = "1";
                    };
                    "enable_ner_for_skills" = {
                        "String" = "1";
                    };
                    "skillrec_music_ts_postrolls" = {
                        "String" = "1";
                    };
                    "experiment_flag_for_uniproxy_correctness_alert" = {
                        "String" = "1";
                    };
                    "mm_disable_fairy_tale_preferred_vins_intent_on_smart_speakers" = {
                        "String" = "1";
                    };
                    "video_use_pure_hw" = {
                        "String" = "1";
                    };
                };
            };
        };
        "header" = {
            "prev_req_id" = "074f472d-9d5f-4c3d-96ef-d8a130c04c7b";
            "ref_message_id" = "47de475c-7630-478d-a95c-920d45e14fea";
            "request_id" = "47ddea05-c2d1-4631-a55c-f5c743639076";
            "sequence_number" = 20;
            "session_id" = "f4327977-c485-4e04-8256-77999455e63e";
        };
        "event_source" = {
            "type" = "Text";
            "source" = "Software";
            "event" = "Directive";
            "id" = "4442965d-52af-4eae-8f26-9b0a9a54cfc4";
        };
        "application" = {
            "device_manufacturer" = "Yandex";
            "lang" = "ru-RU";
            "platform" = "android";
            "device_model" = "Station_2";
            "quasmodrom_subgroup" = "production";
            "device_id" = "XK0000000000000241800000a04c979e";
            "uuid" = "67ea13474338cd0b345142319c0d2c71";
            "device_color" = "black";
            "app_version" = "1.0";
            "os_version" = "9";
            "client_time" = "20220718T123307";
            "device_revision" = "rev1";
            "quasmodrom_group" = "production";
            "timestamp" = "1658136787";
            "timezone" = "Europe/Moscow";
            "app_id" = "ru.yandex.quasar.app";
        };
        "event" = #;
    };
    "request_id" = "47ddea05-c2d1-4631-a55c-f5c743639076";
    "response" = {
        "version" = "trunk@9731612";
        "header" = {
            "response_id" = "9c5ae640-92c5dc07-ba7cbafe-4d7b7375";
            "request_id" = "47ddea05-c2d1-4631-a55c-f5c743639076";
            "ref_message_id" = "47de475c-7630-478d-a95c-920d45e14fea";
            "sequence_number" = 20;
            "dialog_id" = #;
            "session_id" = "f4327977-c485-4e04-8256-77999455e63e";
        };
        "response" = {
            "cards" = [
                {
                    "type" = "simple_text";
                    "text" = "\xD0\xA7\xD1\x82\xD0\xBE\xD0\xB1\xD1\x8B \xD0\xBD\xD0\xB0\xD1\x87\xD0\xB0\xD1\x82\xD1\x8C, \xD1\x81\xD0\xBA\xD0\xB0\xD0\xB6\xD0\xB8\xD1\x82\xD0\xB5: \xC2\xAB\xD0\x9C\xD0\xB5\xD0\xBD\xD1\x8F \xD0\xB7\xD0\xBE\xD0\xB2\xD1\x83\xD1\x82...\xC2\xBB \xE2\x80\x94 \xD0\xB8 \xD0\xB4\xD0\xBE\xD0\xB1\xD0\xB0\xD0\xB2\xD1\x8C\xD1\x82\xD0\xB5 \xD1\x81\xD0\xB2\xD0\xBE\xD1\x91 \xD0\xB8\xD0\xBC\xD1\x8F.";
                };
            ];
            "card" = {
                "type" = "simple_text";
                "text" = "\xD0\xA7\xD1\x82\xD0\xBE\xD0\xB1\xD1\x8B \xD0\xBD\xD0\xB0\xD1\x87\xD0\xB0\xD1\x82\xD1\x8C, \xD1\x81\xD0\xBA\xD0\xB0\xD0\xB6\xD0\xB8\xD1\x82\xD0\xB5: \xC2\xAB\xD0\x9C\xD0\xB5\xD0\xBD\xD1\x8F \xD0\xB7\xD0\xBE\xD0\xB2\xD1\x83\xD1\x82...\xC2\xBB \xE2\x80\x94 \xD0\xB8 \xD0\xB4\xD0\xBE\xD0\xB1\xD0\xB0\xD0\xB2\xD1\x8C\xD1\x82\xD0\xB5 \xD1\x81\xD0\xB2\xD0\xBE\xD1\x91 \xD0\xB8\xD0\xBC\xD1\x8F.";
            };
            "templates" = {};
            "experiments" = {
                "biometry_remove" = "1";
                "use_app_host_pure_Vins_scenario" = "1";
                "allow_subtitles_and_audio_in_mordovia" = "1";
                "podcasts" = "1";
                "hw_voiceprint_enable_fallback_to_server_biometry" = "1";
                "nlg_short_timer_cancel_exp" = "1";
                "hw_enable_morning_show" = "1";
                "hw_music_onboarding_tracks_reask_count=2" = "1";
                "hw_music_thin_client_generative" = "1";
                "music_show_first_track" = "1";
                "enable_tts_gpu" = "1";
                "video_new_seasons_path" = "1";
                "iot" = "1";
                "hw_music_thin_client_fairy_tale_playlists" = "1";
                "hw_voiceprint_enable_remove" = "1";
                "kv_saas_activation_experiment" = "1";
                "change_alarm_sound_music" = "1";
                "music_partials" = "1";
                "hw_alarm_megamind_2906_fix" = "1";
                "alarm_how_long" = "1";
                "hw_music_thin_client_playlist" = "1";
                "enable_reminders_todos" = "1";
                "mm_enable_protocol_scenario=Settings" = "1";
                "bg_beggins_voiceprint_what_is_my_name" = "1";
                "bg_beggins_qr_code" = "1";
                "change_track" = "1";
                "new_fairytale_quasar" = "1";
                "radio_play_onboarding" = "1";
                "mm_enable_begemot_contacts" = "1";
                "change_alarm_sound" = "1";
                "mm_enable_protocol_scenario=MordoviaVideoSelection" = "1";
                "use_coordinates_from_iot_in_laas_request" = "1";
                "ether" = "https://yandex.ru/video/quasar/home/?audio_codec=AAC%2CAC3%2CEAC3%2CVORBIS%2COPUS&current_hdcp_level=None&dynamic_range=SDR%2CHDR10&video_codec=AVC%2CHEVC%2CVP9&video_format=SD%2CHD%2CUHD";
                "hw_music_announce" = "1";
                "change_alarm_sound_radio" = "1";
                "drm_tv_stream" = "1";
                "ignore_trash_classified_results" = "1";
                "bg_fresh_granet_prefix=alice.generative_tale" = "1";
                "video_webview_video_entity" = "1";
                "taxi_nlu" = "1";
                "ugc_enabled" = "1";
                "uniproxy_vins_sessions" = "1";
                "mm_enable_protocol_scenario=Goods" = "1";
                "mordovia_films_gallery_splash" = "{\"card\":{\"log_id\":\"station_home_carousel\",\"states\":[{\"state_id\":0,\"div\":{\"type\":\"body\"}}]},\"templates\":{\"body\":{\"type\":\"container\",\"orientation\":\"vertical\",\"background\":[{\"color\":\"#151517\",\"type\":\"solid\"}],\"paddings\":{\"top\":140,\"left\":80},\"width\":{\"type\":\"match_parent\",\"value\":1920},\"height\":{\"type\":\"fixed\",\"value\":1080},\"items\":[{\"type\":\"cards\"}]},\"cards\":{\"type\":\"container\",\"orientation\":\"horizontal\",\"height\":{\"type\":\"fixed\",\"value\":614},\"margins\":{\"bottom\":245},\"items\":[{\"type\":\"card\"},{\"type\":\"card\"},{\"type\":\"card\"},{\"type\":\"card\"},{\"type\":\"card\"}]},\"card\":{\"type\":\"container\",\"margins\":{\"right\":40},\"width\":{\"type\":\"fixed\",\"value\":410},\"height\":{\"type\":\"match_parent\"},\"items\":[{\"type\":\"placeholder-image\"}]},\"placeholder-image\":{\"type\":\"placeholder\",\"border\":{\"corner_radius\":6},\"height\":{\"type\":\"match_parent\",\"value\":24}},\"placeholder\":{\"type\":\"separator\",\"delimiter_style\":{\"color\":\"#212123\"},\"background\":[{\"color\":\"#212123\",\"type\":\"solid\"}]}}}";
                "hw_alarm_relocation_exp__alarm_show" = "1";
                "cachalot_mm_context_save" = "1";
                "enable_memento_reminders" = "1";
                "fairytale_search_text_noprefix" = "1";
                "market_beru_disable" = "1";
                "general_conversation" = "1";
                "hw_voiceprint_enable_bio_capability" = "1";
                "hw_enable_children_morning_show" = "1";
                "bg_alice_music_onboarding_tracks" = "1";
                "hw_music_thin_client" = "1";
                "tv" = "1";
                "music_sing_song" = "1";
                "tv_stream" = "1";
                "use_memento" = "1";
                "enable_biometry_scoring" = "1";
                "hw_music_enable_prefetch_get_next_correction" = "1";
                "bg_fresh_alice_prefix=alice.goods.best_prices_reask" = "1";
                "username_auto_insert" = "1";
                "alarm_semantic_frame" = "1";
                "weather_precipitation_type" = "1";
                "supress_multi_activation" = "1";
                "weather_precipitation" = "1";
                "enable_timers_alarms" = "1";
                "quasar_gc_instead_of_search" = "1";
                "hw_enable_alice_show_without_music" = "1";
                "enable_tts_timings" = "1";
                "new_music_radio_nlg" = "1";
                "video_not_use_native_youtube_api" = "1";
                "new_nlg" = "1";
                "clock_face_control_turn_off" = "1";
                "enable_full_rtlog" = "1";
                "video_enable_telemetry" = "1";
                "music_session" = "1";
                "translate" = "1";
                "hw_music_fairy_tales_enable_ondemand" = "1";
                "bg_fresh_granet_form=alice.general_conversation.force_exit.ifexp.bg_enable_gc_force_exit" = "1";
                "clock_face_control_unsupported_operation_nlg_response" = "1";
                "hw_enable_phone_calls" = "1";
                "personal_tv_channel" = "1";
                "bg_enable_player_next_track_v2" = "1";
                "bg_enable_generative_tale" = "1";
                "bg_alice_music_onboarding" = "1";
                "dialog_4178_newcards" = "1";
                "kinopoisk_a3m_multi_exp" = "1";
                "personalization" = "1";
                "how_much" = "1";
                "bg_fresh_granet_experiment=bg_enable_player_next_track_v2" = "1";
                "weather_precipitation_starts_ends" = "1";
                "use_trash_talk_classifier" = "1";
                "use_app_host_pure_Video_scenario" = "1";
                "market_disable" = "1";
                "hw_music_what_album_is_this_song_from" = "1";
                "mm_enable_player_features" = "1";
                "radio_fixes" = "1";
                "video_webview_video_entity_seasons" = "1";
                "bg_exp_alice_show_day_part_and_age" = "1";
                "bg_fresh_granet_prefix=alice.goods.best_prices" = "1";
                "mordovia_main_splash" = "{\"card\":{\"log_id\":\"station_informers\",\"states\":[{\"state_id\":0,\"div\":{\"type\":\"body\"}}]},\"templates\":{\"body\":{\"type\":\"container\",\"orientation\":\"horizontal\",\"background\":[{\"color\":\"#151517\",\"type\":\"solid\"}],\"paddings\":{\"top\":140,\"left\":80},\"width\":{\"type\":\"match_parent\",\"value\":1920},\"height\":{\"type\":\"fixed\",\"value\":1080},\"items\":[{\"type\":\"container\",\"width\":{\"type\":\"fixed\",\"value\":1860},\"items\":[{\"type\":\"cards-big\"},{\"type\":\"cards-small\",\"margins\":{\"top\":40}}]}]},\"cards-big\":{\"type\":\"container\",\"orientation\":\"horizontal\",\"items\":[{\"type\":\"card-big\"},{\"type\":\"card-big\"},{\"type\":\"card-big\"}]},\"card-big\":{\"type\":\"container\",\"margins\":{\"right\":40},\"width\":{\"type\":\"fixed\",\"value\":560},\"height\":{\"type\":\"fixed\",\"value\":410},\"items\":[{\"type\":\"placeholder-image\",\"height\":{\"type\":\"fixed\",\"value\":315}},{\"type\":\"placeholder-text\",\"width\":{\"type\":\"fixed\",\"value\":500},\"height\":{\"type\":\"fixed\",\"value\":32},\"margins\":{\"top\":20}},{\"type\":\"placeholder-text\",\"width\":{\"type\":\"fixed\",\"value\":310},\"height\":{\"type\":\"fixed\",\"value\":28},\"margins\":{\"top\":14}}]},\"cards-small\":{\"type\":\"container\",\"orientation\":\"horizontal\",\"items\":[{\"type\":\"card\"},{\"type\":\"card\"},{\"type\":\"card\"},{\"type\":\"card\"}]},\"card\":{\"type\":\"container\",\"margins\":{\"right\":40},\"width\":{\"type\":\"fixed\",\"value\":410},\"height\":{\"type\":\"fixed\",\"value\":400},\"items\":[{\"type\":\"placeholder-image\",\"height\":{\"type\":\"fixed\",\"value\":234}},{\"type\":\"placeholder-text\",\"width\":{\"type\":\"fixed\",\"value\":286},\"height\":{\"type\":\"fixed\",\"value\":32},\"margins\":{\"top\":22}},{\"type\":\"placeholder-text\",\"width\":{\"type\":\"fixed\",\"value\":190},\"height\":{\"type\":\"fixed\",\"value\":28},\"margins\":{\"top\":10}}]},\"placeholder-image\":{\"type\":\"placeholder\",\"border\":{\"corner_radius\":6},\"height\":{\"type\":\"match_parent\"}},\"placeholder-text\":{\"type\":\"placeholder\",\"margins\":{\"top\":24},\"border\":{\"corner_radius\":4},\"height\":{\"type\":\"fixed\",\"value\":24}},\"placeholder\":{\"type\":\"separator\",\"delimiter_style\":{\"color\":\"#212123\"},\"background\":[{\"color\":\"#212123\",\"type\":\"solid\"}]}}}";
                "radio_play_in_quasar" = "1";
                "dj_service_for_games_onboarding" = "1";
                "change_alarm_with_sound" = "1";
                "hw_enrollment_directives" = "1";
                "mm_enable_apphost_apply_scenarios" = "1";
                "music_exp__dj_testid@613372" = "1";
                "music" = "1";
                "bg_fairy_tale_text_slot" = "1";
                "hw_music_onboarding_tracks_reask" = "1";
                "k_schastiyu_dlya_companii" = "1";
                "hw_gc_enable_generative_tale" = "1";
                "bg_time_capsule_v2" = "1";
                "mm_enable_protocol_scenario=CecCommands" = "1";
                "hw_enable_alice_pillow_show" = "1";
                "hw_enable_morning_show_good_morning" = "1";
                "hw_music_thin_client_radio" = "1";
                "hw_music_onboarding" = "1";
                "personal_tv_help" = "1";
                "mm_proactivity_disable_answer" = "1";
                "hw_music_what_year_is_this_song" = "1";
                "tv_without_channel_status_check" = "1";
                "debug_mode" = "1";
                "mm_protocol_priority_scenario_early_win" = "1";
                "pure_general_conversation" = "1";
                "mm_disable_music" = "1";
                "read_factoid_source" = "1";
                "hw_enable_good_night_show" = "1";
                "hw_music_thin_client_use_save_progress" = "1";
                "mm_enable_protocol_scenario=VideoCommand" = "1";
                "music_exp__dj_program@alice_reverse_experiment" = "1";
                "quasar_tv" = "1";
                "bg_alice_music_like" = "1";
                "hw_disable_device_call_shortcut" = "1";
                "mordovia" = "1";
                "recurring_purchase" = "1";
                "bg_beggins_zeliboba_qr_code" = "1";
                "mm_music_fairy_tale_prefer_music_over_vins_on_smart_speakers" = "1";
                "vins_music_play_force_irrelevant" = "1";
                "fairytale_fallback" = "1";
                "weather_tomorrow_significance_threshold=0.7" = "1";
                "analytics_info" = "1";
                "mm_enable_protocol_scenario=HollywoodHardcodedMusic" = "1";
                "mm_enable_parallel_continue" = "1";
                "clock_face_control_turn_on" = "1";
                "hw_enable_good_night_show_by_time" = "1";
                "tv_channels_webview" = "1";
                "sleep_timers" = "1";
                "mm_enable_logging_node" = "1";
                "fm_radio_recommend" = "1";
                "bg_fresh_alice_prefix=alice.music.announce" = "1";
                "music_check_plus_promo" = "1";
                "vins_e2e_partials" = "1";
                "mm_enable_protocol_scenario=AliceShow" = "1";
                "weather_tomorrow_forecast_warning" = "1";
                "mm_enable_apphost_modifiers_in_apply" = "1";
                "bg_fresh_granet_prefix=alice.time_capsule" = "1";
                "taxi" = "1";
                "video_pure_hw_content_details" = "1";
                "hw_music_thin_client_fairy_tale_ondemand" = "1";
                "music_recognizer" = "1";
                "bg_fresh_alice_prefix=alice.messenger_call" = "1";
                "show_video_settings" = "1";
                "new_special_playlists" = "1";
                "music_biometry" = "1";
                "bg_exp_enable_clock_face_turn_on_granet" = "1";
                "market_orders_status_disable" = "1";
                "boost_down_vins_open_or_continue_on_video_screens" = "1";
                "hw_music_onboarding_discovery_radio" = "1";
                "enable_partials" = "1";
                "skillrec_settings_dj" = "1";
                "enable_outgoing_device_to_device_calls" = "1";
                "ambient_sound" = "1";
                "video_render_node" = "1";
                "music_personalization" = "1";
                "biometry_like" = "1";
                "tts_domain_music" = "1";
                "medium_ru_explicit_content" = "1";
                "mm_enable_protocol_scenario=TimeCapsule" = "1";
                "bg_exp_enable_clock_face_turn_off_granet" = "1";
                "random_number_2node" = "1";
                "film_gallery" = "1";
                "hw_music_change_track_version" = "1";
                "quasar" = "1";
                "tv_vod_translation" = "1";
                "bg_fresh_granet_form=alice.mordovia_video_selection" = "1";
                "alarm_snooze" = "1";
                "hw_music_onboarding_genre_radio" = "1";
                "hw_enable_evening_show" = "1";
                "mm_enable_protocol_scenario=LinkARemote" = "1";
                "plus30_multi_exp" = "1";
                "bg_enable_merger" = "1";
                "disable_personal_tv_channel" = "1";
                "bg_goods_best_prices" = "1";
                "hw_music_onboarding_silence" = "1";
                "bg_beggins_goods_best_prices_reask_web_ecom_500" = "1";
                "hw_alarm_relocation_exp__alarm_set" = "1";
                "mm_setup_begemot_beggins_request" = "1";
                "enable_outgoing_device_calls" = "1";
                "video_omit_youtube_restriction" = "1";
                "music_use_websearch" = "1";
                "shopping_list" = "1";
                "quasar_biometry_limit_users" = "1";
                "video_qproxy_players" = "1";
                "mm_enable_session_reset" = "1";
                "enable_ner_for_skills" = "1";
                "experiment_flag_for_uniproxy_correctness_alert" = "1";
                "skillrec_music_ts_postrolls" = "1";
                "video_use_pure_hw" = "1";
                "mm_disable_fairy_tale_preferred_vins_intent_on_smart_speakers" = "1";
            };
            "directives" = [
                {
                    "name" = "player_pause";
                    "payload" = {
                        "smooth" = %false;
                    };
                    "type" = "client_action";
                    "sub_name" = "voiceprint_player_pause";
                };
            ];
            "directives_execution_policy" = "BeforeSpeech";
        };
        "voice_response" = {
            "output_speech" = {
                "type" = "simple";
                "text" = "\xD0\xA7\xD1\x82\xD0\xBE\xD0\xB1\xD1\x8B \xD0\xBD\xD0\xB0\xD1\x87\xD0\xB0\xD1\x82\xD1\x8C, \xD1\x81\xD0\xBA\xD0\xB0\xD0\xB6\xD0\xB8\xD1\x82\xD0\xB5: \xC2\xAB\xD0\x9C\xD0\xB5\xD0\xBD\xD1\x8F \xD0\xB7\xD0\xBE\xD0\xB2\xD1\x83\xD1\x82...\xC2\xBB \xE2\x80\x94 \xD0\xB8 \xD0\xB4\xD0\xBE\xD0\xB1\xD0\xB0\xD0\xB2\xD1\x8C\xD1\x82\xD0\xB5 \xD1\x81\xD0\xB2\xD0\xBE\xD1\x91 \xD0\xB8\xD0\xBC\xD1\x8F.";
            };
            "should_listen" = %true;
        };
    };
    "rest" = {};
    "sequence_number" = 20u;
    "server_time_us" = 1658136787348815u;
    "server_version" = "trunk@9731612";
    "source_uri" = "prt://megamind@2a02:6b8:c14:440a:10b:3def:8870:0;unknown_path";
    "uuid" = "67ea13474338cd0b345142319c0d2c71";
})-";

}

namespace NAlice::NWonderlogs {

Y_UNIT_TEST_SUITE(Megamind) {
    Y_UNIT_TEST(ParseUuid) {
        NYT::TNode row;
        row["uuid"] = "123e4567-e89b-12d3-a456-426614174000";
        const auto actual = TMegamindLogsParser::ParseUuid(row);
        UNIT_ASSERT(actual);
        UNIT_ASSERT_EQUAL("123e4567e89b12d3a456426614174000", *actual);
    }

    Y_UNIT_TEST(InvalidRow) {
        const auto row = NYT::NodeFromYsonString(MEGAMIND_ANALYTICS_LOG);
        TMegamindLogsParser logsParser(row);
        const auto parsedLogs = logsParser.Parse();
        UNIT_ASSERT(parsedLogs.Errors.empty());
    }
}

} // namespace NAlice::NWonderlogs
