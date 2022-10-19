#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

#define PGSIZE (4096)

//Creates a kernel thread, shares the address space with the 
//calling process.
//New Thread uses stack as its user stack. It is passed in the
//argument arg and returns a fake PC.
//It is one page in size (PSSIZE)
//New thread starts executing at the adress specified by fn.
//
int 
thread_create(void (*start_routine)(void*), void *arg) 
{
  void *stack = malloc(PGSIZE*2);
  if (!stack) {
    printf(1, "Error: malloc failed\n");
    exit();   
  }
    
  if((uint)stack % PGSIZE) {
   stack = stack + (4096 - (uint)stack % PGSIZE);
  }

  return clone(start_routine, arg, stack);
}

//Causes the caller to block until a child thread terminates
//upon which the child's id will be returned.
//Similar to wait()
int
thread_join()
{
  void* ustack;
  int thread_id = join(&ustack);
  if (thread_id != -1) {
    free(ustack);
  }
  
  return thread_id;
}

int 
thread_exit()
{
  return 0;
}

//Initializes mutex lock
void lock_init(volatile lock_t *lock) {
  *lock = 0;
}

//Acquire the lock. If not, use sleep() to yield to scheduler
void lock_acquire(volatile lock_t *lock) {
  while (xchg(lock, 1) == 1)
    ; // spin
}

//Release the lock
void lock_release(volatile lock_t *lock) {
  *lock = 0;
}
