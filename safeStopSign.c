/**
* CSC369 Assignment 2
*
* This is the source/implementation file for your safe stop sign
* submission code.
*/
#include "safeStopSign.h"

/**
* @brief Broadcasts a condition variable and does error checking.
*
* @param cond pointer to the condition variable to initialize.
*/
static void broadcastConditionVariable(pthread_cond_t* cond);

/**
* @brief Waits for a condition variable and does error checking.
*
* @param cond pointer to the condition variable to initialize.
*/
static void waitConditionVariable(pthread_cond_t* cond, pthread_mutex_t* mutex);

void initSafeStopSign(SafeStopSign* sign, int count) {
	initStopSign(&sign->base, count);

	// TODO: Add any initialization logic you need.

	// Initialize enterLanelocks, exitLaneLocks and exitLaneConditions
	for (int i = 0; i < DIRECTION_COUNT; ++i) {
		initMutex(&sign->enterLaneLocks[i]);
		initMutex(&sign->exitLaneLocks[i]);
		initConditionVariable(&sign->exitLaneConditions[i]);
	}

	// Initialize enterLaneTokens and exitLaneTokens
	size_t size = sizeof(int) * DIRECTION_COUNT;

	memset(sign->enterLaneTokens, 0, size);
	memset(sign->exitLaneTokens, 0, size);

	// Initialize quadrants
	size = sizeof(int) * QUADRANT_COUNT;

	memset(sign->quadrants, FALSE, size);

	// Initialize goThroughStopSignLock and goThroughStopSignCondition
	initMutex(&sign->goThroughStopSignLock);
	initConditionVariable(&sign->goThroughStopSignCondition);
}

void destroySafeStopSign(SafeStopSign* sign) {
	destroyStopSign(&sign->base);

	// TODO: Add any logic you need to clean up data structures.

	// Destory enterLaneLocks, exitLaneLocks and exitLaneConditions
	for (int i = 0; i < DIRECTION_COUNT; ++i) {
		pthread_mutex_destroy(&sign->enterLaneLocks[i]);
		pthread_mutex_destroy(&sign->exitLaneLocks[i]);
		pthread_cond_destroy(&sign->exitLaneConditions[i]);
	}

	// Destory goThroughStopSignLock and goThroughStopSignCondition
	pthread_mutex_destroy(&sign->goThroughStopSignLock);
	pthread_cond_destroy(&sign->goThroughStopSignCondition);
}

void runStopSignCar(Car* car, SafeStopSign* sign) {

	// TODO: Add your synchronization logic to this function.

	// Obtain lane and laneIndex for the given car
	EntryLane* lane = getLane(car, &sign->base);
	int laneIndex = getLaneIndex(car);

	// Obtain the required quadrants for the given car
	int quadrants[QUADRANT_COUNT];
	int quadrantCount = getStopSignRequiredQuadrants(car, quadrants);

	// Token of the given car
	int token;

	// ------------- Enter Lane -------------

	pthread_mutex_lock(&sign->enterLaneLocks[laneIndex]);

	// Obtain the token
	token = sign->enterLaneTokens[laneIndex];

	enterLane(car, lane);

	// Increment the token for next car
	++sign->enterLaneTokens[laneIndex];

	unlock(&sign->enterLaneLocks[laneIndex]);

	// -------- Go Through Stop Sign ---------

	pthread_mutex_lock(&sign->goThroughStopSignLock);

	// Are required quadrants all clear?
	int canPass;
	do {
		canPass = TRUE;
		for (int i = 0; i < quadrantCount; ++i) {
			int j = quadrants[i];
			if (sign->quadrants[j] == TRUE) {
				canPass = FALSE;
				break;
			}
		}
		// Put the given car in wait
		// if at least one required quadrant is not available
		if (canPass == FALSE) {
			waitConditionVariable(
				&sign->goThroughStopSignCondition,
				&sign->goThroughStopSignLock
			);
		}
	} while (canPass == FALSE);

	// Mark all required quadrants as occupied
	for (int i = 0; i < quadrantCount; ++i) {
		int j = quadrants[i];
		sign->quadrants[j] = TRUE;
	}

	unlock(&sign->goThroughStopSignLock);

	// -> Start going through
	goThroughStopSign(car, &sign->base);
	// <- Finish going through

	pthread_mutex_lock(&sign->goThroughStopSignLock);

	// Unmark all required quadrants
	for (int i = 0; i < quadrantCount; ++i) {
		int j = quadrants[i];
		sign->quadrants[j] = FALSE;
	}

	// Notify other cars to give a try to go through intersection
	broadcastConditionVariable(&sign->goThroughStopSignCondition);

	unlock(&sign->goThroughStopSignLock);

	// ------------- Exit Lane -------------

	pthread_mutex_lock(&sign->exitLaneLocks[laneIndex]);

	// Ensure the order by comparing tokens
	// Put car in wait
	// if its token not matches
	while (token != sign->exitLaneTokens[laneIndex]) {
		waitConditionVariable(
			&sign->exitLaneConditions[laneIndex],
			&sign->exitLaneLocks[laneIndex]
		);
	}

	exitIntersection(car, lane);

	// Increment token for next car
	++sign->exitLaneTokens[laneIndex];

	// Notify other cars to give a try to exit intersection
	broadcastConditionVariable(&sign->exitLaneConditions[laneIndex]);

	unlock(&sign->exitLaneLocks[laneIndex]);
}

static void broadcastConditionVariable(pthread_cond_t* cond) {
	int returnValue = pthread_cond_broadcast(cond);
	if (returnValue != 0) {
		perror("Condition variable boardcast failed."
				"@ " __FILE__ " : " LINE_STRING "\n");	
	}
}

static void waitConditionVariable(pthread_cond_t* cond, pthread_mutex_t* mutex) {
	int returnValue = pthread_cond_wait(cond, mutex);
	if (returnValue != 0) {
		perror("Condition variable wait failed."
				"@ " __FILE__ " : " LINE_STRING "\n");	
	}
}
