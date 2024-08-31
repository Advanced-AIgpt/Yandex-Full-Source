export const AnonymizeDataForSnapshot = (data: unknown) => {
    return JSON.parse(JSON.stringify(data));
};
