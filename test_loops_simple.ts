// Test while loop
let count1 = 0;
while (count1 < 5) {
    count1 = count1 + 1;
}

// Test for loop with continue and break
let count2 = 0;
for (count2 = 0; count2 < 3; count2 = count2 + 1) {
    if (count2 === 1) {
        continue;
    }
    if (count2 === 2) {
        break;
    }
}

// Test do-while loop
let count3 = 0;
do {
    count3 = count3 + 1;
} while (count3 < 3);

// Test loop with complex condition
let count4 = 0;
while (count4 < 10 && count4 > -1) {
    count4 = count4 + 1;
}