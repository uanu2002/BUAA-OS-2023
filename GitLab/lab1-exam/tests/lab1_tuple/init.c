void tuple_1_check(void) {
	printk("%s%T%s", "This is a testcase: ", 2023, 2023, 2023, "\n");
	printk("the tuple is %T\n", 1, 9, 9 - 1);
}

void mips_init() {
	tuple_1_check();
	halt();
}
