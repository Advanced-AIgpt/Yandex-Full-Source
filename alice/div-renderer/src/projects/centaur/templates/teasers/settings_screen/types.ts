export interface ITeaserItemsListItem {
    title: string,
    teaserType: string,
    teaserId: string,
    isChosen: boolean
}

export interface ITeaserItemsList {
    type: 'ItemsList',
    header: {
        text: string,
    },
    items: ITeaserItemsListItem[],
}
