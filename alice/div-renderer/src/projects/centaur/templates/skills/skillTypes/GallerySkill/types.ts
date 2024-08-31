export interface ISkillCardGalleryImage {
    imageUrl: string;
    title: string;
    description: string;
}

export interface ISkillCardGallery {
    type: 'Gallery';
    images: ISkillCardGalleryImage[];
}
