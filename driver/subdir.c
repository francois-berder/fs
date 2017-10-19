#include <string.h>
#include "fat16.h"
#include "fat16_priv.h"
#include "subdir.h"

extern struct storage_dev_t dev;
extern struct fat16_layout layout;
extern struct fat16_bpb bpb;

static int find_entry_in_subdir(struct file_handle *handle, struct dir_entry *entry, char *name)
{
    int ret = -1;
    uint32_t starting_cluster = handle->cluster;

    while (read_from_handle(handle, entry, sizeof(struct dir_entry)) > 0) {
        /* Skip available entry */
        if ((uint8_t)(entry->name[0]) == ROOT_DIR_AVAILABLE_ENTRY)
        continue;

        /* Do not allow filename to start with a NULL character */
        if (entry->name[0] == 0)
            continue;

        /* Ignore any VFAT entry */
        if ((entry->attribute & ROOT_DIR_VFAT_ENTRY) == ROOT_DIR_VFAT_ENTRY)
            continue;

        if (memcmp(name, entry->name, sizeof(entry->name)) == 0) {
            ret = 0;
            break;
        }
    }

    /* Restore state of handle */
    handle->cluster = starting_cluster;
    handle->offset = 0;

    return ret;
}

static int find_available_entry_in_subdir(uint32_t *entry_pos, struct file_handle *handle)
{
    int ret = -1;
    struct dir_entry entry;
    uint32_t starting_cluster = handle->cluster;

    /* Check if there is some space in the entry list */
    while (read_from_handle(handle, &entry, sizeof(entry)) == sizeof(entry)) {
        uint8_t tmp = entry.name[0];
        if (tmp == 0 || tmp == ROOT_DIR_AVAILABLE_ENTRY) {
            *entry_pos = move_to_data_region(handle->cluster, handle->offset);
            *entry_pos -= sizeof(entry);
            ret = 0;
            break;
        }
    }

    /*
     * If there is no space in the entry list, append a dummy entry to the
     * entry list.
     */
    if (ret < 0) {
        memset(&entry, 0, sizeof(entry));
        if (write_from_handle(handle, &entry, sizeof(entry)) == sizeof(entry))
            ret = 0;
    }

    if (ret == 0) {
        *entry_pos = move_to_data_region(handle->cluster, handle->offset);
        *entry_pos -= sizeof(entry);
    }

    /* Restore previous state of handle */
    handle->cluster = starting_cluster;
    handle->offset = 0;

    return ret;
}

int create_file_in_subdir(struct file_handle *handle, char *filename)
{
    struct dir_entry entry;
    uint32_t entry_pos;

    /* Do not allow muliple entries with same name */
    if (find_entry_in_subdir(handle, &entry, filename) == 0)
        return -1;

    /* Try to find an available entry in the current entry list */
    if (find_available_entry_in_subdir(&entry_pos, handle) < 0)
        return -1;

    memcpy(entry.name, filename, sizeof(entry.name));
    entry.attribute = 0;
    memset(entry.reserved, 0, sizeof(entry.reserved));
    memset(entry.time, 0, sizeof(entry.time));
    memset(entry.date, 0, sizeof(entry.date));
    entry.starting_cluster = 0;
    entry.size = 0;
    dev.seek(entry_pos);
    dev.write(&entry, sizeof(entry));

    return 0;
}

int create_directory_in_subdir(struct file_handle *handle, char *dirname)
{
    struct dir_entry entry;
    uint32_t entry_pos;
    uint32_t parent_dir_starting_cluster = handle->cluster;

    /* Do not allow muliple entries with same name */
    if (find_entry_in_subdir(handle, &entry, dirname) == 0)
        return -1;

    /* Try to find an available entry in the current entry list */
    if (find_available_entry_in_subdir(&entry_pos, handle) < 0)
        return -1;

    memcpy(entry.name, dirname, sizeof(entry.name));
    entry.attribute = SUBDIR;
    memset(entry.reserved, 0, sizeof(entry.reserved));
    memset(entry.time, 0, sizeof(entry.time));
    memset(entry.date, 0, sizeof(entry.date));
    if (allocate_cluster(&entry.starting_cluster, 0) < 0) {
        return -1;
    }
    entry.size = 0;
    dev.seek(entry_pos);
    dev.write(&entry, sizeof(entry));

    move_to_data_region(entry.starting_cluster, 0);

    /* Create "."" entry */
    {
        struct dir_entry e;
        memset(&e, 0, sizeof(e));

        e.name[0] = '.';
        memset(&e.name[1], ' ', sizeof(e.name) - 1);
        e.starting_cluster = entry.starting_cluster;
        e.attribute = SUBDIR;
        dev.write(&e, sizeof(e));
    }

    /* Create ".."" entry */
    {
        struct dir_entry e;
        memset(&e, 0, sizeof(e));

        e.name[0] = '.';
        e.name[1] = '.';
        memset(&e.name[2], ' ', sizeof(e.name) - 2);
        e.starting_cluster = parent_dir_starting_cluster;
        e.attribute = SUBDIR;

        dev.write(&e, sizeof(e));
    }

    /* Add dummy entry to indicate end of entry list */
    {
        struct dir_entry e;
        memset(&e, 0, sizeof(e));
        dev.write(&e, sizeof(e));
    }

    return 0;
}

static int open_entry_in_subdir(struct file_handle *handle, char *name, bool read_mode, bool is_file)
{
    struct dir_entry entry;

    if (find_entry_in_subdir(handle, &entry, name) < 0)
        return -1;

    /* Check that we are opening a file or directory and not something else */
    if (entry.attribute & VOLUME || entry.attribute & SYSTEM)
        return -1;

    if (is_file && entry.attribute & SUBDIR)
        return -1;

    if ((entry.attribute & READ_ONLY) && read_mode != READ_MODE)
        return -1;

    memcpy(handle->filename, name, sizeof(handle->filename));
    handle->read_mode = read_mode;
    handle->pos_entry = move_to_data_region(handle->cluster, handle->offset);
    handle->cluster = entry.starting_cluster;
    handle->offset = 0;
    handle->remaining_bytes = entry.size;

    return 0;
}

int open_file_in_subdir(struct file_handle *handle, char *filename, bool read_mode)
{
    return open_entry_in_subdir(handle, filename, read_mode, true);
}

int open_directory_in_subdir(struct file_handle *handle, char *dirname)
{
    return open_entry_in_subdir(handle, dirname, true, false);
}
