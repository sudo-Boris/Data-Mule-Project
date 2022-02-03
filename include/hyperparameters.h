#ifndef HYPERPARAMETERS_H
#define HYPERPARAMETERS_H
#include "globals.h"

#define DISCOVERY_PERIOD    10  // in seconds
#define MAX_NODES           20
#define DEADLINE            60  // in seconds
#define MSG_NUM             50
// #define DEBUG               
// #define EVAL 
// #define DEBUGGER_MULE
// #define DEBUGGER_MOTE               

/* to implement communication between mule and mule, which is different than mule to mote */
extern bool IS_MULE;        

#endif // !HYPERPARAMETERS_H
