const addButtons = store => {
    store.on('@init', () => ({
        addButtons: {
            'searchResultsGranet': {},
            'searchResultsVerstehen': {}
        }
    }))

    store.on('addButtons/reset', ({ addButtons }) => {
        return {addButtons: {
            'searchResultsGranet': {},
            'searchResultsVerstehen': {}
        }}
    })

    store.on('addButtons/update', ({ addButtons }, sample) => {
        let newState = {...addButtons}
        newState[sample.col][sample.idx] = sample.button;
        return {addButtons: newState};
    })

    store.on('addButtons/remove', ({ addButtons }, sample) => {
        let col = {...addButtons[sample.col]}
        delete col[sample.idx];
        return {addButtons: {...addButtons, [sample.col]: col}};
    })
}


export default addButtons;