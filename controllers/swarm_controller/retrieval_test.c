/*
 * retrieval.c - Follow and push behavior.
 *
 *	Made to make the e-puck converge and push the box.
 *  Created on: 17. mars 2011
 *      Author: jannik
 */
#include "search.h"
#include <stdlib.h>
#include <stdio.h>

#define NB_LEDS 8
#define ON 1
#define OFF 0
#define TRUE 1
#define FALSE 0
#define NEUTRAL 3
#define PUSH_THRESHOLD_LIGHT 2300
#define PUSH_THRESHOLD_PROXIMITY 2000
#define REVERSE_LIMIT_R 20
#define FORWARD_LIMIT_R 30
#define TURN_LIMIT_R 10

/* Wheel speed variables */
static double left_wheel_speed;
static double right_wheel_speed;

/* LED variables */
int LED[8];

/* iterator */
int i = 0;

/* Boolean variables */
int converge = FALSE; // Moving towards the box
int push = FALSE; // Standing close to the box

/* check to see if the box is moving towards you */
int converge_check = FALSE;
int converge_check_performed = FALSE;
int converge_check_iterator = 0;
int escape = FALSE;
double prev_ps_values[8];
int reverse_counter_r = 0;
int forward_counter_r = 0;
int turn_counter_r = 0;
int turn_left_r = FALSE;

/******************************
 * Internal functions
*******************************/

static void update_speed(int IR_number)
{
	
	if (IR_number==0)
		left_wheel_speed = left_wheel_speed + 700;
	else if(IR_number == 7)
		right_wheel_speed = right_wheel_speed + 700;
	else if(IR_number == 1)
		left_wheel_speed = left_wheel_speed + 350;
	else if (IR_number == 6)
		right_wheel_speed = right_wheel_speed + 350;
	else if (IR_number == 2)
	{
		left_wheel_speed = left_wheel_speed + 550;
		right_wheel_speed = right_wheel_speed - 300;
	}
    else if (IR_number == 5)
    {
        right_wheel_speed = right_wheel_speed + 550;
        left_wheel_speed = left_wheel_speed - 300;
    }
    else if (IR_number == 3)
        left_wheel_speed = left_wheel_speed + 500;

    else if(IR_number == 4)
        right_wheel_speed = right_wheel_speed + 500;
}
/* The movement for converging to the box */
static void converge_to_box(double IR_sensor_value[8], double ps_sensor_values[8], int IR_threshold)
{
	left_wheel_speed = 0;
	right_wheel_speed = 0;

	if((ps_sensor_values[0] > 100 || ps_sensor_values[7] > 100) && converge_check_performed == FALSE){
		printf("converge check true");
		converge_check = TRUE;
		for(i=0; i<8; i++)
			prev_ps_values[i] = ps_sensor_values[i];
	}
	else{
		for(i=0;i<NB_LEDS;i++){
			if(IR_sensor_value[i] < IR_threshold)
			{
				LED[i] = ON;
				update_speed(i);
			}
			else
				LED[i]=OFF;
		}
	}
}
/* The behavior when pushing the box */
static void push_box(double IR_sensor_value[8], int IR_threshold)
{
	left_wheel_speed = 0;
	right_wheel_speed = 0;

	// Blink for visual pushing feedback
	for(i=0;i<NB_LEDS;i++){
		if(LED[i])
			LED[i] = OFF;
		else
			LED[i]=ON;
		if(IR_sensor_value[i] < IR_threshold)
			update_speed(i);
	}
	if((IR_sensor_value[0]<IR_threshold) && (IR_sensor_value[7]<IR_threshold))
	{
		left_wheel_speed = 1000;
		right_wheel_speed = 1000;
	}
}

/* Selects the behavior push or converge */
static void select_behavior(double IR_sensor_value[8], double ps_sensor_value[8])
{
	push = FALSE;
	converge = TRUE;
	for(i=0;i<NB_LEDS;i++){
		if (IR_sensor_value[i] < PUSH_THRESHOLD_LIGHT && ps_sensor_value[i] > PUSH_THRESHOLD_PROXIMITY){
			push = TRUE;
			break;
		}
	}
}
/******************************
 * External functions
*******************************/

/* Converge, push, and stagnation recovery */
void swarm_retrieval(double IR_sensor_value[8], double ps_sensor_value[8], int IR_threshold)
{
	select_behavior(IR_sensor_value, ps_sensor_value);

	if(converge_check == TRUE){
		printf("converge check %d\n", converge_check_iterator);
		left_wheel_speed = 0;
		right_wheel_speed = 0;
		converge_check_iterator = converge_check_iterator + 1;
		if(converge_check_iterator > 5){
			converge_check = FALSE;
			converge_check_iterator = 0;
			converge_check_performed = TRUE;
			if(ps_sensor_value[0] - prev_ps_values[0] > 300 || ps_sensor_value[7] - prev_ps_values[7] > 300)
				escape = TRUE;
		}
	}
	else if(escape == TRUE){
		printf("escape");
		if(reverse_counter_r < REVERSE_LIMIT_R){
			left_wheel_speed = -1000;
			right_wheel_speed = -1000;
			reverse_counter_r = reverse_counter_r + 1;
		}
		else if(turn_counter_r != TURN_LIMIT_R){
			turn_counter_r = turn_counter_r +1;
			if(turn_left_r == NEUTRAL){
				// Roll a dice, left or right?
				double ran = rand()/((double)(RAND_MAX)+1);
				if (ran > 0.5)
					turn_left_r = FALSE;
				else
					turn_left_r = TRUE;
			}

			if(turn_left_r){ // Turn left
				left_wheel_speed = -300;
				right_wheel_speed = 700;
			}
			else{ // Turn right
				left_wheel_speed = 700;
				right_wheel_speed = -300;
			}
		}
		else if(forward_counter_r < FORWARD_LIMIT_R){
			left_wheel_speed = 1000;
			right_wheel_speed = 1000;
			forward_counter_r = forward_counter_r + 1;
		}
		else{
			escape = FALSE;
			reverse_counter_r = 0;
			forward_counter_r = 0;
			turn_counter_r = 0;
			turn_left_r = NEUTRAL;
		}
	}
	else{
		if(push){
			push_box(IR_sensor_value, IR_threshold);
			converge_check_performed = FALSE;
		}
		else // converge
			converge_to_box(IR_sensor_value, ps_sensor_value, IR_threshold);
	}
}

/* */
double get_retrieval_left_wheel_speed()
{
	return left_wheel_speed;
}

/* */
double get_retrieval_right_wheel_speed()
{
	return right_wheel_speed;
}

/* Returns the state (ON/OFF) of the given LED number */
int get_LED_state(int LED_num)
{
	return LED[LED_num];
}

/* Returns whether the e-puck is currently pushing or not */
int is_pushing()
{
  return push;
}
