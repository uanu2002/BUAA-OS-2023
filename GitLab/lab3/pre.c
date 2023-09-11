/*
选择的规则是：首先按照进程时间片的大小进行排序，时间片小的排在前面；
如果时间片相同，则按照进程的用户 ID 进行排序，用户 ID 小的排在前面。
选择完毕后，将其赋值给 e。添加的内容
*/

void schedule(int yield) {
	static int count = 0; // remaining time slices of current env
	struct Env *e = curenv;

	static int tim[8] = {0};

	if (yield || count == 0 || e == NULL || e->env_status == ENV_NOT_RUNNABLE) {
		if (e != NULL && e->env_status == ENV_RUNNABLE) {
			TAILQ_REMOVE(&env_sched_list, e, env_sched_link);
			TAILQ_INSERT_TAIL(&env_sched_list, e, env_sched_link);

			tim[e->env_user] += e->env_pri;

		}
		panic_on(TAILQ_EMPTY(&env_sched_list));

		struct Env *i;
		int tempcnt = 0xffffff;
		int tempusr = 6;
		TAILQ_FOREACH(i, &env_sched_list, env_sched_link) {
			if (tempcnt >= tim[i->env_user]) {
				if (tempcnt == tim[i->env_user] && tempusr > i->env_user) {
					tempcnt = tim[i->env_user];
					tempusr = i->env_user;
				} else if (tempcnt > tim[i->env_user]) {
					tempcnt = tim[i->env_user];
					tempusr = i->env_user;
				}
			}
		}
		TAILQ_FOREACH(i, &env_sched_list, env_sched_link) {
			if (i->env_user == tempusr) {
				e = i;
				break;
			}
		}

		count = e->env_pri;
	}
	--count;
	env_run(e);
}


/*
在全局有三个调度队列，记为 env_sched_list[3] ，每个队列中的进程单次运行的时间片数量为进程优先级乘
以不同的权重，具体的：
env_sched_list[0] 中进程单次运行时间片数 = 进程优先级数 * 1
env_sched_list[1] 中进程单次运行时间片数 = 进程优先级数 * 2
env_sched_list[2] 中进程单次运行时间片数 = 进程优先级数 * 4
进程创建时全部插入到第一个调度队列（即 env_sched_list[0] ）的队首
进程时间片用完后，根据自身优先级数值加入到另外两个调度队列队尾，若自身优先级为奇数，则顺次加入到下
一个调度队列队尾，若为偶数，则加入到下下个调度队列队尾。当然，进程不再处于原调度队列中。具体地：
env_sched_list[0] 中的进程时间片耗完后，若优先级为奇数，加入到 env_sched_list[1] 队尾；若优
先级为偶数，加入到 env_sched_list[2] 队尾
env_sched_list[1] 中的进程时间片耗完后，若优先级为奇数，加入到 env_sched_list[2] 队尾；若优
先级为偶数，加入到 env_sched_list[0] 队尾
env_sched_list[2] 中的进程时间片耗完后，若优先级为奇数，加入到 env_sched_list[0] 队尾；若优
先级为偶数，加入到 env_sched_list[1] 队尾
sched_yield 函数首先从 env_sched_list[0] 队列开始调度，之后依次按照 0 , 1 , 2 , 0 , 1 , ......的顺序切换
队列，且仅在当前队列中没有可运行进程时切换到下一个队列
*/
int init[3] = {1, 2, 4,};

/* Overview:
 *  Implement simple round-robin scheduling.
 *
 *
 * Hints:
 *  1. The variable which is for counting should be defined as 'static'.
 *  2. Use variable 'env_sched_list', which is a pointer array.
 *  3. CANNOT use `return` statement!
 */
/*** exercise 3.15 ***/
void sched_yield(void) {
	static int count = 0; // remaining time slices of current env
	static int point = 0; // current env_sched_list index
	static int next_point = 0; // next env_sched_list index

	static struct Env *e = NULL;
	/*  hint:
	 *  1. if (count==0), insert `e` into `env_sched_list[1-point]`
	 *     using LIST_REMOVE and LIST_INSERT_TAIL.
	 *  2. if (env_sched_list[point] is empty), point = 1 - point;
	 *     then search through `env_sched_list[point]` for a runnable env `e`,
	 *     and set count = e->env_pri
	 *  3. count--
	 *  4. env_run()
	 *
	 *  functions or macros below may be used (not all):
	 *  LIST_INSERT_TAIL, LIST_REMOVE, LIST_FIRST, LIST_EMPTY
	 */
	if (count == 0 || e == NULL || e->env_status != ENV_RUNNABLE) {
		if (e != NULL) {
			LIST_REMOVE(e, env_sched_link);
			if (e->env_status != ENV_FREE) {
				if (e->env_pri & 1) {
					next_point = point + 1;
					next_point %= 3;
				} else {
					next_point = point + 2;
					next_point %= 3;
				}
				LIST_INSERT_TAIL(&env_sched_list[next_point], e, env_sched_link);
			}
		}
		while (1) {
			while (LIST_EMPTY(&env_sched_list[point])) {
				point++;
				point %= 3;
			}
			e = LIST_FIRST(&env_sched_list[point]);
			if (e->env_status == ENV_FREE)
				LIST_REMOVE(e, env_sched_link);
			else if (e->env_status == ENV_NOT_RUNNABLE) {
				LIST_REMOVE(e, env_sched_link);

				if (e->env_pri & 1) {
					point++;
					point %= 3;
				} else {
					point += 2;
					point %= 3;
				}

				LIST_INSERT_TAIL(&env_sched_list[point], e, env_sched_link);

			} else {
				count = e->env_pri * init[point];
				break;
			}
		}
	}
	count--;
	e->env_runs++;
	printf("\n");
	env_run(e);
}





/*
模拟使用软件 ASID 的情形下，进程建立时 ASID 的分配过程
原本的 asid_alloc 当无法分配时，会发出一个 panic ，而现在我们肯定不想让它这样，因此我们改为返回一个错误的值 65
然后根据 asid_alloc 函数，自己写一个 check_asid_free ，检查 ASID 是否空闲
exam_env_run 就是根据流程图改写代码了，注意先后顺序，首先需要检查 ASID 是否有效（版本号是否相同），如果有效就什么都不用做了，无效的话，检查当前版本号下的 ASID 是否耗尽，没耗尽就直接再分配一个就行了，如果耗尽了，那么就需要让版本号加 1，然后把 ASID 位图全部置为 0，然后重新分配 ASID 即可
*/

#define HARDWARE_ASID(env_asid) ((env_asid) & 0x3f)
#define VERSION_ASID(env_asid) ((env_asid) >> 6)

u_int mkenvid(struct Env *e) {
	/*Hint: lower bits of envid hold e's position in the envs array. */
	u_int idx = (u_int)e - (u_int)envs;
	idx /= sizeof(struct Env);
	/*Hint: avoid envid being zero. */
	return (1 << (LOG2NENV)) | idx;  //LOG2NENV=10
}

static u_int asid_alloc() {
	int i, index, inner;
	for (i = 0; i < 64; ++i) {
		index = i >> 5;
		inner = i & 31;
		if ((asid_bitmap[index] & (1 << inner)) == 0) {
			asid_bitmap[index] |= 1 << inner;
			return i;
		}
	}
	return 65;
}

static u_int check_asid_free(int id) {
	u_int index = id >> 5;
	u_int inner = id & 31;
	if ((asid_bitmap[index] & (1 << inner)) == 0)
		return 1;
	return 0;
}

// Lab 3-1 Exam
static u_int system_version = 0x4;

u_int exam_env_run(struct Env *e) {
	if (VERSION_ASID(e->env_asid) == system_version)
		return 0;
	int proc_asid = HARDWARE_ASID(e->env_asid);
	if (check_asid_free(proc_asid) == 1) {
		e->env_asid = (system_version << 6) | proc_asid;
		asid_bitmap[(proc_asid >> 5)] |= 1 << (proc_asid & 31);
		return 0;
	} else {
		proc_asid = asid_alloc();
		if (proc_asid != 65) {
			e->env_asid = (system_version << 6) | proc_asid;
			return 0;
		} else {
			//printf("%d\n", HARDWARE_ASID(e->env_id));
			system_version += 1;
			asid_bitmap[0] = asid_bitmap[1] = 0;
			proc_asid = asid_alloc();
			e->env_asid = (system_version << 6) | proc_asid;
			return 1;
		}
	}
}

void exam_env_free(struct Env *e) {
	if (VERSION_ASID(e->env_asid) == system_version) {
		asid_free(HARDWARE_ASID(e->env_asid));
	}
}





/*
题目要求： 在本次课上测试中，我们对env_pri的含义进行如下约定：

env_pri的第0-7位仍然表示进程的优先级（PRI）
env_pri的第8-15位为功能字段1（FUNC_1）
env_pri的第16-23位为功能字段2（FUNC_2）
env_pri的第24-31位为功能字段3（FUNC_3）
FUNC_1, FUNC_2, FUNC_3均为无符号数
exam
PART1
在这次的课上测试中，我们需要你实现一个新的调度算法，具体要求如下： 1.修改sched_yield函数，使得在进程调度时高优先级（用PRI表示）的进程永远比低优先级（用PRI表示）的进程先调度。 2.为了简化问题，不需要考虑两个进程优先级相同的情况。 例如：如果有两个进程A和B，优先级分别为1和2，且env_status都为ENV_RUNNABLE，则只有B会得到执行；当B的状态不为ENV_RUNNABLE时，A才会得到执行。

PART2
在PART1中的sched_yield上继续修改，要求如下： 在当前进程（记为进程X）被时钟中断暂停执行时，进行如下操作： 1.根据FUNC_1的值更新进程X的优先级： 若FUNC_1的值为0，则不修改进程X的优先级 若FUNC_1的值为正，则将进程X的优先级减少FUNC_1的值（如果减少之后优先级小于0则优先级置零） 2.完成PART1中的进程调度

解答
有一些坑：

(x<<24)>>24可能导致结果为负数，所以最好x&(0x000000ff)
没有时间片的概念，所以不存在一个进程的时间片用完这种说法，所以不用在两个list之间倒腾来倒腾去
本地测试时，永远输出一个数字，不存在进程切换
*/
#define PRI(X) (((X)->env_pri) & 0xff)
#define FUNC_1(X) ((((X)->env_pri) >> 8) & 0xff)
#define FUNC_2(X) ((((X)->env_pri) >> 16) & 0xff)
#define FUNC_3(X) ((((X)->env_pri) >> 24) & 0xff)
void sched_yield(void) {
	static int count = 0; // remaining time slices of current env
	static int point = 0; // current env_sched_list index
	static struct Env *e = NULL;
	struct Env *tempe;
	int maxpri = 0;

	/*  hint:
	 *  1. if (count==0), insert `e` into `env_sched_list[1-point]`
	 *     using LIST_REMOVE and LIST_INSERT_TAIL.
	 *  2. if (env_sched_list[point] is empty), point = 1 - point;
	 *     then search through `env_sched_list[point]` for a runnable env `e`,
	 *     and set count = e->env_pri
	 *  3. count--
	 *  4. env_run()
	 *
	 *  functions or macros below may be used (not all):
	 *  LIST_INSERT_TAIL, LIST_REMOVE, LIST_FIRST, LIST_EMPTY
	 */
	while (!LIST_EMPTY(&env_sched_list[1])) { // abandon list[1]
		tempe = LIST_FIRST(&env_sched_list[1]);
		LIST_REMOVE(tempe, env_sched_link);
		LIST_INSERT_HEAD(&env_sched_list[0], tempe, env_sched_link);

	}
	if (e != NULL) {
		if (FUNC_1(e) > 0) {
			if (PRI(e) >= FUNC_1(e)) {
				e->env_pri -= FUNC_1(e);
			} else {
				e->env_pri = 0;
			}
		}
	}
	maxpri = 0;
	LIST_FOREACH(tempe, &env_sched_list[0], env_sched_link) {
		if (PRI(tempe) > maxpri && tempe->env_status == ENV_RUNNABLE) {
			e = tempe;
			maxpri = PRI(tempe);
		}
	}

	env_run(e);
}






/*
在今天的实验里我们要求你实现一个简易的 fork 函数（并不包括实际load代码段），通过给定的原始进程块(输入参数struct Env *e)生成一个新的进程控制块，并返回新进程控制块的env_id。同学们需要在 lib/env.c 和 include/env.h 中分别定义和声明 fork 函数，函数接口如下： u_int fork(struct Env *e); 要求如下：

从 env_free_list 中从头申请一个新的进程控制块
新进程控制块的 env_status、env_pgdir、env_cr3、env_pri和原进程控制块保持一致。
为新进程控制块生成对应的 env_id
env_parent_id 的值为原进程控制块的 env_id
返回值为新进程的env_id

本部分要求修改struct Env，在进程控制块中增加字段（具体增加哪些内容请自行组织）组织起进程间的父子、兄弟关系，并按照要求在 lib/env.c 和 include/env.h 中分别定义和声明 lab3_output 函数输出相关内容，详情如下： 函数lab3_output的定义如下： void lab3_output(u_int env_id); 要求输出的内容有其父进程的env_id、其第一个子进程的env_id、其前一个兄弟进程的env_id以及其后一个兄弟进程的env_id 所有的子进程都由fork创建，两个进程如果是兄弟，它们的父进程一定相同。 以某进程第一个子进程是指，由该进程作为父进程使用fork创建的第一个子进程。 兄弟进程间的顺序即为这些进程被创建的顺序，前一个兄弟进程为较早被创建的进程 需要在PART1的fork函数中进行对添加字段的修改 输出格式为：printf("%08x %08x %08x %08x\n", a, b, c, d); 其中a, b, c, d分别为父进程的env_id、第一个子进程的env_id、前一个兄弟进程的env_id以及后一个兄弟进程的env_id 如果a, b, c, d中有不存在的参数，则输出0

在PART2的基础上，在 lib/env.c 和 include/env.h 中分别定义和声明 lab3_get_sum 函数，函数的功能为：给定一个进程的env_id，返回以该进程为根节点的子进程树中进程的数目（包括它本身），具体接口如下： int lab3_get_sum(u_int env_id);
*/

struct Env {
	struct Trapframe env_tf;        // Saved registers
	LIST_ENTRY(Env) env_link;       // Free list
	u_int env_id;                   // Unique environment identifier
	u_int env_parent_id;            // env_id of this env's parent
	u_int env_status;               // Status of the environment
	Pde  *env_pgdir;                // Kernel virtual address of page dir
	u_int env_cr3;
	LIST_ENTRY(Env) env_sched_link;
	u_int env_pri;
	// Lab 4 IPC
	u_int env_ipc_value;            // data value sent to us
	u_int env_ipc_from;             // envid of the sender
	u_int env_ipc_recving;          // env is blocked receiving
	u_int env_ipc_dstva;            // va at which to map received page
	u_int env_ipc_perm;             // perm of page mapping received

	// Lab 4 fault handling
	u_int env_pgfault_handler;      // page fault state
	u_int env_xstacktop;            // top of exception stack

	// Lab 6 scheduler counts
	u_int env_runs;                 // number of times been env_run'ed
	u_int env_nop;                  // align to avoid mul instruction

	int son_num;            // add on exam
	u_int son_id_arr[1024]; // add on exam

};

u_int fork(struct Env *e) {
	struct Env *e_son;
	env_alloc(&e_son, e->env_id);
	e_son->env_status = e->env_status;
	e_son->env_pgdir = e->env_pgdir;
	e_son->env_cr3 = e->env_cr3;
	e_son->env_pri = e->env_pri;

	// ---- father ----
	int son_num = e->son_num;
	e->son_id_arr[son_num] = e_son->env_id;
	e->son_num += 1;

	return e_son->env_id;
}


void lab3_output(u_int env_id) {
	struct Env *e_now;
	u_int fa_id = 0;
	u_int first_son_id = 0;
	u_int bro_bf_id = 0; // "bf" means "before"
	u_int bro_af_id = 0; // "af" means "after"

	envid2env(env_id, &e_now, 0);
	// son part
	first_son_id = e_now->son_id_arr[0];

	// parent part
	fa_id = e_now->env_parent_id;
	if (fa_id == 0) { // do not have parent
		// three 0 now
		bro_bf_id = 0;
		bro_af_id = 0;
	} else { // have a parent
		struct Env *e_fa;
		envid2env(fa_id, &e_fa, 0);
		int index = 0;
		for (index = 0; index < 1024; index++) {
			if (e_fa->son_id_arr[index] == env_id) {
				break;
			} else {
				continue;
			}
		}
		// index is the env of father now
		if (index > 0) { // have bro bf
			bro_bf_id = e_fa->son_id_arr[index - 1];
		}

		// have a bro_af
		if (e_fa->son_num > index + 1) {
			bro_af_id = e_fa->son_id_arr[index + 1];
		}
	}
	// fa_id, fist_son_id, bro_bf, bro_af
	printf("%08x %08x %08x %08x\n", fa_id, first_son_id, bro_bf_id, bro_af_id);
}

int lab3_get_sum(u_int env_id) {
	struct Env *e_now;
	envid2env(env_id, &e_now, 0);
	int son_num = e_now->son_num;
	// if e_now has no son
	if (son_num == 0) {
		return 1;
	} else {
		// have many sons, recuring
		int ans = 1;
		int i = 0;
		for (i = 0; i < son_num; i++) {
			struct Env *e_son;
			u_int son_id = e_now->son_id_arr[i];
			envid2env(son_id, &e_son, 0); // now got a son
			ans += lab3_get_sum(son_id);
		}
		return ans;
	}
}




void sched_yield(void)
{
    static int count = 0; // remaining time slices of current env
    static int point = 0; // current env_sched_list index
 static struct Env *e;
 struct Env *maxe;
 int maxpri = 0;
  while (!LIST_EMPTY(&env_sched_list[1])) {
   e = LIST_FIRST(&env_sched_list[1]);
   LIST_REMOVE(e, env_sched_link);
   LIST_INSERT_HEAD(&env_sched_list[0], e, env_sched_link);

  }
  maxpri = 0;
  LIST_FOREACH(e, &env_sched_list[0],env_sched_link) {
   if (e->env_pri > maxpri && e->env_status == ENV_RUNNABLE) {
    maxe = e;
    maxpri = e->env_pri;
   }
  }

 env_run(maxe);
}



void bubble_sort(int arr[], int len) {
        int i, j, temp;
        for (i = 0; i < len - 1; i++)
                for (j = 0; j < len - 1 - i; j++)
                        if (arr[j] > arr[j + 1]) {
                                temp = arr[j];
                                arr[j] = arr[j + 1];
                                arr[j + 1] = temp;
                        }
}


int values[] = { 88, 56, 100, 2, 25 };

int cmpfunc (const void * a, const void * b)
{
   return ( *(int*)a - *(int*)b );
}

int main()
{
   int n;

   printf("排序之前的列表：\n");
   for( n = 0 ; n < 5; n++ ) {
      printf("%d ", values[n]);
   }

   qsort(values, 5, sizeof(int), cmpfunc);

   printf("\n排序之后的列表：\n");
   for( n = 0 ; n < 5; n++ ) {
      printf("%d ", values[n]);
   }
 
  return(0);
}

