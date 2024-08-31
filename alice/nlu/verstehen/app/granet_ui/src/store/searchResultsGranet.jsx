import { sendSearch, sendEstimate } from '../api/index';
import { timeWrapper } from '../util/index';

const searchResultsGranet = store => {
    store.on('@init', () => ({ searchResultsGranet : {
        'results': [],
        'occuranceRate': [],
        'clfScore': [],
        'texts': [],
        'appIds': [],
        'time': undefined,
        'inProgress': false,
        'compilerMessage': '',
        'selected': []
    } }))

    store.on('searchResultsGranet/reset', ({ searchResultsGranet }) => {
        return {searchResultsGranet: {
            'results': [],
            'occuranceRate': [],
            'clfScore': [],
            'texts': [],
            'appIds': [],
            'time': undefined,
            'inProgress': false,
            'compilerMessage': '',
            'selected': []
        }};
    })

    store.on('searchResultsGranet/search', ({ searchResultsGranet }) => {
        store.dispatch('addButtons/reset');
        const opts = {
            filters: store.get().userSamples.positives.concat(store.get().userSamples.negatives).map(el => el.text),
            printSlotsValues: store.get().settings['printSlotsValues'],
            printSlotsTypes: store.get().settings['printSlotsTypes']
        }
        sendSearch(store.get().apps['current'], 'Granet', store.get().currentGrammar, [], opts,
                   store.get().settings['linesToShow'])
            .then(
                timeWrapper(onSuccessGranetSearch), 
                onErrorGranetSearch
            );
        return {searchResultsGranet: {...searchResultsGranet, 'inProgress': true}};
    })

    store.on('searchResultsGranet/update', ({ searchResultsGranet }, response) => {
        const data = response['data'];
        let positives = [];
        let results = [];
        let scores = [];
        let appIds = [];
        const scores_sum = data.search_result.reduce((acc, el) => acc + el['score'], 0);
        data.search_result.forEach(el => {
            positives.push(el['text']);
            results.push(el['index_payload']); // here we replace original texts with marked up granet index output
            scores.push(el['score'] / scores_sum);
            appIds.push(el['idx']);
        });

        return {
            searchResultsGranet: { ...searchResultsGranet, 
                'results': results, 
                'occuranceRate': scores,
                'time': response['time'],  
                'inProgress': false,
                'texts': positives,
                'appIds' : appIds,
                'compilerMessage': 'Ok!'
            }
        };
    })

    store.on('searchResultsGranet/onGranetFail', ({ searchResultsGranet }, error) => {
        return {searchResultsGranet: {...searchResultsGranet, 'inProgress': false, 'compilerMessage': error}};
    })

    store.on('searchResultsGranet/clfScore/estimate', ({ searchResultsGranet }) => {
        const positives = store.get().searchResultsGranet['texts'].slice(0, 15)
            .concat(store.get().userSamples.positives.map(el => {
                return el.text;
            }));
        if (!positives || positives.length === 0) {
            console.error('Empty positives');
            return;
        }

        const filters = store.get().searchResultsGranet['texts'];
        const opts = {filters: filters};
        sendEstimate(store.get().apps['current'], 'DSSM with Logistic Regression', 
            positives,
            store.get().userSamples.negatives.map(el => {
                return el.text;
            }),
            opts)
        .then(
            (data) => {
                store.dispatch('searchResultsGranet/clfScore/update', data); 
            }, 
            (xhr, ajaxOptions, thrownError) => { 
                console.error(xhr);
            }
        );
    })

    store.on('searchResultsGranet/clfScore/update', ({ searchResultsGranet }, data) => {
        const scores = data.search_result.map(el => {
            return el['score'];  // TODO? check that texts are inthe same order
        });
        return {searchResultsGranet: {...searchResultsGranet, 'clfScore': scores}}
    })

    store.on('searchResultsGranet/removeEntry', ({searchResultsGranet}, appIdx) => {
        let idx;
        for (let i = 0; i < searchResultsGranet.appIds.length; i++) {
            if (searchResultsGranet.appIds[i] == appIdx) {
                idx = i;
            }
        }
        let newSearchResults = {...searchResultsGranet};
        newSearchResults.results.splice(idx, 1);
        newSearchResults.occuranceRate.splice(idx, 1);
        newSearchResults.clfScore.splice(idx, 1);
        newSearchResults.texts.splice(idx, 1);
        newSearchResults.appIds.splice(idx, 1);
        return {searchResultsGranet: newSearchResults};
    })

    store.on('searchResultsGranet/selected/update', ({searchResultsGranet}, value) => {
        return {searchResultsGranet: {...searchResultsGranet, 'selected': value}};
    });
    store.on('searchResultsGranet/selected/remove', ({searchResultsGranet}, appIdx) => {
        let newSelected = searchResultsGranet.selected.filter(item => item != appIdx);
        return {searchResultsGranet: {...searchResultsGranet, 'selected': newSelected}};
    });

    function onSuccessGranetSearch(data, time) {
        store.dispatch('searchResultsGranet/update', {'data': data, 'time': time});

        store.dispatch('searchResultsVerstehen/search');

        store.dispatch('searchResultsGranet/clfScore/estimate');

        store.dispatch('userSamples/reevaluate', 'positives');
        store.dispatch('userSamples/reevaluate', 'negatives');
    }

    function onErrorGranetSearch(xhr, ajaxOptions, thrownError) {
        console.error(xhr.status);
        console.error(ajaxOptions);
        console.error(thrownError);
        store.dispatch('searchResultsGranet/onGranetFail', thrownError);
    }
};

export default searchResultsGranet;
