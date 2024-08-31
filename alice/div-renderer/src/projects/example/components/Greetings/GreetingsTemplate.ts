import { Template, TextBlock } from 'divcard2';

export default function GreetingsTemplate() {
    return new TextBlock({
        text: new Template('hello'),
        text_color: '#ff0',
    });
}
