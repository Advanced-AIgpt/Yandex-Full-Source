import { Div } from 'divcard2';
import { NAlice } from '../../../../../protos';
import { SearchRichCardMusicAlbumsSection } from './SearchRichCardMusicAlbumsSection';
import { SearchRichCardMusicBandsSection } from './SearchRichCardMusicBandsSection';
import { SearchRichCardVideoClipsSection } from './SearchRichCardVideoClipsSection';
import { SearchRichCardFactListSection } from './SearchRichCardFactListSection';
import { SearchRichCardGallerySection } from './SearchRichCardGallerySection';
import { SearchRicCardVideoMoviesSection } from './SearchRichCardVideoMoviesSection';
import { SearchRichCardPersonsSection } from './SearchRichCardPersonsSection';
import { SearchRichCardTextSection } from './SearchRichCardTextSection';
import { SearchRichCardMusicTracksSection } from './SearchRichCardMusicTracksSection';
import { SearchRichCardGeoPlacesSection } from './SearchRichCardGeoPlacesSection';
import { SearchRichCardBooksSection } from './SearchRichCardBooksSection';
import { SearchRichCardCompaniesSection } from './SearchRichCardCompaniesSection';
import { IRequestState } from '../../../../../common/types/common';
import { createDataAdapter, MakeTestSchemaType } from '../../../helpers/createDataAdapter';

export type SectionTemplate = (
    section: NAlice.NData.TSearchRichCardData.TBlock.ITSection,
    requestState: IRequestState,
) => Div | undefined;

type SectionType = Exclude<
    | keyof NAlice.NData.TSearchRichCardData.TBlock.ITSection,
    'Order' | 'Hidden'
>;

const sectionTypeToSectionTemplateMap: Record<SectionType, SectionTemplate> = {
    Persons: SearchRichCardPersonsSection,
    VideoMovies: SearchRicCardVideoMoviesSection,
    Text: SearchRichCardTextSection,
    FactList: SearchRichCardFactListSection,
    Gallery: SearchRichCardGallerySection,
    VideoClips: SearchRichCardVideoClipsSection,
    MusicAlbums: SearchRichCardMusicAlbumsSection,
    MusicTracks: SearchRichCardMusicTracksSection,
    MusicBands: SearchRichCardMusicBandsSection,
    GeoPlaces: SearchRichCardGeoPlacesSection,
    Books: SearchRichCardBooksSection,
    Companies: SearchRichCardCompaniesSection,
};

const schema: MakeTestSchemaType<NAlice.NData.TSearchRichCardData.TBlock.ITSection> = {
    anyOf: [
        { type: 'object', required: ['Persons'] },
        { type: 'object', required: ['VideoMovies'] },
        { type: 'object', required: ['Text'] },
        { type: 'object', required: ['FactList'] },
        { type: 'object', required: ['Gallery'] },
        { type: 'object', required: ['VideoClips'] },
        { type: 'object', required: ['MusicAlbums'] },
        { type: 'object', required: ['MusicTracks'] },
        { type: 'object', required: ['MusicBands'] },
        { type: 'object', required: ['GeoPlaces'] },
        { type: 'object', required: ['Books'] },
        { type: 'object', required: ['Companies'] },
    ],
};

const dataAdapter = createDataAdapter(schema, (section: NAlice.NData.TSearchRichCardData.TBlock.ITSection) => {
    const [sectionType] = Object.keys(section) as SectionType[];

    const sectionTemplate: SectionTemplate = sectionTypeToSectionTemplateMap[sectionType];

    return { sectionTemplate };
});

export const SearchRichCardSection: SectionTemplate = (section, requestState) => {
    const { sectionTemplate } = dataAdapter(section,requestState);

    return sectionTemplate ? sectionTemplate(section, requestState) : undefined;
};
