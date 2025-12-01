// Shopping cart service with TypeScript features

import { Product, Category } from "../models/Product";
import { User } from "../models/User";

interface CartItem {
    product: Product;
    quantity: number;
}

type CartCallback = (total: number) => void;

export class CartService {
    private items: CartItem[] = [];
    private user: User | null = null;
    private onUpdateCallback?: CartCallback;

    constructor(user?: User) {
        if (user) {
            this.user = user;
        }
    }

    public setUser(user: User): void {
        this.user = user;
    }

    public addItem(product: Product, quantity: number = 1): void {
        const existing = this.items.find(item => item.product.id === product.id);

        if (existing) {
            existing.quantity += quantity;
        } else {
            this.items.push({ product, quantity });
        }

        this.notifyUpdate();
    }

    public removeItem(productId: number): boolean {
        const index = this.items.findIndex(item => item.product.id === productId);

        if (index !== -1) {
            this.items.splice(index, 1);
            this.notifyUpdate();
            return true;
        }

        return false;
    }

    public getTotal(): number {
        let total = 0;
        for (const item of this.items) {
            total += item.product.price * item.quantity;
        }
        return total;
    }

    public getItemCount(): number {
        let count = 0;
        for (const item of this.items) {
            count += item.quantity;
        }
        return count;
    }

    public clear(): void {
        this.items = [];
        this.notifyUpdate();
    }

    public onUpdate(callback: CartCallback): void {
        this.onUpdateCallback = callback;
    }

    private notifyUpdate(): void {
        if (this.onUpdateCallback) {
            this.onUpdateCallback(this.getTotal());
        }
    }

    public getSummary(): string {
        const lines: string[] = [];
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
export const createCart = (user?: User): CartService => new CartService(user);
