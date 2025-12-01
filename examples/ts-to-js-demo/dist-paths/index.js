

const { User, createUser } = require("./models/User");
const { Product, Category, createProduct } = require("./models/Product");
const { CartService, createCart } = require("./services/CartService");
const { capitalize, clamp, unique, first, last } = require("./utils/helpers");

function main() {
    
    const user = createUser(1, "John Doe", "john@example.com");
    user.setAge(25);

    console.log("User:", user.getInfo());
    console.log("Is Admin:", user.isAdmin());

    const products = [
        createProduct(1, "Laptop", 999.99, Category.Electronics),
        createProduct(2, "T-Shirt", 29.99, Category.Clothing),
        createProduct(3, "JavaScript Book", 49.99, Category.Books),
    ];

    const laptop = products[0];
    console.log("Product:", laptop.toString());
    console.log("After 20% discount:", laptop.applyDiscount(20).toFixed(2));

    const cart = createCart(user);

    cart.onUpdate((total) => {
        console.log("Cart updated Total:", total.toFixed(2));
    });

    for (const product of products) {
        cart.addItem(product);
    }

    cart.addItem(laptop);

    console.log(cart.getSummary());

    const numbers = [1, 2, 2, 3, 3, 3, 4];
    console.log("Unique numbers:", unique(numbers));
    console.log("First:", first(numbers));
    console.log("Last:", last(numbers));

    const name = "typescript";
    console.log("Capitalized:", capitalize(name));

    const value = 150;
    console.log("Clamped (0-100):", clamp(value, 0, 100));

    return cart.getItemCount();
}

const result = main();
console.log("Total items in cart:", result);
