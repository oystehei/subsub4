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

// max function
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

/* Counters */
static int stagnation_counter = 0;
static int stagnation_check = FALSE;
static int stagnation = FALSE;
static int positive_feedback = 0;

/* Stagnation tables */
double prev_dist_values[8];

// entry point of the controller
int main(int argc, char **argv)
{
  // initialize the Webots API
  wb_robot_init();
  // internal variables
  int i;
  WbDeviceTag ps[8];
  char ps_names[8][4] = {
    "ps0", "ps1", "ps2", "ps3",
    "ps4", "ps5", "ps6", "ps7"
  };
  
  // initialize devices
  for (i=0; i<8 ; i++) {
    ps[i] = wb_robot_get_device(ps_names[i]);
    wb_distance_sensor_enable(ps[i], TIME_STEP);
  }
  
  WbDeviceTag ls[8];
  char ls_names[8][4] = {
    "ls0", "ls1", "ls2", "ls3",
    "ls4", "ls5", "ls6", "ls7"
  };
  
  // initialize devices
  for (i=0; i<8 ; i++) {
    ls[i] = wb_robot_get_device(ls_names[i]);
    wb_light_sensor_enable(ls[i], TIME_STEP);
  }
  
  WbDeviceTag led[8];
  char led_names[8][5] = {
    "led0", "led1", "led2", "led3",
    "led4", "led5", "led6", "led7"
  };
  
  // initialize devices
  for (i=0; i<8 ; i++) {
    led[i] = wb_robot_get_device(led_names[i]);
  }
  
  // feedback loop
  while (1) { 
    // step simulation
    int delay = wb_robot_step(TIME_STEP);
    if (delay == -1) // exit event from webots
      break;
 
    // read sensors outputs
    double ps_values[8];
    for (i=0; i<8 ; i++)
      ps_values[i] = wb_distance_sensor_get_value(ps[i]);
    
    update_search_speed(ps_values, 250);
    
    // set speeds
    double left_speed  = get_search_left_wheel_speed();
    double right_speed = get_search_right_wheel_speed();
    
    
    // read IR sensors outputs
    double ls_values[8];
    for (i=0; i<8 ; i++){
        ls_values[i] = wb_light_sensor_get_value(ls[i]);
      }
    
    int active_ir = FALSE;
    for(i=0; i<8; i++){
      if(ls_values[i] < 2300){
        active_ir = TRUE;
      }
    }
    
    if(active_ir == TRUE){
      swarm_retrieval(ls_values, ps_values, 2300);
      left_speed = get_retrieval_left_wheel_speed();
      right_speed = get_retrieval_right_wheel_speed();
    }
    
    if(is_pushing() == TRUE || stagnation == TRUE){
    // check for stagnation
	stagnation_counter = stagnation_counter + 1;
	if(stagnation_counter == min((50 + positive_feedback * 50), 300) && stagnation == FALSE){
		stagnation_counter = 0; // reset counter
		stagnation_check = TRUE;
		for(i=0; i<8; i++)
                      prev_dist_values[i] = ps_values[i];
	}
	
	if(stagnation_check == TRUE){
            left_speed = 0;
            right_speed = 0;	
	}

	if(stagnation_check == TRUE && stagnation_counter == 5){
		stagnation_counter = 0; // reset counter
		reset_stagnation();
		valuate_pushing(ps_values, prev_dist_values);
		stagnation = get_stagnation_state();
		stagnation_check = FALSE;
		if(stagnation == TRUE)
                      positive_feedback = 0;
                   else
                      positive_feedback = positive_feedback + 1;
	}

	if(stagnation == TRUE){
		stagnation_recovery(ps_values, 300);
		left_speed = get_stagnation_left_wheel_speed();
		right_speed = get_stagnation_right_wheel_speed();
		if(get_stagnation_state() == FALSE){
			reset_stagnation();
			stagnation = FALSE;
			stagnation_counter = 0;
		}
	}
    }

    // write actuators inputs
    wb_differential_wheels_set_speed(left_speed, right_speed);
    
    for(i=0; i<8; i++){
      wb_led_set(led[i], get_LED_state(i));
    }
    
  }
  
  // cleanup the Webots API
  wb_robot_cleanup();
  return 0; //EXIT_SUCCESS
}