#ifndef __USER_LFS_H
#define __USER_LFS_H

#include "lfs.h"

int user_provided_block_device_read(const struct lfs_config *c, lfs_block_t block,
                                    lfs_off_t off, void *buffer, lfs_size_t size);
int user_provided_block_device_prog(const struct lfs_config *c, lfs_block_t block,
                                    lfs_off_t off, const void *buffer, lfs_size_t size);
int user_provided_block_device_erase(const struct lfs_config *c, lfs_block_t block);
int user_provided_block_device_sync(const struct lfs_config *c);

#endif // !__USER_LFS_H
