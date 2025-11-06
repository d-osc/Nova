#include "nova/runtime/Runtime.h"
#include <cstdlib>
#include <cstring>

namespace nova {
namespace runtime {

// Simple allocator implementation
static size_t total_allocated = 0;
static size_t allocation_count = 0;

void* allocate(size_t size, TypeId type_id) {
    // Allocate memory for object + header
    size_t total_size = sizeof(ObjectHeader) + size;
    void* memory = std::malloc(total_size);
    
    if (!memory) {
        panic("Out of memory");
        return nullptr;
    }
    
    // Set up header
    ObjectHeader* header = static_cast<ObjectHeader*>(memory);
    header->size = total_size;
    header->type_id = static_cast<uint32>(type_id);
    header->is_marked = false;
    header->next = nullptr;
    
    // Update statistics
    total_allocated += total_size;
    allocation_count++;
    
    // Return pointer to data area (after header)
    return static_cast<char*>(memory) + sizeof(ObjectHeader);
}

void deallocate(void* ptr) {
    if (!ptr) return;
    
    // Get pointer to header
    ObjectHeader* header = reinterpret_cast<ObjectHeader*>(
        static_cast<char*>(ptr) - sizeof(ObjectHeader)
    );
    
    // Update statistics
    total_allocated -= header->size;
    allocation_count--;
    
    // Free memory
    std::free(header);
}

size_t get_object_size(void* ptr) {
    if (!ptr) return 0;
    
    ObjectHeader* header = reinterpret_cast<ObjectHeader*>(
        static_cast<char*>(ptr) - sizeof(ObjectHeader)
    );
    
    return header->size - sizeof(ObjectHeader);
}

TypeId get_object_type(void* ptr) {
    if (!ptr) return TypeId::OBJECT;
    
    ObjectHeader* header = reinterpret_cast<ObjectHeader*>(
        static_cast<char*>(ptr) - sizeof(ObjectHeader)
    );
    
    return static_cast<TypeId>(header->type_id);
}

// Memory statistics
size_t get_total_allocated() {
    return total_allocated;
}

size_t get_allocation_count() {
    return allocation_count;
}

} // namespace runtime
} // namespace nova
