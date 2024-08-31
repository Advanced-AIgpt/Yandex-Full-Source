declare module "staff-card" {
    declare class StaffCard {
        onHover: () => void;

        constructor(el: HTMLElement, login?: string);
    }

    export = StaffCard;
}
