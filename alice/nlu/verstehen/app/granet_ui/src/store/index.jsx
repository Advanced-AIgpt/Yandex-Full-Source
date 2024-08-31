import createStore from 'storeon';
import persistState from '@storeon/localstorage'

import addButtons from './addButtons';
import apps from './apps';
import appTab from './appTab';
import currentGrammar from './grammar';
import exportGrammar from './exportGrammar';
import exportDataset from './exportDataset';
import info from './info';
import searchResultsGranet from './searchResultsGranet';
import searchResultsVerstehen from './searchResultsVerstehen';
import userSamples from './userSamples';
import settings from './settings';
import synonyms from './synonyms';

const store = createStore([
    appTab,
    currentGrammar, 
    info,
    searchResultsGranet, 
    searchResultsVerstehen,
    userSamples, 
    apps, 
    exportGrammar, 
    exportDataset,
    settings, 
    addButtons,
    synonyms,
    persistState(['currentGrammar', 'userSamples', 'settings'])]);

export default store;