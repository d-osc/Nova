function main(): number {
    // console.group(label) - starts a new group of console messages
    // console.groupEnd() - ends the current group
    // Used for organizing hierarchical log output

    console.log("Main program start");

    // Create first group
    console.group("Database operations");
    console.log("Connecting to database...");
    console.log("Running query...");
    console.groupEnd();

    // Create nested groups
    console.group("User processing");
    console.log("Loading user data...");

    console.group("Validation");
    console.log("Validating email...");
    console.log("Validating password...");
    console.groupEnd();

    console.log("User data processed");
    console.groupEnd();

    console.log("Main program end");

    // Return success code
    return 182;
}
