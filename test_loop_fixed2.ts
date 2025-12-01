// Test while loop
let i = 0;
while (i < 5) {
    i = i + 1;
}

// Test for loop with continue and break
let j = 0;
for (j = 0; j < 3; j = j + 1) {
    if (j === 1) {
        continue;
    }
    if (j === 2) {
        break;
    }
}

// Test do-while loop
let k = 0;
do {
    k = k + 1;
} while (k < 3);

// Test nested loops
let l = 0;
let m = 0;
while (l < 3) {
    l = l + 1;
    while (m < 3) {
        m = m + 1;
        if (m === 1) {
            break;
        }
    }
}

// Test loop with complex condition
let n = 0;
while (n < 10 && n > -1) {
    n = n + 1;
}