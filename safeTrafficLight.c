/**
* CSC369 Assignment 2
*
* This is the source/implementation file for your safe traffic light 
* submission code.
*/
#include "safeTrafficLight.h"

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

/**
* @brief Callback before sleep
*
* @param userPtr pointer to the user data.
*/
static void beforeSleep(void* userPtr);
/**
* @brief Callback after sleep
*
* @param userPtr pointer to the user data.
*/
static void afterSleep(void* userPtr);

void initSafeTrafficLight(SafeTrafficLight* light, int horizontal, int vertical) {
	initTrafficLight(&light->base, horizontal, vertical);

	// TODO: Add any initialization logic you need.

	// Initialize enterLanelocks, exitLaneLocks and exitLaneConditions
	for (int i = 0; i < TRAFFIC_LIGHT_LANE_COUNT; ++i) {
		initMutex(&light->enterLaneLocks[i]);
		initMutex(&light->exitLaneLocks[i]);
		initConditionVariable(&light->exitLaneConditions[i]);
	}

	// Initialize enterLaneTokens and exitLaneTokens
	size_t size = sizeof(int) * TRAFFIC_LIGHT_LANE_COUNT;

	memset(light->enterLaneTokens, 0, size);
	memset(light->exitLaneTokens, 0, size);

	// Initialize goThroughTrafficLightLock and goThroughTrafficLightCondition
	initMutex(&light->goThroughTrafficLightLock);
	initConditionVariable(&light->goThroughTrafficLightCondition);
}

void destroySafeTrafficLight(SafeTrafficLight* light) {
	destroyTrafficLight(&light->base);

	// TODO: Add any logic you need to free data structures

	// Destroy enterLaneLocks, exitLaneLocks and exitLaneConditions
	for (int i = 0; i < TRAFFIC_LIGHT_LANE_COUNT; ++i) {
		pthread_mutex_destroy(&light->enterLaneLocks[i]);
		pthread_mutex_destroy(&light->exitLaneLocks[i]);
		pthread_cond_destroy(&light->exitLaneConditions[i]);
	}

	// Destroy goThroughTrafficLightLock and goThroughTrafficLightCondition
	pthread_mutex_destroy(&light->goThroughTrafficLightLock);
	pthread_cond_destroy(&light->goThroughTrafficLightCondition);
}

void runTrafficLightCar(Car* car, SafeTrafficLight* light) {

	// TODO: Add your synchronization logic to this function.

	// Obtain lane and laneIndex for the given car
	EntryLane* lane = getLaneLight(car, &light->base);
	int laneIndex = getLaneIndexLight(car);

	// Obtain position and its opposite for the given car
	CarPosition position = car->position;
	CarPosition oppositePosition = getOppositePosition(position);

	// Token of the given car
	int token;

	// ------------- Enter Lane -------------

	pthread_mutex_lock(&light->enterLaneLocks[laneIndex]);

	// Obtain the token
	token = light->enterLaneTokens[laneIndex];

	enterLane(car, lane);

	// Increment the token for next car
	++light->enterLaneTokens[laneIndex];

	unlock(&light->enterLaneLocks[laneIndex]);

	// Enter and act are separate calls because a car turning left can first
	// enter the intersection before it needs to check for oncoming traffic.

	// -------- Enter Traffic Light --------

	pthread_mutex_lock(&light->goThroughTrafficLightLock);

	// Is the traffic light suitable for the given car passing
	// through the intersection?
	int canPass;
	do {
		canPass = TRUE;
		LightState lightState = getLightState(&light->base);

		if (lightState == NORTH_SOUTH) {
			if (position != NORTH && position != SOUTH) {
				canPass = FALSE;
			}
		} else if (lightState == EAST_WEST) {
			if (position != EAST && position != WEST) {
				canPass = FALSE;
			}
		} else {
			canPass = FALSE;
		}
		// Put the given car in wait
		// if the traffic light does not permit
		if (canPass == FALSE) {
			waitConditionVariable(
				&light->goThroughTrafficLightCondition,
				&light->goThroughTrafficLightLock
			);
		}
	} while (canPass == FALSE);

	enterTrafficLight(car, &light->base);

	// --------- Act Traffic Light ---------

	// Check oncoming car in opposite direction
	// if the given car need make a left turn
	if (car->action == LEFT_TURN) {
		int straightCount;
		do {
			straightCount = getStraightCount(&light->base, oppositePosition);
			// Put the given car in wait
			// if there are cars going staight from opposite direction
			if (straightCount > 0) {
				waitConditionVariable(
					&light->goThroughTrafficLightCondition,
					&light->goThroughTrafficLightLock
				);
			}
		} while (straightCount > 0);
	}

	// -> beforeSleep: Unlock and notify other cars to give a try
	// to go through traffic light
	// <- afterSleep: Relock
	actTrafficLight(car, &light->base, beforeSleep, afterSleep, light);

	// Notify other cars to given a try to go through traffic light
	broadcastConditionVariable(&light->goThroughTrafficLightCondition);

	unlock(&light->goThroughTrafficLightLock);

	// ------------- Exit Lane -------------

	pthread_mutex_lock(&light->exitLaneLocks[laneIndex]);

	// Ensure the order by comparing tokens
	// Put car in wait
	// if its token not matches
	while (token != light->exitLaneTokens[laneIndex]) {
		waitConditionVariable(
			&light->exitLaneConditions[laneIndex],
			&light->exitLaneLocks[laneIndex]
		);
	}

	exitIntersection(car, lane);

	// Increment token for next car
	++light->exitLaneTokens[laneIndex];

	// Notify other cars to give a try to exit intersection
	broadcastConditionVariable(&light->exitLaneConditions[laneIndex]);

	unlock(&light->exitLaneLocks[laneIndex]);
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

static void beforeSleep(void* userPtr) {
	SafeTrafficLight* light = userPtr;

	// Unlock and notify other cars to give a try
	// to go through traffic light
	broadcastConditionVariable(&light->goThroughTrafficLightCondition);

	unlock(&light->goThroughTrafficLightLock);
}

static void afterSleep(void* userPtr) {
	SafeTrafficLight* light = userPtr;

	pthread_mutex_lock(&light->goThroughTrafficLightLock);
}
