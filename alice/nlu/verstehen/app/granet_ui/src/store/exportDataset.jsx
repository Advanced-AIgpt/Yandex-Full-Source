import { downloadFileFromText, sendSearch } from '../api';


const exportDataset = store => {
    store.on('@init', () => ({ exportDataset : {
        'visible': false,
        'content': '',
        'preview': '', 
        'fetchInProgress': ''
    }}))
  
    store.on('exportDataset/update', ({ exportDataset }, value) => {
        return { exportDataset: {...exportDataset, ...value }}
    })

    store.on('exportDataset/fetchMocks', ({ exportDataset }) => {
        let target = new Array(store.get().userSamples.positives.length).fill('1');
        target = target.concat(new Array(store.get().userSamples.negatives.length).fill('0'));
        const opts = {
            target: target,
            userSamples: 
                store.get().userSamples.positives.map(el => el.text).concat(
                    store.get().userSamples.negatives.map(el => el.text)
                ),
            appIds: store.get().userSamples.positives.map(el => el.appIdx).concat(
                store.get().userSamples.negatives.map(el => el.appIdx)
            )
        }
        sendSearch(store.get().apps['current'], 'Granet', store.get().currentGrammar, [], opts)
            .then(
                onSuccessFetch, 
                onErrorFetch
            );
        return {exportDataset: {...exportDataset, 'fetchInProgress': true}};
    })

    store.on('exportDataset/formPreview', ({ exportDataset }, response) => {
        let lines = ['Target    Text     Mock'];

        for (let res of response) {
            let line = [];
            line.push(res.target);
            if (store.get().settings.showMarkup) {
                line.push(res.markup);
            } else {
                line.push(res.text);
            }
            line.push(res.mock);
            lines.push(line.join('\t'));
        }

        let preview = lines.slice(0, 2).concat(['...']).concat(lines.slice(-1))
        preview.forEach((item, idx, arr) => { 
            if (item.length > 54)
                arr[idx] = item.substr(0, 51) + '...';
        });
        preview = preview.join('\n');

        const content = lines.join('\n');
        return {exportDataset: {...exportDataset, 'preview': preview, 'content': content}};
    })

    store.on('exportDataset/download', ({exportDataset}) => {
        downloadFileFromText(exportDataset.content, 'tmp', 'text/plain');
    })

    function onSuccessFetch(data) {
        const response = data.search_result.map(el => el.index_payload)
        store.dispatch('exportDataset/update', {'fetchInProgress': false});
        store.dispatch('exportDataset/formPreview', response);
    }

    function onErrorFetch(xhr) {
        console.error(xhr);
        store.dispatch('exportDataset/update', {'fetchInProgress': false});
    }
  }

export default exportDataset;