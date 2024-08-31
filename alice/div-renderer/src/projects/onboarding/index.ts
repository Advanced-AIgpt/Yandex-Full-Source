import { templateHelper } from 'divcard2';
import { templatesRegistry } from '../../registries/common';
import GreetingsTemplates from './components/Greetings/GreetingsTemplates';

const templates = {
    ...GreetingsTemplates(),
};

for (const key of Object.keys(templates)) {
    templatesRegistry.add(key, templates[key as keyof typeof templates]);
}

export const onboardingTemplateHelper = templateHelper(templates);
