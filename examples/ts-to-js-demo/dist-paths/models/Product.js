

const Category = {
    Electronics: "electronics",
    Clothing: "clothing",
    Food: "food",
    Books: "books"
};

class Product{
    id;
    name;
    price;
    category;
    metadata;

    constructor(id, name, price, category) {
        this.id = id;
        this.name = name;
        this.price = price;
        this.category = category;
    }

    applyDiscount(percent) {
        const discount = this.price * (percent / 100);
        return this.price - discount;
    }

    setMetadata(data) {
        this.metadata = data;
    }

    toString() {
        return `${this.name} - $${this.price.toFixed(2)}`;
    }
}

const createProduct = (
    id,
    name,
    price,
    category) => {
    return new Product(id, name, price, category);
};

module.exports = { createProduct, Product, Category };
