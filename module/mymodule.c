#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/sched/signal.h>
#include <linux/fs.h>
#include <linux/fdtable.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/path.h> 
// Meta Information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Can Kadri Eltepe" );
MODULE_DESCRIPTION( "lsfd kernel module" );

static struct proc_dir_entry *proc_entry;
static pid_t target_pid = -1;

static int lsfd_show(struct seq_file *m, void *v){
    struct task_struct *task;
    struct files_struct *files;
    struct fdtable *fdt;
    int i;

    if(target_pid < 0){
        seq_puts(m, "no PID is set.\n");
        return 0;
    }
    rcu_read_lock();
    task =pid_task(find_vpid(target_pid),PIDTYPE_PID);
    if (!task){
        rcu_read_unlock();
        seq_printf(m, "PID %d not found.\n", target_pid);
        return 0;
    }

    files = task->files;
    if(!files){
          rcu_read_unlock();
        seq_printf(m, "PID %d has no files struct.\n", target_pid);
        return 0;
    }

    // fdtable accessing
    fdt = files_fdtable(files);
    if (!fdt || !fdt->fd) {
        rcu_read_unlock();
        seq_printf(m, "PID %d: fdtable missing.\n", target_pid);
        return 0;
    }

    seq_printf(m, "Process: %s[%d]\n", task->comm, target_pid);
    for(i = 0; i < fdt->max_fds; i++) {
        struct file *f = fdt->fd[i];
        char *tmp;
        char *path_str;
        const char *name_str;
        loff_t size = 0;

        if (!f) continue;

        // getting name from dentry
        name_str = f->f_path.dentry ? f->f_path.dentry->d_name.name :(const unsigned char *) "(no-dentry)";

        // getting size from inode
        if (file_inode(f))
            size = i_size_read(file_inode(f));

        // the full path string
        tmp = kmalloc(PATH_MAX, GFP_KERNEL);
        if(!tmp){
            seq_printf(m, "FD%d: name: %s  size: %lld bytes  path: (oom)\n",
                       i, name_str, (long long)size);
            continue;
        }

        path_str =d_path(&f->f_path, tmp, PATH_MAX);
        if(IS_ERR(path_str)){
            seq_printf(m, "FD%d: name: %s  size: %lld bytes  path: (error)\n",
                       i, name_str, (long long)size);
   }else{
            seq_printf(m, "FD%d: name: %s  size: %lld bytes  path: %s\n",
                       i, name_str, (long long)size, path_str);
        }

        kfree(tmp);
    }

    rcu_read_unlock();
    return 0;
}

static int lsfd_open(struct inode *inode, struct file *file)
{
    return single_open(file, lsfd_show, NULL);
}

//writing the PID into /proc/lsfd
static ssize_t lsfd_write(struct file *file, const char __user *ubuf,
                          size_t count, loff_t *ppos)
{
    char kbuf[32];
    long pid_val;

    if (count >= 32)
        return -EINVAL;

    if (copy_from_user(kbuf, ubuf, count))
        return -EFAULT;

    kbuf[count] = '\0';

    if (kstrtol(kbuf, 10, &pid_val) != 0)
        return -EINVAL;

    if (pid_val < 0)
        return -EINVAL;

    target_pid = (pid_t)pid_val;
    return count;
}

// the proc ops before creating the proc
static const struct proc_ops lsfd_proc_ops = {
    .proc_open    = lsfd_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
    .proc_write   = lsfd_write,
};
// A function that runs when the module is first loaded
static int __init simple_init(void) {
    proc_entry = proc_create("lsfd", 0666, NULL, &lsfd_proc_ops);
    if (!proc_entry) {
        return -ENOMEM;
    }
    return 0;
}


// A function that runs when the modulesi is removed
static void __exit simple_exit(void) {
    if (proc_entry)
        proc_remove(proc_entry);
}


module_init(simple_init);
module_exit(simple_exit);
