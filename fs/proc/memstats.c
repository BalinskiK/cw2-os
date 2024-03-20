#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/pagewalk.h>
#include <linux/pagemap.h>
#include <linux/page-flags.h>
#include <linux/pageblock-flags.h>
#include <linux/mm_types.h>


int total_phys_pages = 0;
int swapped_out_pages = 0;
int read_only_pages = 0;
int writable_pages = 0;
int shared_pages = 0;
int special_pages = 0;
int huge_pages = 0;

/* Callback function for walking PTEs */
static int pte_entry_callback(pte_t *pte, unsigned long addr,
                              unsigned long next, struct mm_walk *walk)
{
    total_phys_pages++;

    if (pte && !pte_none(*pte)) {
        struct page *page = pte_page(*pte);

        /* Check various page flags */
        if (PageSwapBacked(page)) {
            // Swapped out page
            swapped_out_pages++;
        } else if (PageReserved(page)) {
            // Special page
            special_pages++;
        } else if (PageHuge(page)) {
            // Huge page
            huge_pages++;
        } else if (PageLRU(page)) {
            // Read-only page
            read_only_pages++;
        } else {
            // Writable page
            writable_pages++;
        }
    }

    return 0; // Continue walking
}

/* Callback function for handling holes */
static int pte_hole_callback(unsigned long addr, unsigned long next,
                             int depth, struct mm_walk *walk)
{
    // Handle holes if necessary
    return 0; // Continue walking
}


int proc_pid_memstats(struct seq_file *m, struct pid_namespace *ns, struct pid *pid, struct task_struct *task) {
    struct vm_area_struct *vma;
    struct mm_struct *mm;
    struct mm_walk_ops ops;
    struct mm_walk walk;

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

    total_phys_pages = 0;
    swapped_out_pages = 0;
    read_only_pages = 0;
    writable_pages = 0;
    shared_pages = 0;
    special_pages = 0;
    huge_pages = 0;

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

            // Pagewalk to get additional page statistics
            ops = (struct mm_walk_ops) {
                .pte_entry = pte_entry_callback,
                .pte_hole = pte_hole_callback,
            };

            walk = (struct mm_walk) {
                .ops = &ops,
                .mm = mm,
                .vma = vma,
                .action = ACTION_SUBTREE,
                .no_vma = false,
                .private = NULL,
            };
               walk_page_vma(vma, &ops, NULL);
            }
            
            mmap_read_unlock(mm);
            mmput(mm);    
        };
    
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
