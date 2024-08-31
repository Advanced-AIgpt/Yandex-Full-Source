const settings = store => {
    store.on('@init', () => ({
        settings: {
            'drawerVisible': false,
            'showAddButtons': false,
            'showMarkup': true,
            'showOccurance': false,
            'showClfScore': false,
            'printSlotsValues': false,
            'printSlotsTypes': false,
            'linesToShow': 150  // TODO: read from config
        }
    }))

    store.on('settings/update', ({ settings }, value) => {
        return { settings: { ...settings, ...value } };
    })

    store.on('settings/drawerVisible/update', ({ settings }, value) => {
        return { settings: { ...settings, 'drawerVisible': value } }
    })
}

export default settings;