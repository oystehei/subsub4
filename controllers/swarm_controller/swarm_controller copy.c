#include <webots/robot.h>
#include <webots/differential_wheels.h>
#include <webots/distance_sensor.h>
#include <webots/light_sensor.h>
#include <webots/led.h>
#include "search.c"
#include "retrieval.c"
#include "stagnation.c"
#include <stdio.h>

// time in [ms] of a simulation step
#define TIME_STEP 32
#define TRUE 1
#define FALSE 0

// define max function
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

/* stagnation helpers */
static int stagnation_iterator = 0;
static int stagnation_check = FALSE;
static int stagnation = FALSE;
static int positive_feedback_counter = 0;
double prev_distances[8];

// entry point of the controller
int main(int argc, char **argv)
{
  // initialize the Webots API
  wb_robot_init();
  
  int i; //iterator varaible
  
  WbDeviceTag ps[8];
  char ps_names[8][4] = {
    "ps0", "ps1", "ps2", "ps3",
    "ps4", "ps5", "ps6", "ps7"
  };
  
  // initialize proximity sensors
  for (i=0; i<8 ; i++) {
    ps[i] = wb_robot_get_device(ps_names[i]);
    wb_distance_sensor_enable(ps[i], TIME_STEP);
  }
  
  WbDeviceTag ls[8];
  char ls_names[8][4] = {
    "ls0", "ls1", "ls2", "ls3",
    "ls4", "ls5", "ls6", "ls7"
  };
  
  // initialize IR sensors
  for (i=0; i<8 ; i++) {
    ls[i] = wb_robot_get_device(ls_names[i]);
    wb_light_sensor_enable(ls[i], TIME_STEP);
  }

  while (1) { 
    // step simulation
    int delay = wb_robot_step(TIME_STEP);
    if (delay == -1) // exit event from webots
      break;
 
    // read proximity sensor outputs
    double ps_values[8];
    for (i=0; i<8 ; i++)
      ps_values[i] = wb_distance_sensor_get_value(ps[i]);
    
    update_search_speed(ps_values, 250);
    
    // set speeds of the different wheels
    double left_speed  = get_search_left_wheel_speed();
    double right_speed = get_search_right_wheel_speed();
    
    
    // read IR sensors outputs
    double ls_values[8];
    for (i=0; i<8 ; i++){
        ls_values[i] = wb_light_sensor_get_value(ls[i]);
      }
    
    // is the food source close?
    int food_detected = FALSE;
    for(i=0; i<8; i++){
      if(ls_values[i] < 2300){
        food_detected = TRUE;
      }
    }
    
    // converge towards the food source
    if(food_detected == TRUE){
      swarm_retrieval(ls_values, ps_values, 2300);
      left_speed = get_retrieval_left_wheel_speed();
      right_speed = get_retrieval_right_wheel_speed();
    }
    
    if(is_pushing() == TRUE || stagnation == TRUE){
    // check for stagnation
	stagnation_iterator = stagnation_iterator + 1;
	if(stagnation_iterator == min((50 + positive_feedback_counter * 50), 300) && stagnation == FALSE {
         // reset iterator
		stagnation_iterator = 0;
		stagnation_check = TRUE;
		for(i=0; i<8; i++)
            prev_distances[i] = ps_values[i];
	}
	
    // stop e-puck
	if(stagnation_check == TRUE){
        left_speed = 0;
        right_speed = 0;	
	}
    
    // has the e-puck stagnated?
	if(stagnation_check == TRUE && stagnation_iterator == 5){
		stagnation_iterator = 0; // reset counter
		reset_stagnation();
		valuate_pushing(ps_values, prev_distances);
		stagnation = get_stagnation_state();
		stagnation_check = FALSE;
		if(stagnation == TRUE)
            positive_feedback_counter = 0;
        else
            positive_feedback_counter = positive_feedback_counter + 1;
	}
       
    // reposition
	if(stagnation == TRUE){
		stagnation_recovery(ps_values, 300);
		left_speed = get_stagnation_left_wheel_speed();
		right_speed = get_stagnation_right_wheel_speed();
		if(get_stagnation_state() == FALSE){
			reset_stagnation();
			stagnation = FALSE;
			stagnation_iterator = 0;
		}
	}
       
    }

    // write actuators inputs
    wb_differential_wheels_set_speed(left_speed, right_speed);
    
  }
  
  // cleanup the Webots API
  wb_robot_cleanup();
  return 0; //EXIT_SUCCESS
}