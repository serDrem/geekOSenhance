/*
 * System call handlers
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2003,2004 David Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.59 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/syscall.h>
#include <geekos/errno.h>
#include <geekos/kthread.h>
#include <geekos/int.h>
#include <geekos/elf.h>
#include <geekos/malloc.h>
#include <geekos/screen.h>
#include <geekos/keyboard.h>
#include <geekos/string.h>
#include <geekos/user.h>
#include <geekos/timer.h>
#include <geekos/vfs.h>
#include <libc/sema.h>

/*
 * Allocate a buffer for a user string, and
 * copy it into kernel space.
 * Interrupts must be disabled.
 */
static int Copy_User_String(ulong_t uaddr, ulong_t len, ulong_t maxLen, char **pStr)
{
    int rc = 0;
    char *str;

    /* Ensure that string isn't too long. */
    if (len > maxLen)
        return EINVALID;

    /* Allocate space for the string. */
    str = (char*) Malloc(len+1);
    if (str == 0) {
        rc = ENOMEM;
        goto done;
    }   

    /* Copy data from user space. */
    if (!Copy_From_User(str, uaddr, len)) {
        rc = EINVALID;
        Free(str);
        goto done;
    }   
    str[len] = '\0';

    /* Success! */
    *pStr = str;

done:
    return rc; 
}


/*
 * Null system call.
 * Does nothing except immediately return control back
 * to the interrupted user program.
 * Params:
 *  state - processor registers from user mode
 *
 * Returns:
 *   always returns the value 0 (zero)
 */
static int Sys_Null(struct Interrupt_State* state)
{
    return 0;
}

/*
 * Exit system call.
 * The interrupted user process is terminated.
 * Params:
 *   state->ebx - process exit code
 * Returns:
 *   Never returns to user mode!
 */
static int Sys_Exit(struct Interrupt_State* state)
{
	Exit(state->ebx);
}

/*
 * Print a string to the console.
 * Params:
 *   state->ebx - user pointer of string to be printed
 *   state->ecx - number of characters to print
 * Returns: 0 if successful, -1 if not
 */
static int Sys_PrintString(struct Interrupt_State* state)
{
    int rc = 0;
    uint_t length = state->ecx;
    uchar_t* buf = 0;

    if (length > 0) {
        /* Copy string into kernel. */
        if ((rc = Copy_User_String(state->ebx, length, 1023, (char**) &buf)) != 0)
            goto done;

        /* Write to console. */
        Put_Buf((char*)buf, length);
    }

done:
    if (buf != 0)
        Free(buf);
    return rc;
    
}

/*
 * Get a single key press from the console.
 * Suspends the user process until a key press is available.
 * Params:
 *   state - processor registers from user mode
 * Returns: the key code
 */
static int Sys_GetKey(struct Interrupt_State* state)
{
    struct Kernel_Thread* kthread = g_currentThread;

    while(kthread != 0 && kthread->userContext != 0)
    {
        //if(kthread->userContext->background)
        //{
        //    return -1;
        //}

        kthread = kthread->owner;
    }

    return Wait_For_Key();
    
}

/*
 * Set the current text attributes.
 * Params:
 *   state->ebx - character attributes to use
 * Returns: always returns 0
 */
static int Sys_SetAttr(struct Interrupt_State* state)
{
    TODO("SetAttr system call");
}

/*
 * Get the current cursor position.
 * Params:
 *   state->ebx - pointer to user int where row value should be stored
 *   state->ecx - pointer to user int where column value should be stored
 * Returns: 0 if successful, -1 otherwise
 */
static int Sys_GetCursor(struct Interrupt_State* state)
{
    int row, col;
    Get_Cursor(&row, &col);
    if (!Copy_To_User(state->ebx, &row, sizeof(int)) ||
        !Copy_To_User(state->ecx, &col, sizeof(int)))
        return -1;
    return 0;

}

/*
 * Set the current cursor position.
 * Params:
 *   state->ebx - new row value
 *   state->ecx - new column value
 * Returns: 0 if successful, -1 otherwise
 */
static int Sys_PutCursor(struct Interrupt_State* state)
{
    TODO("PutCursor system call");
}

/*
 * Create a new user process.
 * Params:
 *   state->ebx - user address of name of executable
 *   state->ecx - length of executable name
 *   state->edx - user address of command string
 *   state->esi - length of command string
 * Returns: pid of process if successful, error code (< 0) otherwise
 */
static int Sys_Spawn(struct Interrupt_State* state)
{
    int rc;
    char *program = 0;
    char *command = 0;
    struct Kernel_Thread *process;

    /* Copy program name and command from user space. */
    if ((rc = Copy_User_String(state->ebx, state->ecx, VFS_MAX_PATH_LEN, &program)) != 0 ||
        (rc = Copy_User_String(state->edx, state->esi, 1023, &command)) != 0)
                goto done;

            Enable_Interrupts();


    /*
     * Now that we have collected the program name and command string
     * from user space, we can try to actually spawn the process.
     */
    rc = Spawn(program, command, &process);
    if (rc == 0) {
        KASSERT(process != 0);
        rc = process->pid;
    }

    Disable_Interrupts();

done:
    if (program != 0)
        Free(program);
    if (command != 0)
        Free(command);

    return rc;

}

/*
 * Wait for a process to exit.
 * Params:
 *   state->ebx - pid of process to wait for
 * Returns: the exit code of the process,
 *   or error code (< 0) on error
 */
static int Sys_Wait(struct Interrupt_State* state)
{
    int exitCode;
    struct Kernel_Thread *kthread = Lookup_Thread(state->ebx);
    //if(kthread == 0 || kthread->userContext->background)
    if(kthread == 0)
    {
        return -1;
    }

    Enable_Interrupts();
    exitCode = Join(kthread);
    Disable_Interrupts();

    return exitCode;

}

/*
 * Get pid (process id) of current thread.
 * Params:
 *   state - processor registers from user mode
 * Returns: the pid of the current thread
 */
static int Sys_GetPID(struct Interrupt_State* state)
{
     return g_currentThread->pid;

}

/*
 * Set the scheduling policy.
 * Params:
 *   state->ebx - policy,
 *   state->ecx - number of ticks in quantum
 * Returns: 0 if successful, -1 otherwise
 */
static int Sys_SetSchedulingPolicy(struct Interrupt_State* state)
{
    int policy = state->ebx;
    int quantum = state->ecx;
    
    if(policy!=0 && policy!=1){
    	return -1;
    }
    if(quantum<2 || quantum>100){
	return -1;
    }
    
    if(g_currentSchedulingPolicy != policy){
        g_prevSchedulingPolicy = g_currentSchedulingPolicy;
        g_currentSchedulingPolicy = policy;
    }

    g_Quantum=quantum;
    return 0;
}

/*
 * Get the time of day.
 * Params:
 *   state - processor registers from user mode
 *
 * Returns: value of the g_numTicks global variable
 */
static int Sys_GetTimeOfDay(struct Interrupt_State* state)
{
	return g_numTicks;
}

/*
 * Create a semaphore.
 * Params:
 *   state->ebx - user address of name of semaphore
 *   state->ecx - length of semaphore name
 *   state->edx - initial semaphore count
 * Returns: the global semaphore id
 */
static int Sys_CreateSemaphore(struct Interrupt_State* state)
{
	char *semaphoreName;
   	semaphoreName = (char*) state->ebx;

   	if (state->ecx > MAX_SEMAPHORE_NAME_LENGHT){
    	Print("Error: Max semaphore name lenght exceded.\n");	
       	return -1;
   	} 

   	if (!Copy_From_User(semaphoreName, state->ebx, state->ecx)){
      	Print("Error: Fail to copy memory from user buffer to kernel buffer.\n");
      	return -1;
   	}

   	// find a slot for this semaphore, 
   	// making sure there are no other sempahores with this name
   	int index = 0;
   	for(; index < TOTAL_SEMAPHORES; index++){
		if(semaphores[index].name[0] != NULL && strcmp(semaphoreName, semaphores[index].name ) == 0){
			/*
			 * This is the case where the semaphore was created before. 
			 */
			semaphores[index].num_ref+= 1; /* Increment the reference to this semaphore by 1. */
         	return index;
      		}

      	if(semaphores[index].name[0] == NULL){
			/*
             * This is the case where we create the semaphore if there is a slot for it. 
             */
         	memcpy(semaphores[index].name, semaphoreName, state->ecx);
         	semaphores[index].value = state->edx;
			semaphores[index].id    = index;
			semaphores[index].num_ref = 1;
			semaphores[index].exist   = true;
                Print("Create Semaphores state->edx %d\n", state->edx);
                Print("Create Semaphroes index %d\n", index);
                Print("Create Semaphores semaphores[index].value %d\n", semaphores[index].value);
         	return index;
      	}
   	}

	Print("Error: there is no slot for this semaphore!\n");
   	return -1;
}

/*
 * Acquire a semaphore.
 * Assume that the process has permission to access the semaphore,
 * the call will block until the semaphore count is >= 0.
 * Params:
 *   state->ebx - the semaphore id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_P(struct Interrupt_State* state)
{
	if( state->ebx < 0 || semaphores[state->ebx].exist == false ){
		Print("*** The semaphore with ID = %d does not exist.\n", state->ebx);
        return -1; // semaphore ID does not exist.
	}

	// Third, check if the value of the semaphore < 0.
   	if(semaphores[state->ebx].value <= 0)
   	{
      	Print("*** The %s_sem is not available for  PID = %d\n", semaphores[state->ebx].name, Sys_GetPID(state));
        Print("*** semaphores[state->ebx].value %d\n", semaphores[state->ebx].value);
        Print("*** state->ebx %d\n", state->ebx);
        Wait(&semaphores[state->ebx].waitQueue);
	return -1;
   	}
         
        Print("*** Semaphore %s id %d acquired \n", semaphores[state->ebx].name, state->ebx);
   	semaphores[state->ebx].value--;
   	return 0;

}

/*
 * Release a semaphore.
 * Params:
 *   state->ebx - the semaphore id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_V(struct Interrupt_State* state)
{
	// First, check if the semaphore exist otherwise, distroied,  return -1.
	if( state->ebx < 0 || semaphores[state->ebx].exist == false ){
        Print("*** The semaphore with ID = %d does not exist.\n", state->ebx);
        return -1; // semaphore ID does not exist.
    }

   	if(semaphores[state->ebx].value <= 0)
   	{
      	Print("*** Some threads will wake up.\n");
      	Wake_Up(&semaphores[state->ebx].waitQueue);
   	}

        Print("*** Semaphore %s id %d released\n", semaphores[state->ebx].name, state->ebx);
   	semaphores[state->ebx].value++;
   	return 0;
}

/*
 * Destroy a semaphore.
 * Params:
 *   state->ebx - the semaphore id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_DestroySemaphore(struct Interrupt_State* state)
{
   	//TODO("DestroySemaphore system call");
	// First, we need to remove the authority for accessing the semaphore.


	// Second, check the number of references to this semaphore.
	// Assuming I have authority on this thread, meaning I called Create_Semaphore() at least once.
	if(semaphores[state->ebx].num_ref == 1){
		semaphores[state->ebx].exist   = false;
		semaphores[state->ebx].name[0] = NULL;
		semaphores[state->ebx].id      = -1;
		semaphores[state->ebx].value   = -1;
		return 0;
	}

   	//semaphores[state->ebx].name[0] = NULL;
   	//semaphores[state->ebx].value   = 0;
   	//Print("*** DestroySemaphore is called.\n");
   	return 0;
}


/*
 * Global table of system call handler functions.
 */
const Syscall g_syscallTable[] = {
    Sys_Null,
    Sys_Exit,
    Sys_PrintString,
    Sys_GetKey,
    Sys_SetAttr,
    Sys_GetCursor,
    Sys_PutCursor,
    Sys_Spawn,
    Sys_Wait,
    Sys_GetPID,
    /* Scheduling and semaphore system calls. */
    Sys_SetSchedulingPolicy,
    Sys_GetTimeOfDay,
    Sys_CreateSemaphore,
    Sys_P,
    Sys_V,
    Sys_DestroySemaphore,
};

/*
 * Number of system calls implemented.
 */
const int g_numSyscalls = sizeof(g_syscallTable) / sizeof(Syscall);
