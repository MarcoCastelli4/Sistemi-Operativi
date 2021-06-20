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

	/*
	* File: mentos/inc/libc
	/// @brief Structure used to implement the list_head data structure.
	typedef struct list_head {
		/// @brief The previous element.
		struct list_head *prev;
		/// @brief The subsequent element.
		struct list_head *next;
	} list_head;

	*/
	// Prendo la testa della lista dei processi
	// runqueue: lista dei processi da eseguire
	// run_list : contiene prev e next del processo curr.
	// curr {task_struct} 
	struct list_head *nNode=runqueue->curr->run_list.next;

	// Se la testa si trova nella coda prendo il successivo
	if(nNode==&runqueue->queue)
		nNode=nNode->next;

	// Assegno come prossimo processo il selezionato

	next=list_entry(nNode,struct task_struct,run_list);


	//=======================================================================
#elif defined(SCHEDULER_PRIORITY)
	//==== Implementatin of the Priority Scheduling algorithm ===============
	
	// Prendi il primo elemento della runqueue
	next = list_entry(&runqueue->queue, task_struct, run_list);

	// Assegno a min la sua priorità.
	unsigned int min = next->se.prio;

	list_head * it;
	// Itera sulla runqueue per cercare un elemento a priorità minore (da eseguire prima) 
	list_for_each (it, &runqueue->queue) {
		task_struct *entry = list_entry(it, task_struct, run_list);
		// Verifica se entry ha una priorità minore 
		if (entry->se.prio <= min) {
			min = entry->se.prio;
			// Ho una priorità minore quindi sono il nuovo processo candidato ad essere eseguito
			next = entry;
		}
	}

	//=======================================================================
#elif defined(SCHEDULER_CFS)
	//==== Implementation of the Completely Fair Scheduling ==================

	// Get the weight of the current process.
	// (use GET_WEIGHT macro!)
	int weight = GET_WEIGHT(runqueue->curr->se.prio);

	// NICE_0_LOAD: weight di una task con priorità normale 
	if (weight != NICE_0_LOAD){
		// get the multiplicative factor for its delta_exec.
		double factor = NICE_0_LOAD/weight;

		// delta_exec: tempo passato in esecuzione
		// delta_exec viene aggiustato con factor (priorità pesata)
		delta_exec = delta_exec*factor;
	}

	// Update vruntime of the current process.
	// vruntime: tempo in cui il processo è stato in attesa
	// delta exec viene passato come parametro
	runqueue->curr->se.vruntime += delta_exec;

	// Get the first element of the list
	next = list_entry(&runqueue->queue, task_struct, run_list);

	// Get its virtual runtime.
	// vruntime: tempo di attesa accumulato 
	time_t min = next->se.vruntime;

	// Iter over the runqueue to find the task with the smallest vruntime value
	// Itera sulla runqueue alla ricerca del task con vruntime minore
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
