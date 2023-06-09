#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x32e21920, "module_layout" },
	{ 0xe116e76a, "crypto_alloc_skcipher" },
	{ 0xebdf7b85, "crypto_rng_reset" },
	{ 0x4a165127, "kobject_put" },
	{ 0x766a0927, "mempool_alloc_pages" },
	{ 0xecea0910, "kmem_cache_destroy" },
	{ 0x26087692, "kmalloc_caches" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x39644375, "submit_bio_wait" },
	{ 0xe76391f1, "bio_alloc_bioset" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0x754d539c, "strlen" },
	{ 0x8ef378f, "dm_get_device" },
	{ 0x41ed3709, "get_random_bytes" },
	{ 0xe6e43146, "dm_table_get_mode" },
	{ 0x6bd0e573, "down_interruptible" },
	{ 0x837b7b09, "__dynamic_pr_debug" },
	{ 0x9034a696, "mempool_destroy" },
	{ 0x647a9097, "bioset_init" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0xbd4b4983, "bio_advance" },
	{ 0x999e8297, "vfree" },
	{ 0x6c96b4a0, "dm_register_target" },
	{ 0x97651e6c, "vmemmap_base" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x6fabae87, "kobject_create_and_add" },
	{ 0x2f9c1587, "crypto_alloc_rng" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xc972449f, "mempool_alloc_slab" },
	{ 0x25974000, "wait_for_completion" },
	{ 0xfb578fc5, "memset" },
	{ 0xb9392801, "device_register" },
	{ 0x6a6e05bf, "kstrtou8" },
	{ 0x89940875, "mutex_lock_interruptible" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0x449ad0a7, "memcmp" },
	{ 0x6eaca5a3, "bio_add_page" },
	{ 0x50759163, "kobject_init_and_add" },
	{ 0x844acf45, "bio_clone_fast" },
	{ 0x63876666, "dm_unregister_target" },
	{ 0xd0760fc0, "kfree_sensitive" },
	{ 0xd985dc99, "mempool_free_pages" },
	{ 0x670ecece, "__x86_indirect_thunk_rbx" },
	{ 0x30a0594d, "kmem_cache_free" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0xea34c07e, "sysfs_remove_link" },
	{ 0x8a99a016, "mempool_free_slab" },
	{ 0xf53705fe, "__root_device_register" },
	{ 0x448bb25a, "crypto_req_done" },
	{ 0x3794a4d3, "crypto_stats_rng_generate" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x23e0dc2e, "bio_endio" },
	{ 0x5778ee02, "bio_put" },
	{ 0xc490858f, "device_create_file" },
	{ 0x800473f, "__cond_resched" },
	{ 0x64e8300b, "bioset_exit" },
	{ 0x31503a78, "sysfs_create_link" },
	{ 0x7cd8d75e, "page_offset_base" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0x32efb85e, "submit_bio" },
	{ 0xc9eb5b45, "kmem_cache_alloc" },
	{ 0xc3762aec, "mempool_alloc" },
	{ 0x4b046f43, "root_device_unregister" },
	{ 0xcc5e3c19, "crypto_skcipher_decrypt" },
	{ 0xeb47d6f4, "put_device" },
	{ 0xb855e652, "dm_accept_partial_bio" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0x1000e51, "schedule" },
	{ 0x1953c958, "mempool_create" },
	{ 0x92997ed8, "_printk" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0x3445550c, "crypto_destroy_tfm" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xf35141b2, "kmem_cache_alloc_trace" },
	{ 0xa897e3e7, "mempool_free" },
	{ 0xa38e29b5, "kmem_cache_create" },
	{ 0x51a72807, "crypto_skcipher_setkey" },
	{ 0x3eeb2322, "__wake_up" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0xb320cc0e, "sg_init_one" },
	{ 0x37a0cba, "kfree" },
	{ 0xcf2a6966, "up" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x608741b5, "__init_swait_queue_head" },
	{ 0xa85f4e93, "device_unregister" },
	{ 0xbe972bec, "dm_put_device" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x3ee0e1fa, "crypto_stats_get" },
	{ 0xcf01184e, "dev_set_name" },
	{ 0xb293625b, "sysfs_create_file_ns" },
	{ 0x49cd25ed, "alloc_workqueue" },
	{ 0xc8dcc62a, "krealloc" },
	{ 0xe914e41e, "strcpy" },
	{ 0x4259b566, "bio_associate_blkg" },
	{ 0xb33b7e12, "crypto_skcipher_encrypt" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "C8AB7F7CEB11A10A32AD201");
