#include <mmu.h>
#include <pmap.h>
#include <print.h>

void test_inter() {
	struct Page *p;
	assert(page_alloc(&p) == 0);
	Pde *pgdir = (Pde *)page2kva(p);
	u_int va[4] = {UTEXT, UTEXT + BY2PG, UTEXT + 1024 * BY2PG, UTEXT + 1025 * BY2PG};
	u_int perm[4] = {PTE_V, PTE_V | PTE_D | PTE_G, PTE_V | PTE_G, PTE_V | PTE_D};
	struct Page *pp;

	assert(page_alloc(&pp) == 0);
	assert(page_insert(pgdir, 0, pp, va[0], perm[0]) == 0);
	assert(page_insert(pgdir, 0, pp, va[1], perm[1]) == 0);
	assert(page_insert(pgdir, 0, pp, va[2], perm[2]) == 0);
	assert(page_insert(pgdir, 0, pp, va[3], perm[3]) == 0);
	int r = page_inter_stat(pgdir, pp, PTE_D | PTE_G);
	assert(r == 3);
	printk("test succeeded!\n");
}

void mips_init() {
	mips_detect_memory();
	mips_vm_init();
	page_init();
	test_inter();
	halt();
}
