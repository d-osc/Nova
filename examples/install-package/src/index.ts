// Nova Install Demo
// This example demonstrates using packages installed via `nova install`

import _ from 'lodash';
import { v4 } from 'uuid';

// Demo: Using lodash utilities
function lodashDemo(): void {
    console.log('=== Lodash Demo ===');

    const numbers: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

    // chunk - split array into groups
    const chunks = _.chunk(numbers, 3);
    console.log('Chunk [1-10] by 3:', chunks);

    // shuffle - randomize array
    const shuffled = _.shuffle(numbers);
    console.log('Shuffled:', shuffled);

    // uniq - remove duplicates
    const withDupes = [1, 1, 2, 2, 3, 3, 4, 5, 5];
    console.log('Unique:', _.uniq(withDupes));

    // groupBy - group objects by property
    const users = [
        { name: 'Alice', age: 25 },
        { name: 'Bob', age: 30 },
        { name: 'Charlie', age: 25 },
        { name: 'Diana', age: 30 }
    ];
    const byAge = _.groupBy(users, 'age');
    console.log('Grouped by age:', byAge);

    // sortBy - sort objects
    const sorted = _.sortBy(users, ['age', 'name']);
    console.log('Sorted by age, name:', sorted);
}

// Demo: Using uuid
function uuidDemo(): void {
    console.log('\n=== UUID Demo ===');

    // Generate random UUIDs
    console.log('UUID v4:', v4());
    console.log('UUID v4:', v4());
    console.log('UUID v4:', v4());

    // Generate multiple UUIDs
    const ids: string[] = Array.from({ length: 5 }, () => v4());
    console.log('5 UUIDs:', ids);
}

// Demo: Combining packages
function combinedDemo(): void {
    console.log('\n=== Combined Demo ===');

    // Create objects with UUIDs and process with lodash
    const items = _.times(5, (n) => ({
        id: v4(),
        name: `Item ${n + 1}`,
        value: _.random(1, 100),
        category: _.sample(['A', 'B', 'C'])
    }));

    console.log('Generated items:');
    items.forEach(item => {
        console.log(`  ${item.id.substring(0, 8)}... | ${item.name} | value: ${item.value} | cat: ${item.category}`);
    });

    // Group by category
    const byCategory = _.groupBy(items, 'category');
    console.log('\nGrouped by category:');
    Object.entries(byCategory).forEach(([cat, catItems]) => {
        const total = _.sumBy(catItems, 'value');
        console.log(`  Category ${cat}: ${catItems.length} items, total value: ${total}`);
    });

    // Find max value item
    const maxItem = _.maxBy(items, 'value');
    console.log(`\nHighest value item: ${maxItem?.name} (${maxItem?.value})`);
}

// Run all demos
function main(): void {
    console.log('Nova Install Demo');
    console.log('==================\n');

    lodashDemo();
    uuidDemo();
    combinedDemo();

    console.log('\n==================');
    console.log('Demo completed!');
}

main();
