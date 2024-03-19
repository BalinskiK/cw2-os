#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/mm.h>

int proc_pid_memstats(struct seq_file *m, struct pid_namespace *ns, struct pid *pid, struct task_struct *task) {
    struct vm_area_struct *vma;
    struct mm_struct *mm;

    int total_vm_count = 0;
    unsigned long biggest_vma_size = 0;
    int readable_vm_count = 0;
    int writable_vm_count = 0;
    int executable_vm_count = 0;
    int shared_vm_count = 0;
    int private_vm_count = 0;
    int locked_vm_count = 0;
    int executable_image_vm_count = 0;
    int file_backed_vm_count = 0;
    int anonymous_vm_count = 0;

    int total_phys_pages = 0;
    int swapped_out_pages = 0;
    int read_only_pages = 0;
    int writable_pages = 0;
    int shared_pages = 0;
    int special_pages = 0;
    int huge_pages = 0;
    // Other memory statistics...

    // Lock the task's memory descriptor to ensure data integrity
    //task_lock(task);

    // Gather memory statistics
    mm = get_task_mm(task);

    task_lock(task);
    if (mm) {
        VMA_ITERATOR(vmi, mm, 0);
        mmap_read_lock(mm);
        for_each_vma(vmi, vma) {
            // Count different types of VMAs
            total_vm_count++;
            if (vma->vm_end - vma->vm_start > biggest_vma_size)
                biggest_vma_size = vma->vm_end - vma->vm_start;
            if (vma->vm_flags & VM_READ)
                readable_vm_count++;
            if (vma->vm_flags & VM_WRITE)
                writable_vm_count++;
            if (vma->vm_flags & VM_EXEC)
                executable_vm_count++;
            if (vma->vm_flags & VM_SHARED)
                shared_vm_count++;
            else
                private_vm_count++;
            if (vma->vm_flags & VM_LOCKED)
                locked_vm_count++;
            if (vma->vm_file && vma->vm_flags & VM_EXEC)
                executable_image_vm_count++;
            if (vma->vm_file)
                file_backed_vm_count++;
            else
                anonymous_vm_count++;
        };
        mmap_read_unlock(mm);

        // Count physical pages .special_pages = get_mm_counter(mm, MM_SPECIALPAGES);
        total_phys_pages = get_mm_rss(mm);
        swapped_out_pages = get_mm_counter(mm, MM_SWAPENTS);
        read_only_pages = get_mm_counter(mm, MM_ANONPAGES);
        writable_pages = get_mm_counter(mm, MM_FILEPAGES);
        shared_pages = get_mm_counter(mm, MM_SHMEMPAGES);
        
        huge_pages = get_mm_counter(mm, MM_FILEPAGES);
    }

    // Unlock the memory descriptor
    task_unlock(task);

    seq_printf(m, "Virtual Memory Area Stats:\n");
    seq_printf(m, "Total VMAs: %d\n", total_vm_count);
    seq_printf(m, "Biggest VMA Size: %lu\n", biggest_vma_size);
    seq_printf(m, "Readable VMAs: %d\n", readable_vm_count);
    seq_printf(m, "Writable VMAs: %d\n", writable_vm_count);
    seq_printf(m, "Executable VMAs: %d\n", executable_vm_count);
    seq_printf(m, "Shared VMAs: %d\n", shared_vm_count);
    seq_printf(m, "Private VMAs: %d\n", private_vm_count);
    seq_printf(m, "Locked VMAs: %d\n", locked_vm_count);
    seq_printf(m, "Executable Image VMAs: %d\n", executable_image_vm_count);
    seq_printf(m, "File Backed VMAs: %d\n", file_backed_vm_count);
    seq_printf(m, "Anonymous VMAs: %d\n", anonymous_vm_count);

    seq_printf(m, "Physical Pages Stats:\n");
    seq_printf(m, "Total Physical Pages: %d\n", total_phys_pages);
    seq_printf(m, "Number of Pages Swapped Out: %d\n", swapped_out_pages);
    seq_printf(m, "Read-Only Pages: %d\n", read_only_pages);
    seq_printf(m, "Writable Pages: %d\n", writable_pages);
    seq_printf(m, "Number of Shared Pages: %d\n", shared_pages);
    seq_printf(m, "Number of Special Pages: %d\n", special_pages);
    seq_printf(m, "Number of Huge Pages: %d\n", huge_pages);

    return 0;
}
