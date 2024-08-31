// этот хелпер нужен для нормализации данных
// когда бэкендер отдает данные для верстки, он отдает их в "сыром" виде, в рантайме же они проходят через protobuf
// и получают дополнительную обработку, например, меняется регист букв в полях с PascalCase в snake_case
// сопсна, этот хелпер помогает обработать сырые данные так, чтобы на них можно было разрабатываться

const convert = (data: unknown): unknown => {
    if (data instanceof Array) {
        return data.map(item => convert(item));
    }

    if (data instanceof Object) {
        return Object.keys(data).reduce((result, key) => {
            // конвертим snake_case строку в PascalCase
            const newKey = key.split('_').map(item => item.charAt(0).toUpperCase() + item.slice(1)).join('');

            // @ts-ignore
            // eslint-disable-next-line @typescript-eslint/no-unused-vars
            result[newKey] = convert(data[key]);

            return result;
        }, {});
    }

    return data;
};
