export interface IVideoSearchCard {
    title: string;
    imageUrl: string;
    actionUrl: string;
}

export interface IVideoSearchGallery {
    title: string;
    items: IVideoSearchCard[];
}

export interface IVideoSearchData {
    galleries: IVideoSearchGallery[];
}
