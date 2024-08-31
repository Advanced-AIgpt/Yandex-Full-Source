import { ContainerBlock, DivStateBlock, GalleryBlock, SeparatorBlock, TextBlock } from 'divcard2';

export type ExtractBlockProps<T extends abstract new (...args: any) => any> = Partial<ConstructorParameters<T>[0]>;
export type ContainerBlockProps = ExtractBlockProps<typeof ContainerBlock>;
export type GalleryBlockProps = ExtractBlockProps<typeof GalleryBlock>;
export type TextBlockProps = ExtractBlockProps<typeof TextBlock>;
export type DivStateBlockProps = ExtractBlockProps<typeof DivStateBlock>;
export type SeparatorBlockProps = ExtractBlockProps<typeof SeparatorBlock>;
