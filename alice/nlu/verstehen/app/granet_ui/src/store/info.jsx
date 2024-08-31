const info = store => {
    store.on('@init', () => ({ info: {
        'visible': false
    } }))

    store.on('info/visible', ({ info }, value) => {
        return { info: {...info, 'visible': value} }
    })
}

export default info;
