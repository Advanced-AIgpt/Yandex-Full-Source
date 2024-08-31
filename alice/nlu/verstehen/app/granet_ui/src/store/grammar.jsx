import { defaultGrammar } from '../resources/index';

const currentGrammar = store => {
    store.on('@init', () => ({ currentGrammar : defaultGrammar }))
  
    store.on('currentGrammar/update', (state, newGrammar) => {
      return { currentGrammar: newGrammar }
    })
  }

export default currentGrammar;