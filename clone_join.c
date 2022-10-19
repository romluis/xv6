// Create a new thread
int
clone(void (*fcn)(void*), void *arg, void *stack) 
{
  if ((uint)stack % PGSIZE != 0 || (uint)stack + PGSIZE > *(proc->sz)) {
    return -1;
  }
  
  
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  np->pgdir = proc->pgdir;
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;
  
  np->ustack = stack;
  *((uint*)(stack + PGSIZE - sizeof(uint))) = (uint)arg;
  *((uint*)(stack + PGSIZE - 2 * sizeof(uint))) = 0xffffffff;
  np->tf->esp = (uint)stack + PGSIZE - 2 * sizeof(uint);
  np->tf->eip = (uint)fcn;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);
 
  pid = np->pid;
  np->state = RUNNABLE;
  safestrcpy(np->name, proc->name, sizeof(proc->name));
  
  np->reference_count = proc->reference_count;
  *(np->reference_count) = *(np->reference_count) + 1;
  return pid;
}

// wait for one of the child thread to exit
int
join(void** stack)
{
  if ((uint) stack + sizeof(uint) > *(proc->sz)) {
    return -1;
  }
  
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc || p->pgdir != proc->pgdir)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        *stack = p->ustack;
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        p->state = UNUSED;
        p->highPriorityTime = 0;
        p->lowPriorityTime = 0;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        *(p->reference_count) = *(p->reference_count) - 1;
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}
