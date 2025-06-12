#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include "devices/block.h"
#include <stdbool.h>
#include <stdint.h>

void cache_init(void);

int cache_lookup(block_sector_t sector);

void cache_flush(void);

void cache_read(block_sector_t sector, void *buffer);

int cache_evict(void);

void cache_write(block_sector_t sector, const void *buffer, bool is_metadata);

#endif /* filesys/cache.h */
