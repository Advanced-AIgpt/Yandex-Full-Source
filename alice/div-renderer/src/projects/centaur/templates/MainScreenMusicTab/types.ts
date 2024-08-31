export interface IMusicCard {
    cover: string;
    title: string;
    subtitle?: string;
    id: string;
    action?: string;
    type: string;
}

export interface IMusicLine {
    title: string;
    type: string | null;
    items: IMusicCard[];
}

export interface IMusicTabData {
    lines: IMusicLine[];
}
