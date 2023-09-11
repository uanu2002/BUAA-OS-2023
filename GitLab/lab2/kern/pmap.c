#include <drivers/dev_mp.h>
#include <env.h>
#include <mmu.h>
#include <pmap.h>
#include <printk.h>

/* These variables are set by mips_detect_memory() */
static u_long memsize; /* Maximum physical address */
u_long npage;	       /* Amount of memory(in pages) */

Pde *cur_pgdir;

struct Page *pages;
static u_long freemem;

struct Page_list page_free_list; /* Free list of physical pages */

/* Overview:
 *   Read memory size from DEV_MP to initialize 'memsize' and calculate the corresponding 'npage'
 *   value.
 */
void mips_detect_memory() {
	/* Step 1: Initialize memsize. */
	memsize = *(volatile u_int *)(KSEG1 | DEV_MP_ADDRESS | DEV_MP_MEMORY);

	/* Step 2: Calculate the corresponding 'npage' value. */
	/* Exercise 2.1: Your code here. */
    npage = memsize >> PGSHIFT;
	printk("Memory size: %lu KiB, number of pages: %lu\n", memsize / 1024, npage);
}

/* Overview:
    Allocate `n` bytes physical memory with alignment `align`, if `clear` is set, clear the
    allocated memory.
    This allocator is used only while setting up virtual memory system.
   Post-Condition:
    If we're out of memory, should panic, else return this address of memory we have allocated.*/
void *alloc(u_int n, u_int align, int clear) {
	extern char end[];
	u_long alloced_mem;

	/* Initialize `freemem` if this is the first time. The first virtual address that the
	 * linker did *not* assign to any kernel code or global variables. */
	if (freemem == 0) {
		freemem = (u_long)end; // end
	}

	/* Step 1: Round up `freemem` up to be aligned properly */
	freemem = ROUND(freemem, align);

	/* Step 2: Save current value of `freemem` as allocated chunk. */
	alloced_mem = freemem;

	/* Step 3: Increase `freemem` to record allocation. */
	freemem = freemem + n;

	// Panic if we're out of memory.
	panic_on(PADDR(freemem) >= memsize);

	/* Step 4: Clear allocated chunk if parameter `clear` is set. */
	if (clear) {
		memset((void *)alloced_mem, 0, n);
	}

	/* Step 5: return allocated chunk. */
	return (void *)alloced_mem;
}

/* Overview:
    Set up two-level page table.
   Hint:
    You can get more details about `UPAGES` and `UENVS` in include/mmu.h. */
void mips_vm_init() {
	/* Allocate proper size of physical memory for global array `pages`,
	 * for physical memory management. Then, map virtual address `UPAGES` to
	 * physical address `pages` allocated before. For consideration of alignment,
	 * you should round up the memory size before map. */
	pages = (struct Page *)alloc(npage * sizeof(struct Page), BY2PG, 1);
	printk("to memory %x for struct Pages.\n", freemem);
	printk("pmap.c:\t mips vm init success\n");
}

/* Overview:
 *   Initialize page structure and memory free list. The 'pages' array has one 'struct Page' entry
 * per physical page. Pages are reference counted, and free pages are kept on a linked list.
 *
 * Hint: Use 'LIST_INSERT_HEAD' to insert free pages to 'page_free_list'.
 */
void page_init(void) {
	/* Step 1: Initialize page_free_list. */
	/* Hint: Use macro `LIST_INIT` defined in include/queue.h. */
	/* Exercise 2.3: Your code here. (1/4) */
    LIST_INIT(&page_free_list);
	/* Step 2: Align `freemem` up to multiple of BY2PG. */
	/* Exercise 2.3: Your code here. (2/4) */
    freemem = ROUND(freemem, BY2PG);
	/* Step 3: Mark all memory below `freemem` as used (set `pp_ref` to 1) */
	/* Exercise 2.3: Your code here. (3/4) */
    int num_below = PADDR(freemem) / BY2PG;
    int i;
    for(i = 0; i < num_below; i++)
    {
        pages[i].pp_ref = 1;
    }
	/* Step 4: Mark the other memory as free. */
	/* Exercise 2.3: Your code here. (4/4) */
    for(;i < npage; ++i)
    {
        pages[i].pp_ref = 0;
        LIST_INSERT_HEAD(&page_free_list, pages + i, pp_link);
    }

}

/* Overview:
 *   Allocate a physical page from free memory, and fill this page with zero.
 *
 * Post-Condition:
 *   If failed to allocate a new page (out of memory, there's no free page), return -E_NO_MEM.
 *   Otherwise, set the address of the allocated 'Page' to *pp, and return 0.
 *
 * Note:
 *   This does NOT increase the reference count 'pp_ref' of the page - the caller must do these if
 *   necessary (either explicitly or via page_insert).
 *
 * Hint: Use LIST_FIRST and LIST_REMOVE defined in include/queue.h.
 */
int page_alloc(struct Page **new) {
	/* Step 1: Get a page from free memory. If fails, return the error code.*/
	struct Page *pp;
	/* Exercise 2.4: Your code here. (1/2) */
    if (LIST_EMPTY(&page_free_list))
    {
        return -E_NO_MEM;
    }
    pp = LIST_FIRST(&page_free_list);
	LIST_REMOVE(pp, pp_link);

	/* Step 2: Initialize this page with zero.
	 * Hint: use `memset`. */
	/* Exercise 2.4: Your code here. (2/2) */
    memset(page2kva(pp), 0, BY2PG);
	*new = pp;
	return 0;
}

/* Overview:
 *   Release a page 'pp', mark it as free.
 *
 * Pre-Condition:
 *   'pp->pp_ref' is '0'.
 */
void page_free(struct Page *pp) {
	assert(pp->pp_ref == 0);
	/* Just insert it into 'page_free_list'. */
	/* Exercise 2.5: Your code here. */
    LIST_INSERT_HEAD(&page_free_list, pp, pp_link);
}

/* Overview:
 *   Given 'pgdir', a pointer to a page directory, 'pgdir_walk' returns a pointer to the page table
 *   entry (with permission PTE_D|PTE_V) for virtual address 'va'.
 *
 * Pre-Condition:
 *   'pgdir' is a two-level page table structure.
 *
 * Post-Condition:
 *   If we're out of memory, return -E_NO_MEM.
 *   Otherwise, we get the page table entry, store
 *   the value of page table entry to *ppte, and return 0, indicating success.
 *
 * Hint:
 *   We use a two-level pointer to store page table entry and return a state code to indicate
 *   whether this function succeeds or not.
 */
static int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte) {
	Pde *pgdir_entryp;
	struct Page *pp;

	/* Step 1: Get the corresponding page directory entry. */
	/* Exercise 2.6: Your code here. (1/3) */
    pgdir_entryp = pgdir + PDX(va);
	/* Step 2: If the corresponding page table is not existent (valid) and parameter `create`
	 * is set, create one. Set the permission bits 'PTE_D | PTE_V' for this new page in the
	 * page directory.
	 * If failed to allocate a new page (out of memory), return the error. */
	/* Exercise 2.6: Your code here. (2/3) */
    if(!(*pgdir_entryp & PTE_V))
    {
        if(create)
        {
            int ret = page_alloc(&pp);
            if(ret < 0)
                return ret;
            *pgdir_entryp = page2pa(pp) | PTE_D | PTE_V;
            pp->pp_ref++;
        }
        else
        {
            *ppte = 0;
            return 0;
        }
    }
    *ppte = (Pte *) KADDR(PTE_ADDR(*pgdir_entryp)) + PTX(va);

    return 0;


	/* Step 3: Assign the kernel virtual address of the page table entry to '*ppte'. */
	/* Exercise 2.6: Your code here. (3/3) */

	return 0;
}

/* Overview:
 *   Map the physical page 'pp' at virtual address 'va'. The permission (the low 12 bits) of the
 *   page table entry should be set to 'perm|PTE_V'.
 *
 * Post-Condition:
 *   Return 0 on success
 *   Return -E_NO_MEM, if page table couldn't be allocated
 *
 * Hint:
 *   If there is already a page mapped at `va`, call page_remove() to release this mapping.
 *   The `pp_ref` should be incremented if the insertion succeeds.
 */
int page_insert(Pde *pgdir, u_int asid, struct Page *pp, u_long va, u_int perm) {
	Pte *pte;

	/* Step 1: Get corresponding page table entry. */
	pgdir_walk(pgdir, va, 0, &pte);

	if (pte && (*pte & PTE_V)) {
		if (pa2page(*pte) != pp) {
			page_remove(pgdir, asid, va);
		} else {
			tlb_invalidate(asid, va);
			*pte = page2pa(pp) | perm | PTE_V;
			return 0;
		}
	}

	/* Step 2: Flush TLB with 'tlb_invalidate'. */
	/* Exercise 2.7: Your code here. (1/3) */
    tlb_invalidate(asid, va);
	/* Step 3: Re-get or create the page table entry. */
	/* If failed to create, return the error. */
	/* Exercise 2.7: Your code here. (2/3) */
    int ret = pgdir_walk(pgdir, va, 1, &pte);
    if(ret < 0)
        return ret;
	/* Step 4: Insert the page to the page table entry with 'perm | PTE_V' and increase its
	 * 'pp_ref'. */
	/* Exercise 2.7: Your code here. (3/3) */
    *pte = page2pa(pp) | perm | PTE_V;
    pp->pp_ref++;

	return 0;
}

/*Overview:
    Look up the Page that virtual address `va` map to.
  Post-Condition:
    Return a pointer to corresponding Page, and store it's page table entry to *ppte.
    If `va` doesn't mapped to any Page, return NULL.*/
struct Page *page_lookup(Pde *pgdir, u_long va, Pte **ppte) {
	struct Page *pp;
	Pte *pte;

	/* Step 1: Get the page table entry. */
	pgdir_walk(pgdir, va, 0, &pte);

	/* Hint: Check if the page table entry doesn't exist or is not valid. */
	if (pte == NULL || (*pte & PTE_V) == 0) {
		return NULL;
	}

	/* Step 2: Get the corresponding Page struct. */
	/* Hint: Use function `pa2page`, defined in include/pmap.h . */
	pp = pa2page(*pte);
	if (ppte) {
		*ppte = pte;
	}

	return pp;
}

/* Overview:
 *   Decrease the 'pp_ref' value of Page 'pp'.
 *   When there's no references (mapped virtual address) to this page, release it.
 */
void page_decref(struct Page *pp) {
	assert(pp->pp_ref > 0);

	/* If 'pp_ref' reaches to 0, free this page. */
	if (--pp->pp_ref == 0) {
		page_free(pp);
	}
}

// Overview:
//   Unmap the physical page at virtual address 'va'.
void page_remove(Pde *pgdir, u_int asid, u_long va) {
	Pte *pte;

	/* Step 1: Get the page table entry, and check if the page table entry is valid. */
	struct Page *pp = page_lookup(pgdir, va, &pte);
	if (pp == NULL) {
		return;
	}

	/* Step 2: Decrease reference count on 'pp'. */
	page_decref(pp);

	/* Step 3: Flush TLB. */
	*pte = 0;
	tlb_invalidate(asid, va);
	return;
}

/* Overview:
 *   Invalidate the TLB entry with specified 'asid' and virtual address 'va'.
 *
 * Hint:
 *   Construct a new Entry HI and call 'tlb_out' to flush TLB.
 *   'tlb_out' is defined in mm/tlb_asm.S
 */
void tlb_invalidate(u_int asid, u_long va) {
	tlb_out(PTE_ADDR(va) | (asid << 6));
}

void physical_memory_manage_check(void) {
	struct Page *pp, *pp0, *pp1, *pp2;
	struct Page_list fl;
	int *temp;

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	// now this page_free list must be empty!!!!
	LIST_INIT(&page_free_list);
	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	temp = (int *)page2kva(pp0);
	// write 1000 to pp0
	*temp = 1000;
	// free pp0
	page_free(pp0);
	printk("The number in address temp is %d\n", *temp);

	// alloc again
	assert(page_alloc(&pp0) == 0);
	assert(pp0);

	// pp0 should not change
	assert(temp == (int *)page2kva(pp0));
	// pp0 should be zero
	assert(*temp == 0);

	page_free_list = fl;
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);
	struct Page_list test_free;
	struct Page *test_pages;
	test_pages = (struct Page *)alloc(10 * sizeof(struct Page), BY2PG, 1);
	LIST_INIT(&test_free);
	// LIST_FIRST(&test_free) = &test_pages[0];
	int i, j = 0;
	struct Page *p, *q;
	for (i = 9; i >= 0; i--) {
		test_pages[i].pp_ref = i;
		// test_pages[i].pp_link=NULL;
		// printk("0x%x  0x%x\n",&test_pages[i], test_pages[i].pp_link.le_next);
		LIST_INSERT_HEAD(&test_free, &test_pages[i], pp_link);
		// printk("0x%x  0x%x\n",&test_pages[i], test_pages[i].pp_link.le_next);
	}
	p = LIST_FIRST(&test_free);
	int answer1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	assert(p != NULL);
	while (p != NULL) {
		// printk("%d %d\n",p->pp_ref,answer1[j]);
		assert(p->pp_ref == answer1[j++]);
		// printk("ptr: 0x%x v: %d\n",(p->pp_link).le_next,((p->pp_link).le_next)->pp_ref);
		p = LIST_NEXT(p, pp_link);
	}
	// insert_after test
	int answer2[] = {0, 1, 2, 3, 4, 20, 5, 6, 7, 8, 9};
	q = (struct Page *)alloc(sizeof(struct Page), BY2PG, 1);
	q->pp_ref = 20;

	// printk("---%d\n",test_pages[4].pp_ref);
	LIST_INSERT_AFTER(&test_pages[4], q, pp_link);
	// printk("---%d\n",LIST_NEXT(&test_pages[4],pp_link)->pp_ref);
	p = LIST_FIRST(&test_free);
	j = 0;
	// printk("into test\n");
	while (p != NULL) {
		//      printk("%d %d\n",p->pp_ref,answer2[j]);
		assert(p->pp_ref == answer2[j++]);
		p = LIST_NEXT(p, pp_link);
	}

	printk("physical_memory_manage_check() succeeded\n");
}

void page_check(void) {
	Pde *boot_pgdir = alloc(BY2PG, BY2PG, 1);
	struct Page *pp, *pp0, *pp1, *pp2;
	struct Page_list fl;

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	// now this page_free list must be empty!!!!
	LIST_INIT(&page_free_list);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// there is no free memory, so we can't allocate a page table
	assert(page_insert(boot_pgdir, 0, pp1, 0x0, 0) < 0);

	// free pp0 and try again: pp0 should be used for page table
	page_free(pp0);
	assert(page_insert(boot_pgdir, 0, pp1, 0x0, 0) == 0);
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));

	printk("va2pa(boot_pgdir, 0x0) is %x\n", va2pa(boot_pgdir, 0x0));
	printk("page2pa(pp1) is %x\n", page2pa(pp1));
	//  printk("pp1->pp_ref is %d\n",pp1->pp_ref);
	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	assert(pp1->pp_ref == 1);

	// should be able to map pp2 at BY2PG because pp0 is already allocated for page table
	assert(page_insert(boot_pgdir, 0, pp2, BY2PG, 0) == 0);
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	printk("start page_insert\n");
	// should be able to map pp2 at BY2PG because it's already there
	assert(page_insert(boot_pgdir, 0, pp2, BY2PG, 0) == 0);
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// pp2 should NOT be on the free list
	// could happen in ref counts are handled sloppily in page_insert
	assert(page_alloc(&pp) == -E_NO_MEM);

	// should not be able to map at PDMAP because need free page for page table
	assert(page_insert(boot_pgdir, 0, pp0, PDMAP, 0) < 0);

	// insert pp1 at BY2PG (replacing pp2)
	assert(page_insert(boot_pgdir, 0, pp1, BY2PG, 0) == 0);

	// should have pp1 at both 0 and BY2PG, pp2 nowhere, ...
	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp1));
	// ... and ref counts should reflect this
	assert(pp1->pp_ref == 2);
	printk("pp2->pp_ref %d\n", pp2->pp_ref);
	assert(pp2->pp_ref == 0);
	printk("end page_insert\n");

	// pp2 should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp2);

	// unmapping pp1 at 0 should keep pp1 at BY2PG
	page_remove(boot_pgdir, 0, 0x0);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp1));
	assert(pp1->pp_ref == 1);
	assert(pp2->pp_ref == 0);

	// unmapping pp1 at BY2PG should free it
	page_remove(boot_pgdir, 0, BY2PG);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, BY2PG) == ~0);
	assert(pp1->pp_ref == 0);
	assert(pp2->pp_ref == 0);

	// so it should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// forcibly take pp0 back
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));
	boot_pgdir[0] = 0;
	assert(pp0->pp_ref == 1);
	pp0->pp_ref = 0;

	// give free list back
	page_free_list = fl;

	// free the pages we took
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);

	printk("page_check() succeeded!\n");
}

/*
## 页面保护

如果一个页面处于空闲状态，那么可以利用 page_protect(pp) 函数使得页面 pp 被保护，被保护的页面永远不能再次被 page_alloc 分配出去

同时实现 page_status_query(pp) 函数，支持查询页面 pp 是否处于保护状态

考察要点 struct Page 结构体的内容 空闲页面的链式管理方法 如何遍历 page_free_list （ LIST_FOREACH 的用法） page_alloc 分配空闲页面的工作原理

 解答 首先我们考虑给 struct Page 结构体增加一个字段，表示该页面是否被保护

```C
struct Page {
     Page_LIST_entry_t pp_link;  /* free list link */
 
     // Ref is the count of pointers (usually in page table entries)
     // to this page.  This only holds for pages allocated using
     // page_alloc.  Pages allocated at boot time using pmap.c's "alloc"
     // do not have valid reference count fields.
     u_short pp_protect;	// Lab 2-1 Exam
     u_short pp_ref;
};
```

然后考虑在 page_alloc 分配页面的时候，避免分配 pp_protect 值为 1 的页面

```
int page_alloc(struct Page **pp)
{   
     struct Page *ppage_temp;
     if (LIST_EMPTY(&page_free_list)) return -E_NO_MEM;
     while (1) {
          ppage_temp = LIST_FIRST(&page_free_list);
          LIST_REMOVE(ppage_temp, pp_link);
          if (ppage_temp->pp_protect == 1) continue;
          bzero(page2kva(ppage_temp), BY2PG);
          *pp = ppage_temp;
          break;
     }
     return 0;
 }
```

注意到这里我们的处理方式是遇到一个 pp_protect 的页面就把他从空闲链表中移除，因为按照题意，这个页面永远不会再使用 然后就是加保护和判断，这部分就不难了

```
// Lab 2-1 Exam
int page_protect(struct Page *pp) {
     struct Page *page_i;
     int is_free = 0;
     int is_protect = 0;
     if (!LIST_EMPTY(&page_free_list)) {
         LIST_FOREACH(page_i, &page_free_list, pp_link) {
             if (page_i == pp) {
                 is_free = 1;
                 break;
             }
         }
     }
     if (pp->pp_protect == 1) is_protect = 1;
     if (is_protect == 1) return -2;
     else if(is_protect == 0 && is_free == 0) return -1;
     else if(is_protect == 0 && is_free == 1) {
         pp->pp_protect = 1;
         return 0;
     }
}

int page_status_query(struct Page *pp) {
    struct Page *page_i;
    int is_free = 0;
    int is_protect = 0;
    if (!LIST_EMPTY(&page_free_list)) {
        LIST_FOREACH(page_i, &page_free_list, pp_link) {
            if (page_i == pp) {
                is_free = 1;
                break;
            }   
        }   
    }   
    if (pp->pp_protect == 1) is_protect = 1;
    if (is_protect == 1) return 3;
    else if(is_protect == 0 && is_free == 1) return 2;
    else return 1;
}
```

## 页表查询

根据给出的物理页查找有多少虚拟页被映射到了该物理页

考察知识点有： 如何根据页表查找页面物理地址 KADDR ， PTE_ADDR ， page2pa 等函数的英勇

几个注意点： 需要根据 pgtable_entry | PTE_V 页表页是否有效

根据页表自映射相关知识，页表自身所在页面已经包含在了页目录中，因此无需特殊判断 判断时语句是唯一难点， page2pa(pp) == PTE_ADDR(*pgtable_entry)

```
// Lab 2-2 Exam
int inverted_page_lookup(Pde *pgdir, struct Page *pp, int vpn_buffer[]) {
    int i, j;
    int tot = 0;
    for (i = 0; i < 1024; i++) {
        Pde *pgdir_entry = pgdir + i;
        if ((*pgdir_entry) & PTE_V) {
            Pte *pgtable = KADDR(PTE_ADDR(*pgdir_entry));
            for (j = 0; j < 1024; j++) {
                Pte *pgtable_entry = pgtable + j;
                if ((*pgtable_entry) & PTE_V) {
                    if (page2pa(pp) == PTE_ADDR(*pgtable_entry)) vpn_buffer[tot++] = (i << 10) | j;
                }   
            }   
        }   
    }   
    return tot;
}
```

## 自映射

### exam

在自映射的条件下，请实现函数完成下列任务：

#### 任务0

64位操作系统采用三级页表进行虚拟内存管理，每个页表大小为4KB，页表项需要字对齐，其 余条件与二级页表管理32位操作系统相同。请问64位中最少用多少位表示虚拟地址。

#### 任务1

输入二级页表的起始虚拟地址va，返回一级页表的起始虚拟地址。

#### 任务2

输入页目录的虚拟地址va和一个整数n，返回页目录第n项所对应的二级页表的起始虚拟地址。

**上面的任务1与2，是让你熟悉自映射的有关知识，所有的地址都只是一个u_long类型的数字，并没有 和操作系统打交道，那么最后一个任务则要求你真正填写页表。**

#### 任务3

给定一个一级页表的指针pgdir和二级页表起始虚拟地址va，va为内核态虚拟地址。把合适的地址填写到pgdir的指定位置，使得pgdir能够完成正确的自映射。（即计算出va对应的物理地址所在一级页表项位置，并在那里填入正确的页号和权限位）

输入输出约定：

在include/pmap.h中声明，同时在mm/pmap.c中编写函数：

```
u_long cal_page(int func, u_long va, int n, Pde *pgdir);
```

**输入：**

func为0，1，2，3分别对应前面的任务0123。

1.  va为前述任务中的虚拟地址，func为0时，传入0。
2.  n仅在第二项任务中有意义，意义同题目叙述。在func为0，1，3时，传入0。
3.  pgdir仅在第三项任务中有意义，意义同题目叙述。在func为0，1，2时，传入0。

**输出：**

任务0要求返回正确答案，任务1，2 返回要求地址，任务3返回0即可。

```
u_long cal_page(int func, u_long va, int n, Pde *pgdir) {
	u_long second_begin, first_pn;
	switch (func) {
		case 0:
			return 39;
			break;
		case 1:
			return va + (va >> 10);
			break;
		case 2:
			second_begin = va & (0 - (1 << 22));
			return second_begin + (n << 12);
			break;
		case 3:
			first_pn = (va >> 22); 
			*(pgdir + first_pn) = PADDR(pgdir) | PTE_V | PTE_R;
			return 0;
			break;
	}
	return 0;
}
```

## 页面未使用

### exam

#### 题目背景

我们实现的MOS操作系统中，所有的物理页的可能状态有三种：使用中物理页、空闲物理页、已经被申请但未被使用的物理页。

Note：page_alloc的时候只是申请了一个物理页，但是物理页没有使用，请仔细思考这三种状态物理页的判定方法。

说明：

1.  使用中的物理页：当前使用次数不为0的物理页，状态标记为1
2.  已经被申请但未使用的物理页：当前使用次数为0，但是已经被申请出去的物理页，状态标记为2
3.  空闲物理页：当前可以被申请的物理页，状态标记为3

#### 任务1

在pmap.c中实现函数`int page_alloc2(struct Page **pp)`，并在pmap.h中添加该函数的声明。其功能与原有的page_alloc完全一样（你可以直接复制page_alloc的代码）,唯一的区别在于，如果确实分配到了物理页面，该函数要输出分配到的物理页的信息

输出格式： `printf("page number is %x, start from pa %x\n",ppn,pa);`

其中ppn为页号，pa为该页面的起始物理地址

#### 任务2

在pmap.c中实现函数 `void get_page_status(int pa)` 并在pmap.h中添加该函数的声明。函数输入的是一个物理地址，请按格式输出该物理页的状态信息。

输出格式： `printf("times:%d, page status:%d\n",var1,var2);`

其中var1是统计该函数被调用的次数（首次从1开始），var2是返回该物理地址对应的页面状态标记数字。 评测要求：请确保page_init初始化后page_free_list从表头到表尾物理页下标依次递减

#### 任务3

本次课上测试会对课下测试进行加强测试，请大家在pmap.h中添加以下函数定义（请不要在pmap.c中添加这两个函数的实现，否则远端测评无法编译）：

1.  `void test_queue();`
2.  `void pm_check();`

```
int page_alloc2(struct Page **pp)
{
    struct Page *ppage_temp;

    /* Step 1: Get a page from free memory. If fails, return the error code.*/
	if ((ppage_temp = LIST_FIRST(&page_free_list)) == NULL) {
		return -E_NO_MEM;
	} 


    /* Step 2: Initialize this page.
     * Hint: use `bzero`. */
	LIST_REMOVE(ppage_temp, pp_link);
	u_long temp = page2kva(ppage_temp);

	u_long ppn = page2ppn(ppage_temp);
	u_long pa = page2pa(ppage_temp);

	bzero((void *)temp, BY2PG);
	*pp = ppage_temp;
	printf("page number is %x, start from pa %x\n", ppn, pa);

	return 0;

}

int lab2_times = 0;

void get_page_status(int pa) {
	lab2_times++;
	struct Page *p = pa2page(pa);
	struct Page *tmp;
	int in_list = 0;
	LIST_FOREACH(tmp, &page_free_list, pp_link) {
		if (tmp == p) {
			in_list = 1;
		}
	}
	int status;
	if (p->pp_ref != 0) {
		status = 1;
	} else if (in_list == 0) {
		status = 2;
	} else {
		status = 3;
	}
	printf("times:%d, page status:%d\n",lab2_times,status);

}
```





```
u_int page_perm_stat(Pde *pgdir, struct Page *pp, u_int perm_mask) {
	//Pte *entry = (Pte *)KADDR(PTE_ADDR(*pgdir));
	//*ppte = (Pte *)KADDR(PTE_ADDR(*pgdir_entryp)) + PTX(va);
	int cnt = 0;
	for (int i = 0; i < 1024; ++ i) {
	if (((*(pgdir + i)) & PTE_V) == 0)
		continue;
	Pte *entry = (Pte *)KADDR(PTE_ADDR(*(pgdir+i)));
	for (int i = 0; i < 1024; ++i) {
		Pte *cur = entry + i;
	//for (; cur - pgdir < 1024; cur++) {
		if (pa2page(*cur) == pp) {
			if ((*cur) & PTE_V) {
				if(((*cur) & perm_mask) == perm_mask){
					++cnt;
				}
			}
		}
	}
	}
	return cnt;
}
```

*/

