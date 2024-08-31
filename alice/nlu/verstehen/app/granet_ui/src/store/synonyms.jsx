import { sendSearch } from '../api';
import { timeWrapper } from '../util';

const synonyms = store => {
    store.on('@init', () => ({
        synonyms: {
            'text': '',
            'query': [], //[['$Give', 'загадай'], ['$Number', 'число']],
            'results': [], // [['посоветуй число', 1]],
            'curInterval': undefined,
            'grammarSynonyms': false,
            'noise': 0,
            'nMasks': 1,
            'nSamples': 20
        }
    }))

    store.on('synonyms/update', ({ synonyms }, values) => {
        return { synonyms: {...synonyms, ...values }};
    })

    store.on('synonyms/search', ({ synonyms }, value) => {
        const opts = {
            synonyms: {
                text: (value['text']) ? value.text : synonyms.text,
                interval: value['interval'],
                opts: { 
                    grammarSynonyms: synonyms.grammarSynonyms,
                    noise: synonyms.noise,
                    nMasks: synonyms.nMasks,
                    nSamples: synonyms.nSamples
                }
            }

        }
        sendSearch(store.get().apps['current'], 'Granet', store.get().currentGrammar, [], opts)
            .then(
                timeWrapper(onSuccessSearch), 
                onErrorSearch
            );
        return {
            synonyms: {...synonyms, curInterval: value['interval']}
        }
    })

    store.on('synonyms/expandGrammar', ({ synonyms }, value) => {
        console.log('ExpandGrammar');
        const sample = synonyms.query.map(elem => elem[1]).join('');
        const interval = synonyms.curInterval;
        const opts = {
            updateGrammar: {
                sample: sample,
                interval: interval,
                value: value
            }

        }
        sendSearch(store.get().apps['current'], 'Granet', store.get().currentGrammar, [], opts)
            .then(
                data => { 
                    store.dispatch('currentGrammar/update', data.search_result[0].index_payload.grammar);
                    console.log(data); 
                }, 
                xhr => { console.log(xhr); }
            );
    })

    function onSuccessSearch(data, time) {
        console.log('synonyms/onSuccessSearch');
        console.log(time);
        const values = {
            text: data.search_result[0].index_payload.text,
            query: data.search_result[0].index_payload.query, 
            results: data.search_result[0].index_payload.results,
            curInterval: data.search_result[0].index_payload.interval
        }
        store.dispatch('synonyms/update', values);
    }

    function onErrorSearch(xhr, ajaxOptions, thrownError) {
        console.error(xhr.status);
        console.error(ajaxOptions);
        console.error(thrownError);
    }
}


export default synonyms;