LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    converter.cpp
)

PEERDIR(
    alice/cuttlefish/library/protos

    library/cpp/json
    library/cpp/resource
)

RESOURCE(
    alice/cuttlefish/library/cuttlefish/stream_converter/rms_converter/per_device_model_rms_correction_coefficients.json /per_device_model_rms_correction_coefficients.json
)

END()

RECURSE_FOR_TESTS(ut)
