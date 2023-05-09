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
    char name[DCCTHREAD_MAX_NAME_SIZE];
    ucontext_t *context;
    bool isWaiting;
    dccthread_t *waintingFor;
} dccthread_t;

// Semaphore structure
typedef struct bin_sem{
    int flag;
    int guard;
    struct dlist *sleepingThreadList;
} bin_sem_t;

// Global variables
sigset_t mask;
sigset_t sleepmask;
ucontext_t manager;
struct dlist *readyThreadList;
bin_sem_t *binSem;

//Function to initialize the thread library and creat a new thread to excecute the function func with the parameter param
void dccthread_init(void (*func)(int), int param){

    // Create the ready list
    readyThreadList = dlist_create();

    // Initialize the sleeping semaphore
    binSem =  malloc(sizeof(bin_sem_t));
    binSem->sleepingThreadList = dlist_create();
    binSem->flag = 0;
    binSem->guard = 0;

    // Create the main thread
    dccthread_create("main", func, param);
    getcontext(&manager);

    // Set timer and signal
    timer_t timer_id;
    struct sigevent se;
    struct sigaction sa;
    struct itimerspec ts;

    sa.sa_flags = 0;
    se.sigev_signo = SIGRTMIN;
    se.sigev_notify = SIGEV_SIGNAL;
    se.sigev_notify_attributes = NULL;
    se.sigev_value.sival_ptr = &timer_id;
    
    sa.sa_sigaction = (void *)dccthread_yield;

    ts.it_interval.tv_sec = TIMER_INTERVAL_SEC;
    ts.it_interval.tv_nsec = TIMER_INTERVAL_NSEC;
    ts.it_value.tv_sec = TIMER_INTERVAL_SEC;
    ts.it_value.tv_nsec = TIMER_INTERVAL_NSEC;

    sigaction(SIGRTMIN, &sa, NULL);
    sigemptyset(&mask);
    sigaddset(&mask, SIGRTMIN);
    sigemptyset(&sleepmask);
    sigaddset(&sleepmask, SIGRTMAX);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    timer_create(CLOCK_PROCESS_CPUTIME_ID, &se, &timer_id);
    timer_settime(timer_id, 0, &ts, NULL);

    
    // Run the threads
    while(!dlist_empty(readyThreadList) || !dlist_empty(binSem->sleepingThreadList)){

        // Unblock the sleeping signal
        sigprocmask(SIG_UNBLOCK, &sleepmask, NULL);

        // Block the sleeping signal
        sigprocmask(SIG_BLOCK, &sleepmask, NULL);

        // Get the next thread to be executed
        dccthread_t *nextThread = (dccthread_t*) dlist_get_index(readyThreadList, 0);

        // If the next thread is waiting, put it at the end of the list
        if(nextThread->isWaiting){
            dlist_pop_left(readyThreadList);
            dlist_push_right(readyThreadList, nextThread);
            continue;
        }

        // Swap the context to the next thread's context
        swapcontext(&manager, nextThread->context);

        // Remove the thread from the ready list
        dlist_pop_left(readyThreadList);

        // If the thread is waiting, put it at the end of the list
        if(nextThread->isWaiting){
            dlist_push_right(readyThreadList, nextThread);
        }
    }

    // Destroy the timer
    timer_delete(timer_id);

    // Destroy the ready list
    dlist_destroy(readyThreadList, NULL);

    // Destroy the sleeping list
    dlist_destroy(binSem->sleepingThreadList, NULL);

    exit(0); 
}

//Create a new thread to execute the function func with the parameter param
dccthread_t * dccthread_create(const char *name, void (*func)(int), int param){

    // Alloc memory for the thread and its context
    dccthread_t *newThread = malloc(sizeof(dccthread_t));
    ucontext_t *newContext = malloc(sizeof(ucontext_t));

    // Initialize the new thread's context
    getcontext(newContext);

    // Block the signal
    sigprocmask(SIG_BLOCK, &mask, NULL);

    // Inicialize the thread's attributes
    strcpy(newThread->name, name);
    newThread->isWaiting = false;
    newThread->waintingFor = NULL;

    // Inicialize the context's attributes
    newContext->uc_link = &manager;
    newContext->uc_stack.ss_flags = 0;
    newContext->uc_stack.ss_size = THREAD_STACK_SIZE;
    newContext->uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);

    // Inicialize the threads' context
    newThread->context = newContext;
    
    // Add the new thread to the ready list
    dlist_push_right(readyThreadList, newThread);

    // Creating the context for the new thread
    makecontext(newThread->context, (void(*)(void))func, 1, param);

    // Unblock the signal
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    return newThread;
}

void dccthread_yield(void){

    // Block the signal
    sigprocmask(SIG_BLOCK, &mask, NULL);

    // Return to the end of the list
    dccthread_t *currentThread = dccthread_self();
    dlist_push_right(readyThreadList, currentThread);

    // Call the maneger thread to choose the next thread
    swapcontext(currentThread->context, &manager);

    // Unblock the signal
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

    // Block the signal
    sigprocmask(SIG_BLOCK, &mask, NULL);

    dccthread_t *currentThread = dccthread_self();

    // Find the threads that are waiting for the current thread
    for(int i = 0; i < readyThreadList->count; i++){
        dccthread_t *thread = dlist_get_index(readyThreadList, i);
        if((thread->isWaiting && thread->waintingFor == currentThread)){
            thread->waintingFor = NULL;
            thread->isWaiting = false;
        }
    }

    // Find the sleeping threads that are waiting for the current thread
    for(int i = 0; i < binSem->sleepingThreadList->count; i++){
        dccthread_t *thread = dlist_get_index(binSem->sleepingThreadList, i);
        if((thread->isWaiting && thread->waintingFor == currentThread)){
            thread->waintingFor = NULL;
            thread->isWaiting = false;
        }
    }
        
    // Free the memory allocated for the thread
    free(currentThread);
    
    // Set context back to the manager thread
    setcontext(&manager);

    // Unblock the signal
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

}

// Blocks the current thread untill tid is done executing
void dccthread_wait(dccthread_t *tid){

    // Block the signal
    sigprocmask(SIG_BLOCK, &mask, NULL);

    bool found = false;

    dccthread_t *currentThread = dccthread_self();

    // Look for the object Tid in the ready list
    for(int i = 0; i < readyThreadList->count && !found; i++)
    {
        dccthread_t *thread = dlist_get_index(readyThreadList, i);
        if(thread == tid)
        {
            found = true;
            break;
        }
    }

    // Look for the object Tid in the sleeping list
    for(int i = 0; i < binSem->sleepingThreadList->count && !found; i++)
    {
        dccthread_t *thread = dlist_get_index(binSem->sleepingThreadList, i);
        if(thread == tid)
        {
            found = true;
            break;
        }
    }

    // If we found the object Tid in the ready list, then the current thread is going to stop executing and wait fot tid to end
    if(found)
    {
        currentThread->isWaiting = true; 
        currentThread->waintingFor = tid;

        // Call the maneger thread to choose the next thread
        swapcontext(currentThread->context, &manager);
    }

    // Unblock the signal
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int testAndSet(int *lock){
    int oldValue = *lock;
    *lock = 1;
    return oldValue;
}

void binWait(dccthread_t *thread){
    //while(testAndSet(&binSem->guard) == 1);
    //if(binSem->flag == 0){
    //    binSem->flag = 1;
    //    binSem->guard = 0; // Libera o loop
    //}
    //else{
    //    dlist_push_right(binSem->sleepingThreadList, thread);
    //    binSem->guard = 0; // Libera o loop
    //}
    binSem->flag--;
    if(binSem->flag < 0){
        dlist_push_right(binSem->sleepingThreadList, thread);
    }
}

void binSignal(int sig, siginfo_t *si, void *uc){
    //while(testAndSet(&binSem->guard) == 1);
    //dccthread_t *thread;
    //if(dlist_empty(binSem->sleepingThreadList)){
    //    binSem->flag = 0; // Ninguem está esperando
    //}
    //else{
    //    thread = (dccthread_t *)si->si_value.sival_ptr;
    //    dlist_pop_left(binSem->sleepingThreadList);
    //    dlist_push_right(readyThreadList, thread);
    //}
    //binSem->guard = 0; // Libera o loop
    dccthread_t* sleepingThread = (dccthread_t *)si->si_value.sival_ptr;
    binSem->flag++;
    if(binSem->flag <= 0){
        dlist_pop_left(binSem->sleepingThreadList);
        dlist_push_right(readyThreadList, sleepingThread);
    }
    //dlist_pop_left(binSem->sleepingThreadList);
    //dlist_push_right(readyThreadList, sleeping1);
}

void dccthread_sleep(struct timespec ts){

    // Block the signal
    sigprocmask(SIG_BLOCK, &mask, NULL);

    dccthread_t *currentThread = dccthread_self();

    // Acquire the semaphore and put the thread to sleep
    binWait(currentThread);

    // Set the timer
    timer_t timerp;
    struct sigaction s1;
    struct sigevent sev;
    struct itimerspec its;

    s1.sa_flags = SA_SIGINFO;
    s1.sa_sigaction = binSignal;
    s1.sa_mask = mask;
    sigaction(SIGRTMAX, &s1, NULL);

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMAX;
    sev.sigev_value.sival_ptr = currentThread;
    timer_create(CLOCK_REALTIME, &sev, &timerp);

    its.it_value = ts;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    timer_settime(timerp, 0, &its, NULL);

    // Call the maneger thread to choose the next thread
    swapcontext(currentThread->context, &manager);

    // Unblock the signal
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}