#pragma once
/**
* CSC369 Assignment 2
*
* This is the header file for your safe stop sign submission code.
*/
#include "car.h"
#include "stopSign.h"

/**
* @brief Structure that you can modify as part of your solution to implement
* proper synchronization for the stop sign intersection.
*
* This is basically a wrapper around StopSign, since you are not allowed to 
* modify or directly access members of StopSign.
*/
typedef struct _SafeStopSign {

	/**
	* @brief The underlying stop sign.
	*
	* You are not allowed to modify the underlying stop sign or directly
	* access its members. All interactions must be done through the functions
	* you have been provided.
	*/
	StopSign base;

	// TODO: Add any members you need for synchronization here.

	/**
	* @brief The locks for entering lanes
	*/
	pthread_mutex_t enterLaneLocks[DIRECTION_COUNT];
	/**
	* @brief The tokens for entering lanes
	*/
	int enterLaneTokens[DIRECTION_COUNT];

	/**
	* @brief The locks for exiting lanes
	*/
	pthread_mutex_t exitLaneLocks[DIRECTION_COUNT];
	/**
	* @brief The conditions for exiting lanes
	*/
	pthread_cond_t exitLaneConditions[DIRECTION_COUNT];
	/**
	* @brief The tokens for exiting lanes
	*/
	int exitLaneTokens[DIRECTION_COUNT];

	/**
	* @brief The available indicators for quadrants
	*/
	int quadrants[QUADRANT_COUNT];

	/**
	* @brief The locks for going through stop sign
	*/
	pthread_mutex_t goThroughStopSignLock;
	/**
	* @brief The conditions for going through stop sign
	*/
	pthread_cond_t goThroughStopSignCondition;

} SafeStopSign;

/**
* @brief Initializes the safe stop sign.
*
* @param sign pointer to the instance of SafeStopSign to be initialized.
* @param carCount number of cars in the simulation.
*/
void initSafeStopSign(SafeStopSign* sign, int carCount);

/**
* @brief Destroys the safe stop sign.
*
* @param sign pointer to the instance of SafeStopSign to be freed
*/
void destroySafeStopSign(SafeStopSign* sign);

/**
* @brief Runs a car-thread in a stop-sign scenario.
*
* @param car pointer to the car.
* @param sign pointer to the stop sign intersection.
*/
void runStopSignCar(Car* car, SafeStopSign* sign);
