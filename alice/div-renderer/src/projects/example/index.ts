import { templateHelper } from 'divcard2';
import { templatesRegistry } from '../../registries/common';
import GreetingsTemplate from './components/Greetings/GreetingsTemplate';

const templates = {
    greetings: GreetingsTemplate(),
};

for (const key of Object.keys(templates)) {
    templatesRegistry.add(key, templates[key as keyof typeof templates]);
}

export const exampleTemplateHelper = templateHelper(templates);
