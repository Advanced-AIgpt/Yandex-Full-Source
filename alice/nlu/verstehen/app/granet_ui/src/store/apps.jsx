import { fetchApps } from '../api/index';

const apps = store => {
    store.on('@init', () => ({
        apps: {
            'all': [],
            'current': undefined,
            'appsIndices': [],
            'curIndex': undefined
        }
    }))

    store.on('apps/current/update', ({ apps }, newCurrent) => {
        return {apps: {...apps, 'current': newCurrent}};
    })

    store.on('apps/curIndex/update', ({ apps }, newIdx) => {
        return {apps: {...apps, 'curIndex': newIdx}};
    })

    store.on('apps/all/update', ({ apps }, data) => {
        if (!data.hasOwnProperty('all_apps')) {
            return { apps: { 'all': [], 'current': 'None' } }
        }

        let keys = [];
        for (let key in data.all_apps) {
            keys.push(key);
        }
        return {
            apps: {
                'all': keys,
                'current': data.default_app,
                'appsIndices': data.all_apps,
                'curIndex': data.all_apps[data.default_app].default_index
            }
        }
    })

    store.on('apps/all/failedUpdate', ({ apps }) => {
        return {
            apps: {
                'all': [],
                'current': 'None'
            }
        }
    })

    store.on('apps/load', ({ apps }) => {
        fetchApps()
            .then(
                (data) => {
                    store.dispatch('apps/all/update', data);
                },
                (xhr, ajaxOptions, thrownError) => { 
                    console.error(xhr); 
                    store.dispatch('apps/all/failedUpdate');
                }
            );
    })
}


export default apps;