export const wrappers = {
    centaur: (d: unknown) => ({
        layer: {
            dialog: {},
        },
        do_not_show_close_button: false,
        inactivity_timeout: 'Medium',
        div2_card: {
            body: d,
        },
    }),
};
