import { IVanillaBlocksProps } from '@yandex-lego/serp-header/User2';
import { IUserProps } from '@yandex-lego/serp-header/YandexHeaderServer';

declare module '@yandex-lego/serp-header/dist/base/user2.desktop' {
    export function getContent(props: {
        tld: IVanillaBlocksProps['tld'];
        lang: IVanillaBlocksProps['lang'];
        key: IVanillaBlocksProps['key'];
        ctx: IUserProps;
    }): string;
}
