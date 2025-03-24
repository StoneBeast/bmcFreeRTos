/*** 
 * @Author       : stoneBeast
 * @Date         : 2025-03-21 15:02:24
 * @Encoding     : UTF-8
 * @LastEditors  : stoneBeast
 * @LastEditTime : 2025-03-21 15:37:28
 * @Description  : 
 */
#include "user_lfs.h"
#include "logStore.h"

int user_provided_block_device_read(const struct lfs_config *c, lfs_block_t block,
                                    lfs_off_t off, void *buffer, lfs_size_t size)
{
    W25QXX_result_t w_res;
    w_res = w25qxx_read(&w25qxx, (block*(c->block_size)) + off, buffer, size);

    if (w_res == W25QXX_Ok)
        return LFS_ERR_OK;

    return LFS_ERR_IO;
}

int user_provided_block_device_prog(const struct lfs_config *c, lfs_block_t block,
                                    lfs_off_t off, const void *buffer, lfs_size_t size)
{
    W25QXX_result_t w_res;
    w_res = w25qxx_write(&w25qxx, (block * (c->block_size)) + off, buffer, size);

    if (w_res == W25QXX_Ok)
        return LFS_ERR_OK;

    return LFS_ERR_IO;
}

int user_provided_block_device_erase(const struct lfs_config *c, lfs_block_t block)
{
    W25QXX_result_t w_res;
    w_res = w25qxx_erase(&w25qxx, (block * (c->block_size)), c->block_size);

    if (w_res == W25QXX_Ok)
        return LFS_ERR_OK;

    return LFS_ERR_IO;
}

int user_provided_block_device_sync(const struct lfs_config *c)
{
    return LFS_ERR_OK;
}
