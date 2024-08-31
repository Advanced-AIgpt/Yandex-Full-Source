export function getS3Asset(path: string) {
    return `https://dialogs.s3.yandex.net/smart_displays/${path}`;
}

export function getStaticS3Asset(path: string) {
    return `https://yastatic.net/s3/dialogs/smart_displays/${path}`;
}
