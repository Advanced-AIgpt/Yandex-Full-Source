import { sendSearch } from '../api/index';
import { generate_uuidv4 } from '../util';

const userSamples = store => {
    store.on('@init', () => ({
        userSamples: {
            'positives': [],
            'negatives': [],
            'reevaluateInProgress': false,
            'positivesSelected': [],
            'negativesSelected': []
        }
    }))

    store.on('userSamples/reset', ({ userSamples }) => {
        console.log('userSamples/reset');
        return { userSamples: {
            'positives': [],
            'negatives': [],
            'reevaluateInProgress': false,
            'positivesSelected': [],
            'negativesSelected': []
        }};
    })


    store.on('userSamples/reevaluate', ({ userSamples }, key) => {
        if (userSamples[key].length === 0) {
            console.log('Nothing to reevaluate');
            return;
        }
        let texts = [];
        let appIds = [];
        store.get().userSamples[key].forEach(el => {
            texts.push(el.text);
            appIds.push(el.appIdx);
        })

        const opts = {
            userSamples: texts,
            appIds: appIds
        }
        sendSearch(store.get().apps['current'], 'Granet', store.get().currentGrammar, [], opts)
            .then(
                function (data) { onReevaluateSuccess(data, key); },
                onReevaluateFail
            );
        return { userSamples: { ...userSamples, 'reevaluateInProgress': true } };
    })

    store.on('userSamples/add', ({ userSamples }, data) => {
        const key = data.key;
        const text = data.value.text;
        const appIdx = data.value.appIdx;
        console.log('userSamples/add');
        console.log(data);
        if (!text) {
            console.error('Attemted to add empty text');
            return;
        }

        const hasText = userSamples[key].reduce((acc, sample) => acc || sample.text === text, false)
        if (hasText) {
            console.warn('Attemted to add duplicate sample');
            return;
        }

        const opts = {
            userSamples: [text],
            appIds: [appIdx]
        };
        console.log('Adding ' + text + ' to ' + key);
        sendSearch(store.get().apps['current'], 'Granet', store.get().currentGrammar, [], opts)
            .then(
                data => onAddSuccess(data, key, text, appIdx),
                (xhr, ajaxOptions, thrownError) => onAddFail(xhr, ajaxOptions, thrownError, key, text, appIdx)
            );
    })

    store.on('userSamples/update', ({ userSamples }, data) => {
        const key = data.key;
        const value = data.value;
        let idx = -1;
        for (let i = 0; i < userSamples[key].length; i++) {
            if (userSamples[key][i].text === value.text) {
                idx = i;
                break;
            }
        }
        console.log('userSamples/update');
        console.log(data);
        let newUserSamples = { ...userSamples }
        if (idx === -1) {
            newUserSamples[key].push(value);
        } else {
            newUserSamples[key][idx] = value;
        }
        return { userSamples: newUserSamples };
    })    

    store.on('userSamples/positives/delete', ({ userSamples }, uuid) => {
        let idx;
        for (let i = 0; i < userSamples.positives.length; i++) {
            let sample = userSamples.positives[i];
            if (sample.uuid === uuid) {
                idx = i;
                break;
            }
        }
        return { userSamples: { ...userSamples, 'positives': userSamples['positives'].filter((data, i) => i !== idx) } };
    })

    store.on('userSamples/negatives/delete', ({ userSamples }, uuid) => {
        let idx;
        for (let i = 0; i < userSamples.negatives.length; i++) {
            let sample = userSamples.negatives[i];
            if (sample.uuid === uuid) {
                idx = i;
                break;
            }
        }
        return { userSamples: { ...userSamples, 'negatives': userSamples['negatives'].filter((data, i) => i !== idx) } };
    })

    store.on('userSamples/selected/update', ({ userSamples }, item) => {
        return { userSamples: { ...userSamples, [item.key + 'Selected']: item.value } }
    })

    store.on('userSamples/reevaluate/success', ({ userSamples }, response) => {
        let newSamples = [];
        response.data.search_result.forEach(el => {
            newSamples.push(
                {
                    text: el.index_payload.text,
                    isPositive: el.index_payload.is_positive,
                    debugInfo: el.index_payload.debug_info,
                    markup: el.index_payload.markup,
                    appIdx: el.idx,
                    uuid: generate_uuidv4()
                }
            );
        });
        let newUserSamples = { ...userSamples, 'reevaluateInProgress': false }
        newUserSamples[response.key] = newSamples
        return { userSamples: newUserSamples }
    })

    store.on('userSamples/reevaluate/fail', ({ userSamples }) => {
        return { userSamples: { ...userSamples, 'reevaluateInProgress': false } }
    })

    store.on('userSamples/add/fail', ({ userSamples }, sample) => {
        store.dispatch('userSamples/update', { key: sample.key, value: sample.value });
    })

    function onAddSuccess(data, key, text, appIdx) {
        const isPositive = data.search_result[0].index_payload.is_positive;
        const debugInfo = data.search_result[0].index_payload.debug_info;
        const markup = data.search_result[0].index_payload.markup;
        let elem = {
            text: text,
            isPositive: isPositive,
            debugInfo: debugInfo,
            markup: markup,
            appIdx: appIdx,
            uuid: generate_uuidv4()
        };
        store.dispatch('userSamples/update', { key: key, value: elem });
    }

    function onAddFail(xhr, ajaxOptions, thrownError, key, text, appIdx) {
        console.error(xhr.status);
        console.error(ajaxOptions);
        console.error(thrownError);
        const elem = { 
            text: text, 
            isPositive: false, 
            debugInfo: 'Search failed', 
            markup: text,
            appIdx: appIdx, 
            uuid: generate_uuidv4() };
        store.dispatch('userSamples/add/fail', { key: key, value: elem });
    }

    function onReevaluateSuccess(data, key) {
        store.dispatch('userSamples/reevaluate/success', { data: data, key: key });
    }

    function onReevaluateFail(xhr, ajaxOptions, thrownError) {
        console.error(xhr.status);
        console.error(ajaxOptions);
        console.error(thrownError);
        store.dispatch('userSamples/reevaluate/fail');
    }
}

export default userSamples;
