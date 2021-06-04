/// @file scheduler_algorithm.c
/// @brief Round Robin algorithm.
/// @date Mar 2019.

#include "prio.h"
#include "debug.h"
#include "assert.h"
#include "list_head.h"
#include "scheduler.h"

#define GET_WEIGHT(prio) prio_to_weight[USER_PRIO((prio))]
#define NICE_0_LOAD GET_WEIGHT(DEFAULT_PRIO)

task_struct *pick_next_task(runqueue_t *runqueue, time_t delta_exec)
{
	// Pointer to the next task to schedule.
	task_struct *next = NULL;

#if defined(SCHEDULER_RR)
	//==== Implementatin of the Round-Robin Scheduling algorithm ============

	// Prendo la testa della lista dei processi
	struct list_head *nNode=runqueue->curr->run_list.next;

	// Se la testa si trova nella coda prendo il successivo
	if(nNode==&runqueue->queue)
		nNode=nNode->next;

	// Assegno come prossimo processo il selezionato
	next=list_entry(nNode,struct task_struct,run_list);


	//=======================================================================
#elif defined(SCHEDULER_PRIORITY)
	//==== Implementatin of the Priority Scheduling algorithm ===============
	
	// get the first element of the list
	next = list_entry(&runqueue->queue, task_struct, run_list);

	// Get its static priority.
	unsigned int min = next->se.prio;

	list_head * it;
	// Iter over the runqueue to find the task with the smallest priority value
	list_for_each (it, &runqueue->queue) {
		task_struct *entry = list_entry(it, task_struct, run_list);
		// Check if the entry has a lower priority
		if (entry->se.prio <= min) {
			min = entry->se.prio;
			next = entry;
		}
	}

	//=======================================================================
#elif defined(SCHEDULER_CFS)
	//==== Implementation of the Completely Fair Scheduling ==================

	// Get the weight of the current process.
	// (use GET_WEIGHT macro!)
	int weight = GET_WEIGHT(runqueue->curr->se.prio);

	if (weight != NICE_0_LOAD){
		// get the multiplicative factor for its delta_exec.
		double factor = NICE_0_LOAD/weight;

		// weight the delta_exec with the multiplicative factor.
		delta_exec = delta_exec*factor;
	}

	// Update vruntime of the current process.
	runqueue->curr->se.vruntime += delta_exec;

	// Get the first element of the list
	next = list_entry(&runqueue->queue, task_struct, run_list);


	// Get its virtual runtime.
	time_t min = next->se.vruntime;

	// Iter over the runqueue to find the task with the smallest vruntime value
	list_head * it;
	list_for_each (it, &runqueue->queue) {
		task_struct *entry = list_entry(it, task_struct, run_list);
		// Check if the entry has a lower priority
		if (entry->se.vruntime <= min) {
			min = entry->se.vruntime;
			next = entry;
		}
	}

	//========================================================================
#else
#error "You should enable a scheduling algorithm!"
#endif
	assert(next && "No valid task selected. Have you implemented a scheduling algorithm?");

	return next;
}
