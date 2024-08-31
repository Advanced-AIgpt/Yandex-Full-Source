import { Avatar } from '../../../../common/helpers/avatar';

export function getImageCoverByLink(link: string, type: string) {
    const avatar = Avatar.fromUrl(link);
    return avatar ? avatar.setTypeName(type).toString() : link;
}
// avatars.yandex.net/get-music-content/5708920/3a11feb7.a.20128394-1/700x700
