const appTab = store => {
    store.on('@init', () => ({ appTab: 'search' }))

    store.on('appTab/update', ({ appTab }, value) => {
        console.log(value);
        return { appTab: value }
    })
}

export default appTab;