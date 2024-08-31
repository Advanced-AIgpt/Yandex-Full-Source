import { NAlice } from '../../../../protos';

export function ChooseAvatarsMdsImage(image: (NAlice.ITAvatarMdsImage|null|undefined)): string {
    if (image == null || image.BaseUrl?.length === 0 || image.Sizes?.length === 0) {
        return '';
    }
    const baseUrl = image.BaseUrl;
    if (baseUrl == null || baseUrl.length === 0) {
        return '';
    }
    const sizes = image.Sizes;
    if (sizes == null || sizes.length === 0 || sizes[0].length === 0) {
        return baseUrl + 'orig';
    }

    return baseUrl + sizes[0];
}

export function GetOrDefault<T>(value: (T|null|undefined), defaultValue: T): T {
    if (value == null) {
        return defaultValue;
    }
    return value;
}
