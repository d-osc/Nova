// Product model demonstrating generics and enums

export enum Category {
    Electronics = "electronics",
    Clothing = "clothing",
    Food = "food",
    Books = "books"
}

interface IProduct<T = any> {
    id: number;
    name: string;
    price: number;
    category: Category;
    metadata?: T;
}

export class Product<T = any> implements IProduct<T> {
    readonly id: number;
    public name: string;
    public price: number;
    public category: Category;
    public metadata?: T;

    constructor(id: number, name: string, price: number, category: Category) {
        this.id = id;
        this.name = name;
        this.price = price;
        this.category = category;
    }

    public applyDiscount(percent: number): number {
        const discount = this.price * (percent / 100);
        return this.price - discount;
    }

    public setMetadata(data: T): void {
        this.metadata = data;
    }

    public toString(): string {
        return `${this.name} - $${this.price.toFixed(2)}`;
    }
}

export const createProduct = <T>(
    id: number,
    name: string,
    price: number,
    category: Category
): Product<T> => {
    return new Product<T>(id, name, price, category);
};
