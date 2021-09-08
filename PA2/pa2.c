/**********************************************************************
 * Copyright (c) 2019-2021
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

/* THIS FILE IS ALL YOURS; DO WHATEVER YOU WANT TO DO IN THIS FILE */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>


#include "types.h"
#include "list_head.h"

struct BoostingType {
    bool pip;
    bool pcp;
};
struct BoostingType boostingType = {
        .pip = false,
        .pcp = false
};
bool first;

/**
 * The process which is currently running
 */
#include "process.h"
extern struct process *current;


/**
 * List head to hold the processes ready to run
 */
extern struct list_head readyqueue;


/**
 * Resources in the system.
 */
#include "resource.h"
extern struct resource resources[NR_RESOURCES];


/**
 * Monotonically increasing ticks
 */
extern unsigned int ticks;


/**
 * Quiet mode. True if the program was started with -q option
 */
extern bool quiet;


/***********************************************************************
 * Default FCFS resource acquision function
 *
 * DESCRIPTION
 *   This is the default resource acquision function which is called back
 *   whenever the current process is to acquire resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
bool fcfs_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}

	/* OK, this resource is taken by @r->owner. */

	/* Update the current process state */
	current->status = PROCESS_WAIT;

	/* And append current to waitqueue */
	list_add_tail(&current->list, &r->waitqueue);

	/**
	 * And return false to indicate the resource is not available.
	 * The scheduler framework will soon call schedule() function to
	 * schedule out current and to pick the next process to run.
	 */
	return false;
}

/***********************************************************************
 * Default FCFS resource release function
 *
 * DESCRIPTION
 *   This is the default resource release function which is called back
 *   whenever the current process is to release resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
void fcfs_release(int resource_id)
{
	struct resource *r = resources + resource_id;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);

	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue)) {
		struct process *waiter =
				list_first_entry(&r->waitqueue, struct process, list);

		/**
		 * Ensure the waiter is in the wait status
		 */
		assert(waiter->status == PROCESS_WAIT);

		/**
		 * Take out the waiter from the waiting queue. Note we use
		 * list_del_init() over list_del() to maintain the list head tidy
		 * (otherwise, the framework will complain on the list head
		 * when the process exits).
		 */
		list_del_init(&waiter->list);

		/* Update the process status */
		waiter->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&waiter->list, &readyqueue);
	}
}



#include "sched.h"

/***********************************************************************
 * FIFO scheduler
 ***********************************************************************/
static int fifo_initialize(void)
{
	return 0;
}

static void fifo_finalize(void)
{
}

static struct process *fifo_schedule(void)
{
	struct process *next = NULL;

	/* You may inspect the situation by calling dump_status() at any time */
	// dump_status();

	/**
	 * When there was no process to run in the previous tick (so does
	 * in the very beginning of the simulation), there will be
	 * no @current process. In this case, pick the next without examining
	 * the current process. Also, when the current process is blocked
	 * while acquiring a resource, @current is (supposed to be) attached
	 * to the waitqueue of the corresponding resource. In this case just
	 * pick the next as well.
	 */
	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	/* The current process has remaining lifetime. Schedule it again */
	if (current->age < current->lifespan) {
		return current;
	}

pick_next:
	/* Let's pick a new process to run next */

	if (!list_empty(&readyqueue)) {
		/**
		 * If the ready queue is not empty, pick the first process
		 * in the ready queue
		 */
		next = list_first_entry(&readyqueue, struct process, list);

		/**
		 * Detach the process from the ready queue. Note we use list_del_init()
		 * instead of list_del() to maintain the list head tidy. Otherwise,
		 * the framework will complain (assert) on process exit.
		 */
		list_del_init(&next->list);
	}

	/* Return the next process to run */
	return next;
}

struct scheduler fifo_scheduler = {
	.name = "FIFO",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.initialize = fifo_initialize,
	.finalize = fifo_finalize,
	.schedule = fifo_schedule,
};


/***********************************************************************
 * SJF scheduler
 ***********************************************************************/
static struct process *sjf_schedule(void)
{
	/**
	 * Implement your own SJF scheduler here.
	 */
//     dump_status();

    struct process *next = NULL;

    if (!current || current->status == PROCESS_WAIT) {
        goto pick_next;
    }

    /* The current process has remaining lifetime. Schedule it again */
    if (current->age < current->lifespan) {
        return current;
    }

pick_next:
    /* Let's pick a new process to run next */

    if (!list_empty(&readyqueue)) {

        /**
                * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
        * @pos:	the type * to use as a loop cursor.
        * @n:		another type * to use as temporary storage
        * @head:	the head for your list.
        * @member:	the name of the list_head within the struct.
        */

        struct process *pos = NULL;
        struct process *n = NULL;
        struct process *temp = NULL;

        list_for_each_entry_safe(pos,n,&readyqueue,list) {
            if(temp == NULL) {
                temp = pos;
            }
            else if ( temp != NULL && temp->lifespan > pos->lifespan) {
                temp = pos;
            }
        }

        next = temp;
        list_del_init(&next->list);
    }

    /* Return the next process to run */
    return next;
}

struct scheduler sjf_scheduler = {
	.name = "Shortest-Job First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = sjf_schedule,		 /* TODO: Assign sjf_schedule()
								to this function pointer to activate
								SJF in the system */
};


/***********************************************************************
 * SRTF scheduler
 ***********************************************************************/


static struct process *srtf_schedule(void)
{
    /**
     * Implement your own SrtF scheduler here.
     */
    struct process *next = NULL;

    if (!current || current->status == PROCESS_WAIT) {
        goto pick_next;
    }

    /* The current process has remaining lifetime. Schedule it again */
    if (current->age < current->lifespan) {
        list_move(&current->list, &readyqueue);
        goto pick_next; // return x
    }

pick_next:
    /* Let's pick a new process to run next */

    if (!list_empty(&readyqueue)) {

        struct process *pos = NULL;
        struct process *n = NULL;
        struct process *temp = NULL;

        list_for_each_entry_safe(pos,n,&readyqueue,list) {
            if(temp == NULL) {
                temp = pos;
            }
            else if ( temp != NULL && temp->lifespan > pos->lifespan) {
                temp = pos;
            }
        }
        next = temp;
        /**
         * Detach the process from the ready queue. Note we use list_del_init()
         * instead of list_del() to maintain the list head tidy. Otherwise,
         * the framework will complain (assert) on process exit.
         */
        list_del_init(&next->list);
    }

    /* Return the next process to run */
    return next;
}
struct scheduler srtf_scheduler = {
	.name = "Shortest Remaining Time First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = srtf_schedule,
	/* You need to check the newly created processes to implement SRTF.
	 * You may use @forked() callback to mark newly created processes */
	/* Obviously, you should implement srtf_schedule() and attach it here */
};


/***********************************************************************
 * Round-robin scheduler
 ***********************************************************************/
static struct process *rr_schedule(void)
{
    /**
     * Implement your own rr scheduler here.
     */


    struct process *next = NULL;

    if (!current || current->status == PROCESS_WAIT) {
        goto pick_next;
    }

    /* The current process has remaining lifetime. Schedule it again */
    if (current->age < current->lifespan) {
        list_move_tail(&current->list, &readyqueue);
        goto pick_next;
    }

pick_next:
    /* Let's pick a new process to run next */

    if (!list_empty(&readyqueue)) {
        next = list_first_entry(&readyqueue, struct process, list);
        /**
         * Detach the process from the ready queue. Note we use list_del_init()
         * instead of list_del() to maintain the list head tidy. Otherwise,
         * the framework will complain (assert) on process exit.
         */
        list_del_init(&next->list);
    }

    /* Return the next process to run */
    return next;
}

struct scheduler rr_scheduler = {
	.name = "Round-Robin",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = rr_schedule,
	/* Obviously, you should implement rr_schedule() and attach it here */
};


/***********************************************************************
 * Default FCFS resource acquision function
 *
 * DESCRIPTION
 *   This is the default resource acquision function which is called back
 *   whenever the current process is to acquire resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/

static int prio_initialize(void)
{
    boostingType.pip = false;
    boostingType.pcp = false;
    first = true;

    struct process *pos = NULL;
    struct process *n = NULL;
    struct process *temp = NULL;

    list_for_each_entry_safe(pos,n,&readyqueue,list) {
        pos->prio_orig = pos->prio;
    }

    return 0;
}

bool prio_acquire(int resource_id)
{
    struct resource *r = resources + resource_id;

    if (!r->owner) {
        /* This resource is not owned by any one. Take it! */
        r->owner = current;
        //when it gets the resource
        if(boostingType.pcp == true) {
            r->owner->prio = MAX_PRIO;
        }
        return true;
    }


    /* OK, this resource is taken by @r->owner. */

    if (boostingType.pip == true){
        if(current -> prio > r->owner->prio){
            r->owner->prio = current->prio;
        }
    }

    /* Update the current process state */
    current->status = PROCESS_WAIT;

    /* And append current to waitqueue */
    list_move(&current->list, &r->waitqueue);

    /**
     * And return false to indicate the resource is not available.
     * The scheduler framework will soon call schedule() function to
     * schedule out current and to pick the next process to run.
     */
    return false;
}

/***********************************************************************
 * Default FCFS resource release function
 *
 * DESCRIPTION
 *   This is the default resource release function which is called back
 *   whenever the current process is to release resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
void prio_release(int resource_id)
{
    struct resource *r = resources + resource_id;

    /* Ensure that the owner process is releasing the resource */
    assert(r->owner == current);
    r->owner->prio = r->owner->prio_orig;
    /* Un-own this resource */
    r->owner = NULL;


    /* Let's wake up ONE waiter (if exists) that came first */
    if (!list_empty(&r->waitqueue)) {

//
        struct process *waiter = NULL;
        struct process *n = NULL;
        list_for_each_entry_safe(waiter, n, &r->waitqueue,list) {
            list_del_init(&waiter->list);
            waiter->status = PROCESS_READY;
            list_move(&waiter->list, &readyqueue);
            list_del_init(&current->list);
        }


    }
}

/***********************************************************************
 * Priority scheduler
 ***********************************************************************/


static struct process *prio_schedule(void)
{
    /**
     * Implement your own prio scheduler here.
     */
    struct process *next = NULL;

    if (!current || current->status == PROCESS_WAIT) {
        goto pick_next;
    }

    /* The current process has remaining lifetime. Schedule it again */
    if (current->age < current->lifespan) {
        list_move(&current->list, &readyqueue);
        goto pick_next;
    }

pick_next:
    /* Let's pick a new process to run next */

    if (!list_empty(&readyqueue)) {

        struct process *pos = NULL;
        struct process *n = NULL;
        struct process *temp = NULL;

        list_for_each_entry_safe(pos,n,&readyqueue,list) {
            if(temp == NULL) {
                temp = pos;
            }
            else if ( temp != NULL && temp->prio < pos->prio) {
                temp = pos;
            }
        }
        next = temp;
        /**
         * Detach the process from the ready queue. Note we use list_del_init()
         * instead of list_del() to maintain the list head tidy. Otherwise,
         * the framework will complain (assert) on process exit.
         */
        list_del_init(&next->list);
    }

    /* Return the next process to run */
    return next;
}
struct scheduler prio_scheduler = {
	.name = "Priority",
	.acquire = prio_acquire,
	.release = prio_release,
	.initialize = prio_initialize,
	.schedule = prio_schedule,
	/**
	 * Implement your own acqure/release function to make priority
	 * scheduler correct.
	 */
	/* Implement your own prio_schedule() and attach it here */
};


/***********************************************************************
 * Priority scheduler with aging
 ***********************************************************************/


static struct process *pa_schedule(void)
{
    /**
     * Implement your own pa scheduler here.
     */
//     dump_status();
    struct process *next = NULL;

    if (!current || current->status == PROCESS_WAIT) {
        goto pick_next;
    }

    /* The current process has remaining lifetime. Schedule it again */
    if (current->age < current->lifespan) {
        list_move_tail(&current->list, &readyqueue);
        goto pick_next;
    }

pick_next:
    /* Let's pick a new process to run next */

    if (!list_empty(&readyqueue)) {

        struct process *pos = NULL;
        struct process *n = NULL;
        struct process *temp = NULL;


        list_for_each_entry_safe(pos,n,&readyqueue,list) {
            if(temp == NULL) {
                temp = pos;
            }
            else if ( temp != NULL && temp->prio < pos->prio) {
                temp = pos;
            }
        }
        next = temp;
        next->prio = next->prio_orig;
        /**
         * Detach the process from the ready queue. Note we use list_del_init()
         * instead of list_del() to maintain the list head tidy. Otherwise,
         * the framework will complain (assert) on process exit.
         */
        list_del_init(&next->list);

        list_for_each_entry_safe(pos,n,&readyqueue,list) {
            pos->prio +=1;
        }

        /** for Test **/
        /*
        list_for_each_entry_safe(pos,n,&readyqueue,list) {
            if(temp == NULL) {
                temp = pos;
            }
            else if ( temp != NULL && temp->prio < pos->prio && first == false) {
                temp = pos;
            }
            if (pos->pid == 1) {
                printf("\n Num: %d, \tPrio: %d in tick: %d \n",pos->pid, pos->prio, ticks);
            }
            if (pos->pid == 2) {
                printf("\n Num: %d, \tPrio: %d in tick: %d \n",pos->pid, pos->prio, ticks);
            }
            if (pos->pid == 3) {
                printf("\n Num: %d, \tPrio: %d in tick: %d \n",pos->pid, pos->prio, ticks);
            }
            if (pos->pid == 4) {
                printf("\n Num: %d, \tPrio: %d in tick: %d \n",pos->pid, pos->prio, ticks);
            }
            if (pos->pid == 5) {
                printf("\n Num: %d, \tPrio: %d in tick: %d \n",pos->pid, pos->prio, ticks);
            }
            if (pos->pid == 6) {
                printf("\n Num: %d, \tPrio: %d in tick: %d \n",pos->pid, pos->prio, ticks);
            }
        }
        printf("\n Num: %d, \tPrio: %d in tick: %d \n",next->pid, next->prio, ticks);

         */

    }
    /* Return the next process to run */

    return next;
}

struct scheduler pa_scheduler = {
        /* Implement your own pa_schedule() and attach it here */

	.name = "Priority + aging",
	.acquire = prio_acquire,
	.release = prio_release,
	.initialize = prio_initialize,
	.schedule = pa_schedule,
	/**
	 * Implement your own acqure/release function to make priority
	 * scheduler correct.
	 */
};


/***********************************************************************
 * Priority scheduler with priority ceiling protocol
 ***********************************************************************/

static struct process *pcp_schedule(void)
{
    /**
     * Implement your own pcp scheduler here.
     */

    boostingType.pip = false;
    boostingType.pcp = true;

    struct process *next = NULL;



    if (!current || current->status == PROCESS_WAIT) {
        goto pick_next;
    }

    /* The current process has remaining lifetime. Schedule it again */
    if (current->age < current->lifespan) {
        list_move(&current->list, &readyqueue);
        goto pick_next;
    }

    pick_next:
    /* Let's pick a new process to run next */

    if (!list_empty(&readyqueue)) {

        struct process *pos = NULL;
        struct process *n = NULL;
        struct process *temp = NULL;

        list_for_each_entry_safe(pos,n,&readyqueue,list) {
            if(temp == NULL) {
                temp = pos;
            }
            else if ( temp != NULL && temp->prio <= pos->prio && first == false) {
                temp = pos;
            }
        }
        next = temp;
        /**
         * Detach the process from the ready queue. Note we use list_del_init()
         * instead of list_del() to maintain the list head tidy. Otherwise,
         * the framework will complain (assert) on process exit.
         */
        list_del_init(&next->list);
    }

    /* Return the next process to run */
    first= false;
    return next;
}
struct scheduler pcp_scheduler = {
	.name = "Priority + PCP Protocol",
	.acquire = prio_acquire,
	.release = prio_release,
	.initialize = prio_initialize,
	.schedule = pcp_schedule,
	/**
	 * Implement your own acqure/release function too to make priority
	 * scheduler correct.
	 */
};


/***********************************************************************
 * Priority scheduler with priority inheritance protocol
 ***********************************************************************/



static struct process *pip_schedule(void)
{
    /**
     * Implement your own pip scheduler here.
     */
//    dump_status();
    boostingType.pip = true;
    boostingType.pcp = false;

    struct process *next = NULL;

    if (!current || current->status == PROCESS_WAIT) {
        goto pick_next;
    }

    /* The current process has remaining lifetime. Schedule it again */
    if (current->age < current->lifespan) {
        list_move(&current->list, &readyqueue);
        goto pick_next;
    }

    pick_next:
    /* Let's pick a new process to run next */

    if (!list_empty(&readyqueue)) {

        struct process *pos = NULL;
        struct process *n = NULL;
        struct process *temp = NULL;

        list_for_each_entry_safe(pos,n,&readyqueue,list) {
            if(temp == NULL) {
                temp = pos;
            }
            else if ( temp != NULL && temp->prio < pos->prio && first == false) {
                temp = pos;
            }
            /** for Test **/

//            if (pos->pid == 1) {
//                printf("\n current Num: %d, \tPrio: %d in tick: %d \n",pos->pid, pos->prio, ticks);
//            }
//            if (pos->pid == 2) {
//                printf("\n current Num: %d, \tPrio: %d in tick: %d \n",pos->pid, pos->prio, ticks);
//            }
//            if (pos->pid == 3) {
//                printf("\n current Num: %d, \tPrio: %d in tick: %d \n",pos->pid, pos->prio, ticks);
//            }
//            if (pos->pid == 4) {
//                printf("\n current Num: %d, \tPrio: %d in tick: %d \n",pos->pid, pos->prio, ticks);
//            }
        }
        next = temp;
        /**
         * Detach the process from the ready queue. Note we use list_del_init()
         * instead of list_del() to maintain the list head tidy. Otherwise,
         * the framework will complain (assert) on process exit.
         */
        list_del_init(&next->list);

    }

    /* Return the next process to run */
    first = false;

    return next;
}
struct scheduler pip_scheduler = {
	.name = "Priority + PIP Protocol",
	.acquire = prio_acquire,
	.release = prio_release,
	.initialize = prio_initialize,
	.schedule = pip_schedule,
	/**
	 * Ditto
	 */
};
