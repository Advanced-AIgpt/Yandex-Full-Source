#pragma once

#include <util/generic/string.h>

namespace NAlice::NCuttlefish::NAws {

// Standard env variable names for aws cli
static const TString AWS_ACCESS_KEY_ID_ENV_NAME = "AWS_ACCESS_KEY_ID";
static const TString AWS_SECRET_ACCESS_KEY_ENV_NAME = "AWS_SECRET_ACCESS_KEY";


// S3
static const TString S3_SERVICE_NAME = "s3";
// https://wiki.yandex-team.ru/mds/s3-api/s3-clients/#primery
static const TString S3_YANDEX_INTERNAL_REGION = "us-east-1";
static const TString S3_YANDEX_INTERNAL_HOST = "s3.mds.yandex.net";

} // namespace NAlice::NCuttlefish::NAws
