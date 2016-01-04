#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <semaphore.h>

/*Individual job*/
typedef struct thpool_job_t {
    void   (*function)(void* arg);    //函数指针
    void                    *arg ;    //函数的参数
    struct tpool_job_t     *next ;    //指向下一个任务
    struct tpool_job_t     *prev ;    //指向前一个任务
}thpool_job_t ;

/*job queue as doubly linked list*/
typedef struct thpool_jobqueue {
    thpool_job_t    *head ;           //队列的头指针
    thpool_job_t    *tail;            //对列的尾指针
    int             jobsN;           //队列中工作的个数
    sem_t           *queueSem;        //原子信号量
}thpool_jobqueue;

/*thread pool*/

typedef struct thpool_t {
    pthread_t          *threads ;        //线程的ID
    int                 threadsN ;        //线程的数量
    thpool_jobqueue    *jobqueue;        //工作队列的指针
}thpool_t;


/*线程池中的线程都需要互斥锁和指向线程池的一个指针*/
typedef struct thread_data{
    pthread_mutex_t     *mutex_p ;
    thpool_t            *tp_p ;
}thread_data;



/*
 *    初始化线程池
 *    为线程池， 工作队列， 申请内存空间，信号等申请内存空间
 *    @param  :将被使用的线程ID
 *    @return :成功返回的线程池结构体，错误返回null
 */

thpool_t   *thpool_init (int threadsN);

/*
 * 每个线程要做的事情
 * 这是一个无止境循环，当撤销这线程池的时候，这个循环才会被中断
 *@param: 线程池
 *@return:不做任何的事情
 */

void  thpool_thread_do (thpool_t *tp_p);

/*
 *向工作队列里面添加任何
 *采用来了一个行为和他的参数，添加到线程池的工作对列中去，
 * 如果你想添加工作函数，需要更多的参数，通过传递一个指向结构体的指针，就可以实现一个接口
 * ATTENTION：为了不引起警告，你不得不将函数和参数都带上
 *
 * @param: 添加工作的线程线程池
 * @param: 这个工作的处理函数
 * @param:函数的参数
 * @return : int
 */

int thpool_t_add_work (thpool_t *tp_p ,void* (*function_p) (void *), void* arg_p );


/*
 *摧毁线程池
 *
 *这将撤销这个线程池和释放所申请的内存空间，当你在调用这个函数的时候，存在有的线程还在运行中，那么
 *停止他们现在所做的工作，然后他们被撤销掉
 * @param:你想要撤销的线程池的指针
 */


void thpool_destory (thpool_t  *tp_p);

/*-----------------------Queue specific---------------------------------*/



/*
 * 初始化队列
 * @param: 指向线程池的指针
 * @return :成功的时候返回是 0 ，分配内存失败的时候，返回是-1
 */
int thpool_jobqueue_init (thpool_t *tp_p);


/*
 *添加任务到队列
 *一个新的工作任务将被添加到队列，在使用这个函数或者其他向别的类似这样
 *函数 thpool_jobqueue_empty ()之前，这个新的任务要被申请内存空间
 *
 * @param: 指向线程池的指针
 * @param:指向一个已经申请内存空间的任务
 * @return   nothing
 */
void thpool_jobqueue_add (thpool_t * tp_p , thpool_job_t *newjob_p);

/*
 * 移除对列的最后一个任务
 *这个函数将不会被释放申请的内存空间，所以要保证
 *
 *@param :指向线程池的指针
 *@return : 成功返回0 ，如果对列是空的，就返回-1
 */
int thpool_jobqueue_removelast (thpool_t *tp_p);


/*
 *对列的最后一个任务
 *在队列里面得到最后一个任务，即使队列是空的，这个函数依旧可以使用
 *
 *参数：指向线程池结构体的指针
 *返回值：得到队列中最后一个任务的指针，或者在对列是空的情况下，返回是空
 */
thpool_job_t * thpool_jobqueue_peek (thpool_t *tp_p);

/*
 *移除和撤销这个队列中的所有任务
 *这个函数将删除这个队列中的所有任务，将任务对列恢复到初始化状态，因此队列的头和对列的尾都设置为NULL ，此时队列中任务= 0
 *
 *参数：指向线程池结构体的指针
 *
 */
void thpool_jobqueue_empty (thpool_t *tp_p);

#endif
