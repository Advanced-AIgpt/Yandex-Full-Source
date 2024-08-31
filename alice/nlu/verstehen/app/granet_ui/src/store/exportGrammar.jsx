import { sendSearch } from "../api";

const exportGrammar = store => {
    store.on('@init', () => ({ exportGrammar : {
        'visible': false, 
        'grammarAsExperiment': undefined,
        'requestFailed': false
    }}))
  
    store.on('export/visible/update', ({ exportGrammar }, value) => {
        return { exportGrammar: {...exportGrammar, 'visible': value }}
    })

    store.on('export/grammarAsExperiment/update', ({ exportGrammar } , experiment) => {
        return { exportGrammar: {...exportGrammar, 'grammarAsExperiment': experiment}}
    })

    store.on('export/requestFailed/update', ({ exportGrammar } , value) => {
        return { exportGrammar: {...exportGrammar, 'requestFailed': value}}
    })

    store.on('export/grammarAsExperiment/get', ({ exportGrammar }) => {
        const opts = {grammarAsExperiment: true}
        sendSearch(store.get().apps['current'], 'Granet', store.get().currentGrammar, [], opts)
        .then(
            (data) => { 
                const experiment = data.search_result[0]['index_payload'].debug_info;
                store.dispatch('export/requestFailed/update', false);
                store.dispatch('export/grammarAsExperiment/update', experiment);
            }, 
            (xhr, ajaxOptions, thrownError) => { 
                console.error(xhr);
                store.dispatch('export/requestFailed/update', true);
                store.dispatch('export/grammarAsExperiment/update', 'Search failed!');
            }
        ); 
    })
  }

export default exportGrammar;