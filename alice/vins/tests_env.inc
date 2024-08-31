PEERDIR(
    alice/vins_contrib/pytest-mongodb
    alice/vins_contrib/pytest-randomly
)

DATA(arcadia/alice/vins/api)

TEST_CWD(alice/vins)

ENV(PYTEST_ADDOPTS=--randomly-seed=1 --mongodb-fixture-dir=./api/)

ENV(LANG=en_US.UTF-8)
ENV(LANGUAGE=en_US:en)
ENV(LC_ALL=en_US.UTF-8)
ENV(IS_PYTEST=yes)
ENV(KERAS_BACKEND=tensorflow)
ENV(VINS_BASS_TIMEOUT=10)
ENV(VINS_WIZARD_TIMEOUT=1)
ENV(VINS_S3_ACCESS_KEY_ID=test)
ENV(VINS_S3_SECRET_ACCESS_KEY=test)
ENV(VINS_S3_ENDPOINT_URL=https://s3.us-east-2.amazonaws.com/)
ENV(VINS_DISABLE_SENTRY=1)
ENV(VINS_UNIQUE_TMPDIR=1)
