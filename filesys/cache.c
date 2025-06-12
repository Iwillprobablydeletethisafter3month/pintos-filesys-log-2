//cache.c

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "devices/block.h"
#include "threads/synch.h"
#include "filesys/filesys.h"

#define CACHE_SIZE 64

struct cache_entry {
    bool valid;
    bool dirty;
    bool accessed;
    bool is_metadata;

    block_sector_t sector;

    uint8_t data[BLOCK_SECTOR_SIZE];
    struct lock lock;
};

static struct cache_entry cache[CACHE_SIZE];
static size_t clock_hand = 0;
static struct lock cache_lock;

void cache_init(void) {
    lock_init(&cache_lock); //synch.c
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache[i].valid = false;
        cache[i].dirty = false;
        cache[i].accessed = false;
        cache[i].is_metadata = false;
        lock_init(&cache[i].lock);
    }
    clock_hand = 0;
}

int cache_lookup(block_sector_t sector) {
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].valid && cache[i].sector == sector) {
            return i;
        }
    }
    return -1;
}

void cache_read(block_sector_t sector, void *buffer) {
    lock_acquire(&cache_lock);

    int idx = cache_lookup(sector);
    if (idx == -1) {
        idx = cache_evict();  // Nếu chưa có, thay thế 1 block
        block_read(fs_device, sector, cache[idx].data);  // block.c
        cache[idx].valid = true;
        cache[idx].dirty = false;
        cache[idx].sector = sector;
    }

    cache[idx].accessed = true;
    memcpy(buffer, cache[idx].data, BLOCK_SECTOR_SIZE);

    lock_release(&cache_lock);
}

void cache_flush(void) {
    lock_acquire(&cache_lock);
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].valid && cache[i].dirty) {
            block_write(fs_device, cache[i].sector, cache[i].data);
            cache[i].dirty = false;
        }
    }
    lock_release(&cache_lock);
}

int cache_evict(void) {
    while (true) {
        struct cache_entry *entry = &cache[clock_hand];

        if (!entry->valid) {
            return clock_hand;
        }

        if (entry->accessed) {
            entry->accessed = false;
        } else {
            if (!entry->is_metadata) {
                if (entry->dirty) {
                    block_write(fs_device, entry->sector, entry->data);
                    entry->dirty = false;
                }
                entry->valid = false;
                return clock_hand;
            }
        }

        clock_hand = (clock_hand + 1) % CACHE_SIZE;
    }
}

void cache_write(block_sector_t sector, const void *buffer, bool is_metadata) {
    lock_acquire(&cache_lock);

    int idx = cache_lookup(sector);
    if (idx == -1) {
        idx = cache_evict();
        block_read(fs_device, sector, cache[idx].data);
        cache[idx].valid = true;
        cache[idx].dirty = false;
        cache[idx].sector = sector;
    }

    cache[idx].accessed = true;
    cache[idx].dirty = true;
    cache[idx].is_metadata = is_metadata;

    memcpy(cache[idx].data, buffer, BLOCK_SECTOR_SIZE);

    lock_release(&cache_lock);
}
