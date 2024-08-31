import { DivCustomBlock } from 'divcard2';

interface IShortVideoProps {
    url: string;
    isLooped: boolean;
    options?: Omit<ConstructorParameters<typeof DivCustomBlock>[0], 'custom_type' | 'custom_props'>;
}

export default function ShortVideo({ url, isLooped, options = {} }: IShortVideoProps) {
    return new DivCustomBlock({
        ...options,
        custom_type: 'short_video',
        custom_props: {
            url,
            is_looped: isLooped,
        },
    });
}
