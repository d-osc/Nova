function testSimpleWhile(): number {
    let i = 0;
    while (i < 5) {
        i = i + 1;
    }
    return i;
}

function main(): number {
    return testSimpleWhile();
}