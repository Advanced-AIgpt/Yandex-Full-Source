export default abstract class AbstractRegistry<T> {
    protected items: {[id: string]: T} = {};

    /**
     * Add item to registry. **Throw error if duplicated key**
     * @param id
     * @param item
     */
    add(id: string, item: T | null) {
        if (item !== null) {
            if (Object.prototype.hasOwnProperty.call(this.items, id)) {
                throw new Error(`Duplicated key ${id}`);
            }
            this.items[id] = item;
        }
    }

    /**
     * Add item to registry. Replace item if duplicated key
     * @param id
     * @param item
     */
    addOrReplace(id: string, item: T | null) {
        if (item !== null) {
            this.items[id] = item;
        }
    }

    /**
     * Get exists item or null if not exists
     * @param id
     */
    get(id: string) {
        return this.items[id] || null;
    }

    /**
     * get object all of items
     */
    getAll(): {[id: string]: T} {
        return this.items;
    }
}
