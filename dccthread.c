#include "dccthread.h"
#include "dlist.h"

#include <ucontext.h>
#include <stdbool.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#define TIMER_INTERVAL_SEC 0
#define TIMER_INTERVAL_NSEC 10000000

//Thread structure
typedef struct dccthread{
    char name[DCCTHREAD_MAX_NAME_SIZE]; //Threads's name
    ucontext_t *context;
    bool isSleeping;
    bool isWaiting;
    dccthread_t *waintingFor;

} dccthread_t;

typedef struct bin_sem{
    int flag;
    int guard;
    struct dlist *sleepingThreadList;
} bin_sem_t;

sigset_t mask;
ucontext_t manager;
struct dlist *readyThreadList;
bin_sem_t *binSem;

//Function to initialize the thread library and creat a new thread to excecute the function func with the parameter param
void dccthread_init(void (*func)(int), int param){
readyThreadList = dlist_create();
binSem =  malloc(sizeof(bin_sem_t));
binSem->sleepingThreadList = dlist_create();
binSem->flag = 0;
binSem->guard = 0;
    // 1 -- Initialize a meneger thread that will scalonate the new threads 
    // 2 -- Inicialize a main thread that will execute func THE NAME HAS TO BE MAIN
    dccthread_create("main", func, param);
    getcontext(&manager);

    // set timer
    timer_t timer_id;
    struct sigevent se;
    struct sigaction sa;
    struct itimerspec ts;

    se.sigev_signo = SIGRTMIN;
    se.sigev_notify = SIGEV_SIGNAL;
    se.sigev_notify_attributes = NULL;
    se.sigev_value.sival_ptr = &timer_id;
    sa.sa_flags = 0;
    sa.sa_handler = (void *)dccthread_yield;

    ts.it_interval.tv_sec = TIMER_INTERVAL_SEC;
    ts.it_interval.tv_nsec = TIMER_INTERVAL_NSEC;
    ts.it_value.tv_sec = TIMER_INTERVAL_SEC;
    ts.it_value.tv_nsec = TIMER_INTERVAL_NSEC;

    sigaction(SIGRTMIN, &sa, NULL);
    timer_create(CLOCK_PROCESS_CPUTIME_ID, &se, &timer_id);
    timer_settime(timer_id, 0, &ts, NULL);

    sigemptyset(&mask);
    sigaddset(&mask, SIGRTMIN);

    sigprocmask(SIG_BLOCK, &mask, NULL);
    // 3 -- Execute the main thread
    while(!dlist_empty(readyThreadList))
    {
        dccthread_t *nextThread = (dccthread_t*) dlist_get_index(readyThreadList, 0); // get the next in line thread

        if(nextThread->isWaiting)
        {
            dlist_pop_left(readyThreadList);
            dlist_push_right(readyThreadList, nextThread);
        }

        swapcontext(&manager, nextThread->context);
        dlist_pop_left(readyThreadList);

        if(nextThread->isWaiting){
            dlist_push_right(readyThreadList, nextThread);
        }
    }
    exit(0); 
}

//Create a new thread with name that will execute func with param
dccthread_t * dccthread_create(const char *name, void (*func)(int), int param){

    // 1 -- Inicialize a new thread 

        // Inicialize the thread
        dccthread_t *newThread = malloc(sizeof(dccthread_t));
        strcpy(newThread->name, name);
        newThread->isWaiting = false;
        newThread->waintingFor = NULL;
        newThread->isSleeping = false;

        // Inicialize the context
        ucontext_t *newContext = malloc(sizeof(ucontext_t));
        getcontext(newContext);// Initializes the new thread's context as the currently active context

        sigprocmask(SIG_BLOCK, &mask, NULL);

        newContext->uc_link = &manager;
        newContext->uc_stack.ss_flags = 0;
        newContext->uc_stack.ss_size = THREAD_STACK_SIZE;
        newContext->uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);

        newThread->context = newContext;
    
    // 2 -- Add it to the list of threads ready to execute
    dlist_push_right(readyThreadList, newThread);

    // 3 -- Return to the thread that originally called the function
    makecontext(newThread->context, (void(*)(void))func, 1, param); // criação do contexto da thread

    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    return newThread;
}

void dccthread_yield(void){

    sigprocmask(SIG_BLOCK, &mask, NULL);

    // 1 -- Remove the current thread from the CPU
    dccthread_t *currentThread = dccthread_self();
    dlist_push_right(readyThreadList, currentThread); // Return to the end of the list
    // 2 -- Call the maneger thread to choose the next thread
    swapcontext(currentThread->context, &manager);
    // * a tread must be executed in the order they were created

    sigprocmask(SIG_UNBLOCK, &mask, NULL);

}

// Return a pointer to the thread that is currently being executed
dccthread_t *dccthread_self(void){
    dccthread_t *currentThread = dlist_get_index(readyThreadList, 0);
    return currentThread;
}

// Return the name of the tid thread
const char *dccthread_name(dccthread_t *tid){
    return tid->name;
}

// Ends the current thread
void dccthread_exit(void){

    sigprocmask(SIG_BLOCK, &mask, NULL);

    dccthread_t *currentThread = dccthread_self();
    for(int i = 0; i < readyThreadList->count; i++)
    {
        dccthread_t *thread = dlist_get_index(readyThreadList, i);
        if((thread->isWaiting && thread->waintingFor == currentThread)){
            thread->waintingFor = NULL;
            thread->isWaiting = false;
        }
    }
    
    free(currentThread);

    sigprocmask(SIG_UNBLOCK, &mask, NULL);

} // when the lat thread calls for exit the program must end

// Blocks the current thread untill tid is done executing
void dccthread_wait(dccthread_t *tid){

    sigprocmask(SIG_BLOCK, &mask, NULL);

    bool found = false;

    // if we find the object Tid in the ready list its not done executing yet
    for(int i = 0; i < readyThreadList->count && !found; i++)
    {
        dccthread_t *thread = dlist_get_index(readyThreadList, i);
        if(thread == tid)
        {
            found = true;
            break;
        }
    }

    // if we found the object Tid in the ready list, then the current thread is going to stop executing and wait fot tid to end
    if(found)
    {
        dccthread_t *currentThread = dccthread_self();
        currentThread->isWaiting = true; 
        currentThread->waintingFor = tid;
        swapcontext(currentThread->context, &manager);
    }

    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int cmp(const void *e1, const void *e2, void *userdata){
	return (dccthread_t *)e1 != (dccthread_t *)e2;
}

int testAndSet(int *lock){
    int oldValue = *lock;
    *lock = 1;
    return oldValue;
}

void binWait(dccthread_t *thread){
    while(testAndSet(&binSem->guard) == 1);
    if(binSem->flag == 0){
        binSem->flag = 1;
        binSem->guard = 0;
    }
    else{
        binSem->guard = 1;
        thread->isSleeping = true;
        dlist_push_right(binSem->sleepingThreadList, thread);
        binSem->guard = 0;
    }
}

void binSignal(int sig, siginfo_t *si, void *uc){
    while(testAndSet(&binSem->guard) == 1);
    dccthread_t *thread;
    if(dlist_empty(binSem->sleepingThreadList)){
        binSem->flag = 0;
    }
    else{
        thread = dlist_get_index(binSem->sleepingThreadList, 0);
        dlist_push_right(readyThreadList, thread);
    }
    binSem->guard = 0;
}

void dccthread_sleep(struct timespec ts){
    sigprocmask(SIG_BLOCK, &mask, NULL);

    dccthread_t *currentThread = dccthread_self();
    binWait(currentThread);

    timer_t sleepTimer;
    struct sigevent sleepEvent;
    struct sigaction sleepAction;
    struct itimerspec sleepSpec;

    sleepAction.sa_flags = SA_SIGINFO;
    sleepAction.sa_sigaction = binSignal;
    sleepAction.sa_mask = mask;
    sigaction(SIGRTMAX, &sleepAction, NULL);

    sleepEvent.sigev_notify = SIGEV_SIGNAL;
    sleepEvent.sigev_signo = SIGRTMAX;
    sleepEvent.sigev_value.sival_ptr = currentThread;
    timer_create(CLOCK_REALTIME, &sleepEvent, &sleepTimer);

    sleepSpec.it_value = ts;
    sleepSpec.it_interval.tv_sec = ts.tv_sec;
    sleepSpec.it_interval.tv_nsec = ts.tv_nsec;
    timer_settime(sleepTimer, 0, &sleepSpec, NULL);

    swapcontext(currentThread->context, &manager);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}