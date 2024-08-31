### Залить рендерер в S3
```bash
$ ya make && ./upload --key_id KEY_ID --access_key ACCESS_KEY [--endpoint_url ENDPOINT_URL] [--bucket_name BUCKET_NAME] --path PATH [--prefix PREFIX]
```
Arguments:
* KEY_ID - S3 AccessKeyId
* ACCESS_KEY - S3 AccessSecretKey
* ENDPOINT_URL - **optional** - S3 EndpointUrl `default: http://s3.mds.yandex.net/` 
* BUCKET_NAME  - **optional** - S3 BucketName `default: div2html` 
* PATH - Path to folder where sources located - all files in folder will be uploaded
* PREFIX - **optional** - Add prefix to uploaded files, there is no prefix by default
