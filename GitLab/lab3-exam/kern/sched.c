#include <env.h>
#include <pmap.h>
#include <printk.h>

/* Overview:
 *   Implement a round-robin scheduling to select a runnable env and schedule it using 'env_run'.
 *
 * Post-Condition:
 *   If 'yield' is set (non-zero), 'curenv' should not be scheduled again unless it is the only
 *   runnable env.
 *
 * Hints:
 *   1. The variable 'count' used for counting slices should be defined as 'static'.
 *   2. Use variable 'env_sched_list', which contains and only contains all runnable envs.
 *   3. You shouldn't use any 'return' statement because this function is 'noreturn'.
 */
void schedule(int yield) {
    static int count = 0; // remaining time slices of current env
    struct Env *e = curenv;
    static int user_time[5];
    int st[5] = {0};
    int st2[5] = {0};
    /* We always decrease the 'count' by 1.
     *
     * If 'yield' is set, or 'count' has been decreased to 0, or 'e' (previous 'curenv') is
     * 'NULL', or 'e' is not runnable, then we pick up a new env from 'env_sched_list' (list of
     * all runnable envs), set 'count' to its priority, and schedule it with 'env_run'. **Panic
     * if that list is empty**.
     *
     * (Note that if 'e' is still a runnable env, we should move it to the tail of
     * 'env_sched_list' before picking up another env from its head, or we will schedule the
     * head env repeatedly.)
     *
     * Otherwise, we simply schedule 'e' again.
     *
     * You may want to use macros below:
     *   'TAILQ_FIRST', 'TAILQ_REMOVE', 'TAILQ_INSERT_TAIL'
     */
    /* Exercise 3.12: Your code here. */
    if (yield || count <= 0 || e == NULL || e->env_status != ENV_RUNNABLE) {
        if(e != NULL) {
            TAILQ_REMOVE(&env_sched_list, e, env_sched_link);
            if(e->env_status == ENV_RUNNABLE) {
                user_time[e->env_user] += e->env_pri;
                TAILQ_INSERT_TAIL(&env_sched_list, e, env_sched_link);
            }
        }

        if (TAILQ_EMPTY(&env_sched_list)) {
            panic("no runnable envs");
        }
        struct Env *i;
      //  int user_num = 0;
        //TAILQ_FOREACH(i, &env_sched_list, env_sched_link) {
          //  if(i->env_status == ENV_RUNNABLE){
            //    st[i->env_user] = 1;
           // }
           // st2[i->env_user] = 1;
       // }
     //   int selected_user = -1;
        int j = -1;
        int min = 9999999;
     //   for( ; j<5;j++){
      //      if(st2[j])
       //         user_num++;
       // }
      //  j=0;
       // for( ; j<user_num; j++){
        //    if(user_time[j] <= min){
         //       min = user_time[j];
          //      selected_user = j;
           // }
       // }
        TAILQ_FOREACH(i, &env_sched_list, env_sched_link) {
            if (i->env_status == ENV_RUNNABLE && user_time[i->env_user] <= min) {
                if(user_time[i->env_user] == min && i->env_user > j)
                {
                    j = i->env_user;
                    e = i;
                }
                else if(user_time[i->env_user] < min){
                    j = i->env_user;
                    e = i;
                }
                min = user_time[i->env_user];
            }
        }

        count = e->env_pri;
    }
    count--;
    env_run(e);
}
