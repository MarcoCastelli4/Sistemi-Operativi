///                MentOS, The Mentoring Operating system project
/// @file deadlock_prevention.c
/// @brief Deadlock prevention algorithms source code.
/// @copyright (c) 2019 This file is distributed under the MIT License.
/// See LICENSE.md for details.
///
/// Complete request() and state_safe() functions, according to what you learnt
/// in deadlock lectures.
///
/// In order to complete the functions I prepared for you a library to manage
/// easily arrays. The library is arr_math.h and you can find the documentation
/// in include/util/arr_math.h header file.
///
/// In addition you will find the same data structures that you have seen in
/// class:
/// - available: number of resources instances currently available;
/// - max: matrix of the maximum number of resources instances that each task
/// may require;
/// - alloc: matrix of current resources instances allocation of each task.
/// - need: matrix of current resources instances needs of each task.
/// Assume that these data structure are already filled with the
/// information described, in other words, you need to use these data structures
/// in read-only.
///
/// Suggestion!
/// From arr_math.h you may need only: arr_g_any(), arr_add(), arr_sub(),
/// all() and arr_ne();

#include "deadlock_prevention.h"

#include "arr_math.h"
#include "kheap.h"

/// Array of resources instances currently available;
uint32_t *  available;
/// Matrix of the maximum resources instances that each task may require;
uint32_t ** max;
/// Matrix of current resources instances allocation of each task.
uint32_t ** alloc;
/// Matrix of current resources instances need of each task.
uint32_t ** need;

/// @brief Check if the current system resource allocation maintains the system
/// in a safe state.
/// @param n Number of tasks currently in the system.
/// @param m Number of resource types in the system (length of req_vec).
static bool_t state_safe(size_t n, size_t m)
{
    // Alloco work come copia available.
    uint32_t *work = memcpy(kmalloc(sizeof(uint32_t) * m), available,
                            sizeof(uint32_t) * m);

    // Alloco finish inizializzato con tutti falso (zero in c).
    uint32_t *finish = all(kmalloc(sizeof(uint32_t) * n), 0UL, n);
    uint32_t *all_true = all(kmalloc(sizeof(uint32_t) * n), 1UL, n);

    int i;
    // Loop while finish is not equal an array all true (ones).
    // arr_ne ritorna true se esiste un elemento dell'array di sx differente dal corrispettivo in quello di dx 
    while (arr_ne(finish,all_true,n))
    {
        // Cerca una task che riesce a soddisfare la richieste e di conseguenza rilascerebbe le sue risorse.
        for (i = 0; i < n && (finish[i] || arr_g_any(need[i],work,m)); i++);
        // Sono arrivato in fondo alla lista e nessun processo può terminare quindi ritorno false
        if (i == n)
        {
            // Free memory.
            kfree(work);
            kfree(finish);
            kfree(all_true);
            return false;
        }
        else
        {
            // Assume to make available the resources that the task found needs.
            // Assumo che le risorse del processo i-esimo ritornino disponibili e metto finish a true
           arr_add(work,alloc[i],m);
           finish[i]=1;
        }
    }

    // Free memory.
    kfree(work);
    kfree(finish);
    kfree(all_true);
    // esiste la sequenza SAFE
    return true;
}

deadlock_status_t request(uint32_t *req_vec, size_t task_i, size_t n, size_t m)
{
    // Controlla che le risorse richieste non superino il valore dichiarato dal processo inizialmente
    // arr_g_any: confronto tra array, verifica che ogni valore dell'array di sx sia minore o uguale a di quello di dx
    if (arr_g_any(req_vec,need[task_i],m))
    {
        return ERROR;
    }
    // Controlla che ci siano risorse disponibili per accontentare la richiesta
    if (arr_g_any(req_vec,available,m))
    {
        return WAIT;
    }

    // Simulo l'allocazione delle risorse.
    arr_sub(available,req_vec,m);
    arr_add(alloc[task_i],req_vec,m);
    arr_sub(need[task_i],req_vec,m);

    // Controlla se accontentando la richiesta si resta in stato safe o meno
    if (!state_safe(n,m))
    {   
        // Non è safe quindi annullo la simulazione fatta in precedenza
        // ripristinando i valori precedenti delle strutture
        arr_add(available,req_vec,m);
        arr_sub(alloc[task_i],req_vec,m);
        arr_add(need[task_i],req_vec,m);
        return WAIT_UNSAFE;
    }
    // è safe
    return SAFE;
}
