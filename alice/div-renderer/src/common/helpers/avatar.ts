type Protocol = 'http' | 'https';

interface AvatarData {
    readonly namespace: string;
    readonly domain?: string;
    readonly groupId: string;
    readonly imageId: string;
    readonly typeName: string;
    readonly parameters?: string;
    readonly protocol?: Protocol;
}

type AvatarNamespace = 'get-entity_search' | 'i' | 'get-altay';
type AvatarSize = 'small' | 'smallSquare' | 'mediumRectangle' | 'largeSquare' | 'largeRectangle';
type AvatarType = 'Face';

const entitySearchSizeMap: { [key in AvatarSize]?: string } = {
    small: '134x178', // 0.7582
    smallSquare: '80x80', // 1
    largeSquare: '156x156_2x', // 1
    largeRectangle: '284x160', // 1.75
};

const altaySizeMap: { [key in AvatarSize]?: string } = {
    mediumRectangle: 'h220',
    largeRectangle: 'h336',
};

/**
 * Class for working with links to the avatar<br>
 * {@link https://wiki.yandex-team.ru/mds/avatars/ Avatars wiki}<br>
 * You can see type of images in {@link https://mds.yandex-team.ru/avatars/?page=1 namespace}
 * @example ```
 * Avatar
 *  .fromUrl('https://avatars.yandex.net/get-music-content/2110367/c60de1ac.a.10261576-2/200x200')
 *  ?.setTypeName('zen_scale_1200').toString()
 * ```
 */
export class Avatar {
    public readonly namespace: string;
    public readonly domain: string;
    public readonly groupId: string;
    public readonly imageId: string;
    private typeName: string;
    private readonly parameters: string;
    private protocol: Protocol | undefined;

    constructor(options: AvatarData) {
        this.domain = options.domain || 'avatars.yandex.net';
        this.groupId = options.groupId;
        this.namespace = options.namespace;
        this.imageId = options.imageId;
        this.typeName = options.typeName;
        this.parameters = options.parameters || '';
        this.protocol = options.protocol || 'https';
    }

    /**
     * Change type name to new name. If a namespace is specified, the change will only apply if the namespace matches.
     * @param typeName
     * @param namespace
     */
    public setTypeName(typeName: string, namespace?: string) {
        if (namespace && namespace !== this.namespace) {
            return this;
        }
        this.typeName = typeName;
        return this;
    }

    public setProtocol(protocol: Protocol) {
        this.protocol = protocol;
        return this;
    }

    public toString() {
        return `${this.protocol}://${this.domain}/get-${this.namespace}/${this.groupId}/${this.imageId}/${this.typeName}${this.parameters}`;
    }

    /**
     *  Получить объект ссылки аватарницы по урле.
     *  Если ссылка не из аватарницы - вернет `null`
     * @param url
     * @returns {Avatar | null}
     */
    public static fromUrl(url: string): Avatar | null {
        const parsed = Avatar.parseUrl(url);
        if (!parsed) {
            return null;
        }
        return new Avatar(parsed);
    }

    private static parseUrl(url: string) {
        const match = url.match(/^(((https?):\/\/)?(avatars(-int)?(.mdst?)?.yandex.net))\/get-([^/]+)\/([^/]+)\/([^/]+)\/([^/]*?)(\?.*)?$/);
        if (!match) {
            return null;
        }
        const [, , , protocol, domain, , , namespace, groupId, imageId, typeName = '', parameters = ''] = match;
        return {
            domain,
            namespace,
            groupId,
            imageId,
            typeName,
            parameters,
            protocol: protocol as (Protocol | undefined),
        };
    }

    public static setImageSize({
        data,
        size,
        type = '',
        namespace,
    }: {
        data: string;
        size: AvatarSize;
        type?: AvatarType | '';
        namespace?: AvatarNamespace;
    }) {
        const url = new URL(data);
        // если неймспейс не передан, пытаемся вытащить его из урлы
        const avatarNamespace = namespace ?? url.pathname.split('/')[1];

        switch (avatarNamespace) {
            case 'get-entity_search': {
                url.pathname = url.pathname
                    .split('/')
                    .map((item, index, arr) => {
                        const isLastItem = index === arr.length - 1;
                        const sizeValue = entitySearchSizeMap[size];

                        return isLastItem ? `S${sizeValue}${type}` : item;
                    })
                    .join('/');

                break;
            }
            case 'i': {
                if (size === 'largeRectangle') {
                    url.searchParams.append('n', '33');
                    url.searchParams.append('h', '344');
                }

                break;
            }
            case 'get-altay': {
                url.pathname = url.pathname
                    .split('/')
                    .map((item, index, arr) => {
                        const isLastItem = index === arr.length - 1;
                        const sizeValue = altaySizeMap[size] ?? item;

                        return isLastItem ? sizeValue : item;
                    })
                    .join('/');

                break;
            }
        }

        return url.toString();
    }
}
