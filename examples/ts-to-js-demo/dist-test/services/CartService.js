"use strict";
// Shopping cart service with TypeScript features

const { Product, Category } = require("../models/Product");
const { User } = require("../models/User");

class CartService {
    items = [];
    user = null;
    onUpdateCallback;

    constructor(user) {
        if (user) {
            this.user = user;
        }
    }

    setUser(user) {
        this.user = user;
    }

    addItem(product, quantity = 1) {
        const existing = this.items.find(item => item.product.id === product.id);

        if (existing) {
            existing.quantity += quantity;
        } else {
            this.items.push({ product, quantity });
        }

        this.notifyUpdate();
    }

    removeItem(productId) {
        const index = this.items.findIndex(item => item.product.id === productId);

        if (index !== -1) {
            this.items.splice(index, 1);
            this.notifyUpdate();
            return true;
        }

        return false;
    }

    getTotal() {
        let total = 0;
        for (const item of this.items) {
            total += item.product.price * item.quantity;
        }
        return total;
    }

    getItemCount() {
        let count = 0;
        for (const item of this.items) {
            count += item.quantity;
        }
        return count;
    }

    clear() {
        this.items = [];
        this.notifyUpdate();
    }

    onUpdate(callback) {
        this.onUpdateCallback = callback;
    }

    notifyUpdate() {
        if (this.onUpdateCallback) {
            this.onUpdateCallback(this.getTotal());
        }
    }

    getSummary() {
        const lines = [];
        lines.push("=== Cart Summary ===");

        if (this.user) {
            lines.push(`Customer: ${this.user.getInfo()}`);
        }

        lines.push(`Items: ${this.getItemCount()}`);
        lines.push(`Total: $${this.getTotal().toFixed(2)}`);

        return lines.join("\n");
    }
}

// Factory function with arrow syntax
const createCart = (user) => new CartService(user);

module.exports = { createCart, CartService };
