// User model with TypeScript features

interface IUser {
    id: number;
    name: string;
    email: string;
    age?: number;
}

type UserRole = "admin" | "user" | "guest";

export class User implements IUser {
    public id: number;
    public name: string;
    public email: string;
    public age?: number;
    private role: UserRole;

    constructor(id: number, name: string, email: string, role: UserRole = "user") {
        this.id = id;
        this.name = name;
        this.email = email;
        this.role = role;
    }

    public getInfo(): string {
        return `${this.name} (${this.email})`;
    }

    public isAdmin(): boolean {
        return this.role === "admin";
    }

    public setAge(age: number): void {
        this.age = age;
    }
}

export function createUser(id: number, name: string, email: string): User {
    return new User(id, name, email);
}
