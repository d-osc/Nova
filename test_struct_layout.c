#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Match the LLVM struct layout
struct NovaObject {
    uint8_t header[24];  // ObjectHeader
    int64_t field0;
    int64_t field1;
    int64_t field2;
    int64_t field3;
    int64_t field4;
    int64_t field5;
    int64_t field6;
    int64_t field7;
};

int main() {
    // Allocate a struct
    struct NovaObject* obj = (struct NovaObject*)malloc(sizeof(struct NovaObject));

    // Store pointer values (like in the LLVM IR)
    const char* str1 = "Max";
    const char* str2 = "Golden";

    obj->field0 = (int64_t)str1;
    obj->field1 = (int64_t)str2;

    printf("Stored field0: %lld (pointer to: %s)\n", obj->field0, (char*)obj->field0);
    printf("Stored field1: %lld (pointer to: %s)\n", obj->field1, (char*)obj->field1);

    // Load them back
    int64_t loaded0 = obj->field0;
    int64_t loaded1 = obj->field1;

    printf("Loaded field0: %lld (pointer to: %s)\n", loaded0, (char*)loaded0);
    printf("Loaded field1: %lld (pointer to: %s)\n", loaded1, (char*)loaded1);

    // Also test via GEP-style access
    int64_t* field1_ptr = (int64_t*)((uint8_t*)obj + 24 + 8);  // header(24) + field0(8)
    printf("Via offset field1: %lld (pointer to: %s)\n", *field1_ptr, (char*)*field1_ptr);

    free(obj);
    return 0;
}
