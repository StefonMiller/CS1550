#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "museumsim.h"

//
// In all of the definitions below, some code has been provided as an example
// to get you started, but you do not have to use it. You may change anything
// in this file except the function signatures.
//


struct shared_data {
	// Add any relevant synchronization constructs and shared state here.
	// For example:
	pthread_mutex_t ticket_mutex;
	pthread_cond_t  guide_wait_to_leave;
	pthread_cond_t  guide_wait_to_admit;
	pthread_cond_t  guide_wait_to_enter;
	pthread_cond_t  visitor_wait_to_enter;
	int tickets;
	int curr_ticket;
	int in_queue;
	int curr_guides;
	int curr_inside;
	int guides_finished;
};

static struct shared_data shared;


/**
 * Set up the shared variables for your implementation.
 * 
 * `museum_init` will be called before any threads of the simulation
 * are spawned.
 */
void museum_init(int num_guides, int num_visitors)
{
	//Initialize mutex
	pthread_mutex_init(&shared.ticket_mutex, NULL);
	//Set number of tickets to the number of guides * the number of visitors per guide. If the actual number of visitors is less than that, set it to the number of visitors
	shared.tickets = MIN(VISITORS_PER_GUIDE * num_guides, num_visitors);
	//Initialize condition variables
	pthread_cond_init(&shared.guide_wait_to_leave, NULL);
	pthread_cond_init(&shared.guide_wait_to_admit, NULL);
	pthread_cond_init(&shared.guide_wait_to_enter, NULL);
	pthread_cond_init(&shared.visitor_wait_to_enter, NULL);
	//Set the current ticket number to the number of total tickets
	shared.curr_ticket = shared.tickets + 1;
	//Number of guests waiting to tour
	shared.in_queue = 0;
	//Number of guides in the museum
	shared.curr_guides = 0;
	//Number of visitors in the museum
	shared.curr_inside = 0;
	//Number of guides waiting to leave
	shared.guides_finished = 0;
}


/**
 * Tear down the shared variables for your implementation.
 * 
 * `museum_destroy` will be called after all threads of the simulation
 * are done executing.
 */
void museum_destroy()
{
	pthread_mutex_destroy(&shared.ticket_mutex);
	pthread_cond_destroy(&shared.guide_wait_to_leave);
	pthread_cond_destroy(&shared.guide_wait_to_admit);
	pthread_cond_destroy(&shared.guide_wait_to_enter);
	pthread_cond_destroy(&shared.visitor_wait_to_enter);
}


/**
 * Implements the visitor arrival, touring, and leaving sequence.
 */
void visitor(int id)
{
	//Indicate visitor arrival
	visitor_arrives(id);
	//Lock mutex
	pthread_mutex_lock(&shared.ticket_mutex);
	{
		//Check if there are tickets left to claim
		if(shared.tickets == 0)
		{
			//If no tickets left, leave the visitor queue
			pthread_mutex_unlock(&shared.ticket_mutex);
			return;
		}
		//If there are tickets, claim one
		int ticket_num = shared.tickets--;
		//Increment the in_queue variable to indicate the user is in queue
		shared.in_queue++;
		//Broadcast the user is in queue to all guides
		pthread_cond_broadcast(&shared.guide_wait_to_admit);
		//Once in queue, wait to be admitted
		while(ticket_num < shared.curr_ticket)
		{
			pthread_cond_wait(&shared.visitor_wait_to_enter, &shared.ticket_mutex);
		}
	}
	//Unlock mutex before touring for concurrent tours to take place
	pthread_mutex_unlock(&shared.ticket_mutex);
	visitor_tours(id);
	//Re-lock mutex after touring
	pthread_mutex_lock(&shared.ticket_mutex);
	{
		//After touring, decrement the curr-inside variable
		shared.curr_inside--;
		//Broadcast to guides waiting to leave in case they are waiting on visitors to leave
		pthread_cond_broadcast(&shared.guide_wait_to_leave);
		visitor_leaves(id);
	}
	//After we indicate the visitor left, unlock the mutex and return
	pthread_mutex_unlock(&shared.ticket_mutex);
	return;
	
}

/**
 * Implements the guide arrival, entering, admitting, and leaving sequence.
 */
void guide(int id)
{
	//Indicate the guide has arrived
	guide_arrives(id);
	//Once a guide has arrived, lock the mutex
	pthread_mutex_lock(&shared.ticket_mutex);
	{
		//If there are no tickets left and there are no visitors in queue, then the guide can leave before entering
		if(shared.tickets == 0 && shared.in_queue == 0)
		{
			pthread_mutex_unlock(&shared.ticket_mutex);
			return;
		}
		//Wait outside if there are guides waiting to leave the museum or the museum has the max number of guides
		while(shared.guides_finished != 0 || shared.curr_guides == GUIDES_ALLOWED_INSIDE)
		{
			pthread_cond_wait(&shared.guide_wait_to_enter, &shared.ticket_mutex);
		}
		//Once the guide can enter, indicate that they enter the musuem
		guide_enters(id);
		//Increment the number of guides inside
		shared.curr_guides++;
		//Keep track of visitors admitted by the guide
		int admitted_visitors = 0;
		//Admit visitors until either at max value or no more left to admit
		while(admitted_visitors < VISITORS_PER_GUIDE)
		{
			//If there are no more tickets and no more visitors in the queue, break out of the loop
			if(shared.tickets == 0 && shared.in_queue == 0)
			{
				break;
			}

			//Flag indicating whether or not there are visitors left to admit
			int visitors_left = 1;

			//Wait for visitors to enter or the musuem to get space
			while(shared.in_queue == 0)
			{
				pthread_cond_wait(&shared.guide_wait_to_admit, &shared.ticket_mutex);
				//While waiting for visitors, if there are no tickets left, set the flag and exit the loop
				if(shared.tickets == 0 && shared.in_queue == 0)
				{
					visitors_left = 0;
					break;
				}
			}
			//Check flag before proceeding to make sure there are visitors left for the guide to admit
			if(visitors_left == 0)
			{
				break;
			}

			//Once we have space for the visitor and there are visitors in the queue, admit one
			guide_admits(id);
			//Remove visitor from queue
			shared.in_queue--;
			//Decrement the current ticket number
			shared.curr_ticket--;
			//Increment the number of visitors inside
			shared.curr_inside++;
			//Increment the number of visitors admitted by the guide
			admitted_visitors++;
			//Broadcast to visitors that they can enter the musuem
			pthread_cond_broadcast(&shared.visitor_wait_to_enter);
		}
		//Increment the number of guides finished once we break out of the loop
		shared.guides_finished++;
		//Broadcast to other potential guides once we finish
		pthread_cond_broadcast(&shared.guide_wait_to_leave);
		//Once a guide has finished, wait for all visitors to leave and the other guide to finish
		while(shared.curr_inside != 0 || shared.guides_finished < shared.curr_guides)
		{
			pthread_cond_wait(&shared.guide_wait_to_leave, &shared.ticket_mutex);
		}
		//Once the guides are all finished, leave together
		shared.guides_finished--;
		//Decrement number of guides in the museum
		shared.curr_guides--;
		pthread_cond_broadcast(&shared.guide_wait_to_enter);
		guide_leaves(id);

	}
	pthread_mutex_unlock(&shared.ticket_mutex);
	return;
}
