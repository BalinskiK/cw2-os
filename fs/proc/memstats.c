#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/mm.h>

unsigned long pw_virt;
unsigned long pw_phys;

void walk_address(struct mm_struct *mm, unsigned long virt)
{
    pw_phys = 0;
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

    pgd = pgd_offset(mm, virt);
    if (pgd_none(*pgd) || pgd_bad(*pgd) || !pgd_present(*pgd))
        return;

    pud = pud_offset(pgd, virt);
    if (pud_none(*pud) || pud_bad(*pud) || !pud_present(*pud))
        return;

    pmd = pmd_offset(pud, virt);
    if (pmd_none(*pmd) || pmd_bad(*pmd) || !pmd_present(*pmd))
        return;

    if (pmd_huge(*pmd)) { /* Huge page */
        pw_phys = pmd_huge_page(*pmd)->index << PAGE_SHIFT;
        return;
    }

    pte = pte_offset_map(pmd, virt);
    if (pte_none(*pte) || !pte_present(*pte))
        return;

    pw_phys = page_to_phys(pte_page(*pte));
}

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

    // Lock the task's memory descriptor to ensure data integrity
    task_lock(task);

    // Gather memory statistics
    mm = get_task_mm(task);
    if (mm) {
        struct mm_walk walk = {
            .pmd_entry = walk_address,
            .mm = mm,
        };
        total_phys_pages = 0; // Reset the count before walking

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

            // Walk through the address space and count physical pages
            mm_walk(&walk, vma->vm_start, vma->vm_end - 1);
        };
        mmap_read_unlock(mm);
    }

    // Unlock the memory descriptor
    task_unlock(task);

    // Output virtual memory area stats...
    // Output physical memory area stats
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
