import { sendSearch } from '../api/index';
import { timeWrapper } from '../util/index';

const searchResultsVerstehen = store => {
    store.on('@init', () => ({ searchResultsVerstehen : {
        'results': [],
        'almostMatchedMarkup': [],
        'occuranceRate': [],
        'clfScore': [], 
        'appIds': [],
        'time': undefined,
        'inProgress': false,
        'almostMatchedInProgress': false,
        'selected': []
    } }))

    store.on('searchResultsVerstehen/reset', ({ searchResultsVerstehen }) => {
        return {searchResultsVerstehen : {
            'results': [],
            'almostMatchedMarkup': [],
            'occuranceRate': [],
            'clfScore': [], 
            'appIds': [],
            'time': undefined,
            'inProgress': false,
            'almostMatchedInProgress': false,
            'selected': []
        }};
    })

    store.on('searchResultsVerstehen/search', ({ searchResultsVerstehen }) => {
        store.dispatch('addButtons/reset');
        // cut first 15 samples for most indices, this way embeddings-based indices should work better
        // TODO get rid of hardcode
        const nPositives = store.get().apps['curIndex'] === 'DSSM with MLP' ? undefined : 15;
        const positives = store.get().searchResultsGranet['texts'].slice(0, nPositives)
            .concat(store.get().userSamples['positives'].map(el => {
                return el.text;
            }).slice(0, nPositives));
        if (!positives || positives.length === 0) {
            console.error('Empty positives');
            return;
        }

        const filters = store.get().searchResultsGranet['texts']
            .concat(store.get().userSamples.positives.concat(store.get().userSamples.negatives).map(el => {
                return el.text;
            }));
        const opts = {filters: filters}
        sendSearch(store.get().apps['current'], store.get().apps['curIndex'], 
            positives,
            store.get().userSamples['negatives'].map(el => {
                return el.text;
            }),
            opts)
        .then(
            timeWrapper(onSuccessVerstehenSearch), 
            onErrorVerstehenSearch
        );
        return {searchResultsVerstehen: {...searchResultsVerstehen, 'inProgress': true}};
    })

    store.on('searchResultsVerstehen/update', ({ searchResultsVerstehen }, response) => {
        const data = response['data'];

        let results = [];
        let scores = [];
        let appIds = [];
        data.search_result.forEach(el => {
            results.push(el['text']);
            scores.push(el['score']);
            appIds.push(el['idx']);
        });

        let occRate;
        if (data.search_result.length === 0) {
            occRate = [];
        } else if (data.search_result[0]['payload'] && data.search_result[0].payload['occurrence_rate']) {
            const occSum = data.search_result.reduce((acc, el) => acc + el.payload.occurrence_rate, 0)
            occRate = data.search_result.map(el => {
                return el.payload.occurrence_rate / occSum;
            });
        } else {
            occRate = data.search_result.map(el => {
                return 1 / data.search_result.length});
        }

        store.dispatch('searchResultsVerstehen/checkMatching', appIds);

        return {
            searchResultsVerstehen: {...searchResultsVerstehen, 
                'results': results, 
                'occuranceRate': occRate,
                'clfScore': scores,
                'appIds': appIds,
                'time': response['time'],  
                'inProgress': false, 
                'almostMatchedInProgress': true
            }
          };
    })

    store.on('searchResultsVerstehen/failedUpdate', ({ searchResultsVerstehen }) => {
        return {searchResultsVerstehen: {...searchResultsVerstehen, 'inProgress': false}};
    })

    store.on('searchResultsVerstehen/checkMatching', ({ searchResultsVerstehen }, appIds) => {
        const opts = {almostMatched: true, appIds: appIds}
        sendSearch(store.get().apps['current'], 'Granet', 
            store.get().currentGrammar, [],
            opts)
        .then(
            (data) => {
                store.dispatch('searchResultsVerstehen/almostMatchedMarkup/update', data);
            }, 
            (xhr, ajaxOptions, thrownError) => {
                console.error(xhr);
                console.error(ajaxOptions);
                console.error(thrownError);
                store.dispatch('searchResultsVerstehen/almostMatchedMarkup/updateOnFail');
            }
        );
        return {searchResultsVerstehen: {...searchResultsVerstehen, 'almostMatchedInProgress': true}};
    })

    store.on('searchResultsVerstehen/almostMatchedMarkup/update', ({ searchResultsVerstehen }, data) => {
        const markups = data.search_result.map((item, idx) => {
            if (item.text !== searchResultsVerstehen.results[idx]) {
                console.error(item.text + " " + searchResultsVerstehen.results[idx]);
            }
            return item.index_payload;
        });
        return {searchResultsVerstehen: {...searchResultsVerstehen, 'almostMatchedMarkup': markups, 'almostMatchedInProgress': false}}
    })

    store.on('searchResultsVerstehen/almostMatchedMarkup/updateOnFail', ({ searchResultsVerstehen }) => {
        return {searchResultsVerstehen: {...searchResultsVerstehen, 'almostMatchedInProgress': false}};
    })

    store.on('searchResultsVerstehen/removeEntry', ({searchResultsVerstehen}, appIdx) => {
        let idx;
        for (let i = 0; i < searchResultsVerstehen.appIds.length; i++) {
            if (searchResultsVerstehen.appIds[i] == appIdx) {
                idx = i;
            }
        }
        let newSearchResults = {...searchResultsVerstehen};
        newSearchResults.results.splice(idx, 1);
        newSearchResults.occuranceRate.splice(idx, 1);
        newSearchResults.clfScore.splice(idx, 1);
        newSearchResults.almostMatchedMarkup.splice(idx, 1);
        newSearchResults.appIds.splice(idx, 1);
        return {searchResultsVerstehen: newSearchResults}
    })

    store.on('searchResultsVerstehen/selected/update', ({searchResultsVerstehen}, value) => {
        return {searchResultsVerstehen: {...searchResultsVerstehen, 'selected': value}};
    });

    store.on('searchResultsVerstehen/selected/remove', ({searchResultsVerstehen}, appIdx) => {
        let newSelected = searchResultsVerstehen.selected.filter(item => item != appIdx);
        return {searchResultsVerstehen: {...searchResultsVerstehen, 'selected': newSelected}};
    });

    function onSuccessVerstehenSearch(data, time) {
        store.dispatch('searchResultsVerstehen/update', {'data': data, 'time': time}); 
    }

    function onErrorVerstehenSearch(xhr, ajaxOptions, thrownError) {
        console.error(xhr.status);
        console.error(ajaxOptions);
        console.error(thrownError);
        store.dispatch('searchResultsVerstehen/failedUpdate');
    }
  }

export default searchResultsVerstehen;
