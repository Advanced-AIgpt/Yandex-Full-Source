import { MainPlayerPart } from './MainPlayerPart';
import { SecondaryPlayerPart } from './SecondaryPlayerPart';

export interface IMusicArtist {
    id?: string | null;
    name?: string | null;
    isComposer?: boolean | null;
    isVarious?: boolean | null;
}

export interface IMusicTrackInfo {
    id: string;
    title: string;
    subtype?: string | null;
    altImageUrl?: string | null;
    imageBackground?: string;
    artists?: IMusicArtist[] | null;
    isLiked?: boolean;
}

export type IMusicDivProps = Parameters<typeof MainPlayerPart>[0] &
    Parameters<typeof SecondaryPlayerPart>[0] & { imageBackground?: string };
