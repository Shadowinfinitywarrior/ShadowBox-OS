#ifndef SHADOWBOX_SYSFS_H
#define SHADOWBOX_SYSFS_H

#include "types.h"
#include "vfs.h"

typedef int64_t loff_t;

#define SYSFS_KOBJ_ATTR 0x01
#define SYSFS_KOBJ_BIN_ATTR 0x02
#define SYSFS_KOBJ_METHOD 0x04

/*
 * sysfs_attribute_t - Regular sysfs attribute
 * @name:  Attribute name
 * @mode:  File permissions string
 * @show:  Read handler
 * @store: Write handler
 * @private: Private data
 */
typedef struct sysfs_attribute {
    const char *name;
    const char *mode;
    ssize_t (*show)(struct sysfs_attribute *attr, char *buf);
    ssize_t (*store)(struct sysfs_attribute *attr, const char *buf, size_t count);
    void *private;
} sysfs_attribute_t;

/*
 * sysfs_bin_attribute_t - Binary sysfs attribute
 */
typedef struct sysfs_bin_attribute {
    const char *name;
    const char *mode;
    size_t size;
    void *private;
    ssize_t (*read)(struct sysfs_bin_attribute *attr, void *buf, size_t count, loff_t offset);
    ssize_t (*write)(struct sysfs_bin_attribute *attr, const void *buf, size_t count, loff_t offset);
} sysfs_bin_attribute_t;

/*
 * kobject_t - Kernel object embedded in sysfs hierarchy
 * @name:       Object name
 * @parent:     Parent kobject
 * @kset:       Containing kset
 * @attrs:      Regular attributes
 * @bin_attrs:  Binary attributes
 * @attr_count: Number of attributes
 * @release:    Release callback
 * @private:    Private data
 * @sysfs_dentry: Corresponding sysfs dentry
 */
typedef struct kobject {
    const char *name;
    struct kobject *parent;
    struct kobject *kset;

    sysfs_attribute_t *attrs;
    sysfs_bin_attribute_t *bin_attrs;
    int attr_count;

    void (*release)(struct kobject *kobj);
    void *private;

    vfs_node_t *sysfs_dentry;
} kobject_t;

/*
 * kset_t - A set of kobjects grouped together
 */
typedef struct kset {
    struct kobject kobj;
    struct kset *parent;
    struct kobject *list;
} kset_t;

extern vfs_node_t *sysfs_root;

/*
 * sysfs_init - Initialize sysfs filesystem
 */
void sysfs_init(void);

/*
 * kobject_init_and_add - Initialize and register a kobject
 * @kobj:   Kobject to initialize
 * @kset:   Parent kset (or NULL)
 * @parent: Parent kobject (or NULL)
 * @fmt:    Name format string
 * Returns: 0 on success, -1 on error
 */
int kobject_init_and_add(struct kobject *kobj, struct kset *kset,
                         struct kobject *parent, const char *fmt, ...);

/*
 * kobject_put - Decrement kobject reference count
 * @kobj: Kobject to release
 */
void kobject_put(struct kobject *kobj);

/*
 * kobject_get - Increment kobject reference count
 * @kobj: Kobject to reference
 * Returns: The same kobject pointer
 */
struct kobject *kobject_get(struct kobject *kobj);

/*
 * sysfs_create_file - Create a sysfs attribute file
 * @kobj: Parent kobject
 * @attr: Attribute descriptor
 * Returns: 0 on success, -1 on error
 */
int sysfs_create_file(struct kobject *kobj, const struct sysfs_attribute *attr);

/*
 * sysfs_remove_file - Remove a sysfs attribute file
 * @kobj: Parent kobject
 * @attr: Attribute to remove
 */
void sysfs_remove_file(struct kobject *kobj, const struct sysfs_attribute *attr);

/*
 * sysfs_create_bin_file - Create a binary sysfs file
 * @kobj: Parent kobject
 * @attr: Binary attribute descriptor
 * Returns: 0 on success, -1 on error
 */
int sysfs_create_bin_file(struct kobject *kobj, const struct sysfs_bin_attribute *attr);

/*
 * sysfs_remove_bin_file - Remove a binary sysfs file
 * @kobj: Parent kobject
 * @attr: Binary attribute to remove
 */
void sysfs_remove_bin_file(struct kobject *kobj, const struct sysfs_bin_attribute *attr);

/*
 * kset_create_and_add - Create and register a kset
 * @name:         Kset name
 * @parent_kset:  Parent kset
 * Returns: New kset, or NULL
 */
struct kset *kset_create_and_add(const char *name, struct kset *parent_kset);

/*
 * kset_unregister - Unregister and free a kset
 * @kset: Kset to unregister
 */
void kset_unregister(struct kset *kset);

/*
 * sysfs_create_dir - Create a directory for a kobject
 * @kobj: Kobject to create directory for
 * Returns: 0 on success, -1 on error
 */
int sysfs_create_dir(struct kobject *kobj);

/*
 * sysfs_remove_dir - Remove a kobject directory
 * @kobj: Kobject whose directory to remove
 */
void sysfs_remove_dir(struct kobject *kobj);

/*
 * sysfs_create_link - Create a sysfs symlink
 * @kobj:   Directory for the link
 * @target: Link target
 * @name:   Link name
 * Returns: 0 on success, -1 on error
 */
int sysfs_create_link(struct kobject *kobj, struct kobject *target, const char *name);

/*
 * sysfs_remove_link - Remove a sysfs symlink
 * @kobj: Directory containing the link
 * @name: Link name to remove
 */
void sysfs_remove_link(struct kobject *kobj, const char *name);

#endif
