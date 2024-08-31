import { ContainerBlock, Div, FixedSize, ImageBlock, MatchParentSize } from 'divcard2';

interface IPartImageProps {
    imageUrl: string;
    imageOptions?: Partial<ConstructorParameters<typeof ImageBlock>[0]>;
    size?: number;
}

export default function PartImage({
    imageUrl,
    imageOptions,
    size,
}: IPartImageProps): Div[] {
    const imageDiv = new ImageBlock({
        ...imageOptions,
        width: size ? new FixedSize({ value: size }) : new MatchParentSize({ weight: 1 }),
        height: size ? new FixedSize({ value: size }) : new MatchParentSize({ weight: 1 }),
        image_url: imageUrl,
    });

    if (size) {
        return [new ContainerBlock({
            width: new MatchParentSize({ weight: 1 }),
            height: new MatchParentSize({ weight: 1 }),
            content_alignment_vertical: 'center',
            content_alignment_horizontal: 'center',
            items: [
                imageDiv,
            ],
        })];
    }

    return [imageDiv];
}
