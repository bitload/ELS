#include "stepperdrive.h"
#include "menu.h"

#define USE_FLOATING_POINT

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

//
// Current position of the motor, in steps
//
int32_t currentPosition;

//
// Desired position of the motor, in steps
//
int32_t desiredPosition;

//
// current state-machine state
// bit 0 - step signal
// bit 1 - direction signal
//
uint16_t state;

//
// Is the drive enabled?
//
uint8_t enabled = 1;

uint32_t previousSpindlePosition;
int32_t currentPosition;

float feed = (float)STEPS_PER_REVOLUTION / (float)ENCODER_RESOLUTION;
float previousFeed;
int16_t feedDirection = 1;
int16_t previousFeedDirection;
float feedval = 1;
const float pitch_selection_values[26] = {0.05, 0.10, 0.20, 0.25, 0.30, 0.35, 0.40, 0.45, 0.50, 0.60, 0.70, 0.75, 0.80, 1.00, 1.25, 1.50, 1.75, 2.00, 2.50, 3.00, 3.50, 4.00, 4.50, 5.00, 5.50, 6.00};
uint32_t update_counter = 0;
uint32_t spindlePosition = 0;
char lock_nut = 1;
char move_right = 0;

void StepperDrive();
void initHardware(void);

void setDesiredPosition(int32_t steps);
void incrementCurrentPosition(int32_t increment);
void setCurrentPosition(int32_t position);

void setEnabled();

uint8_t isAlarm();

void set_lock_nut(void){
	lock_nut = 1;
	__HAL_TIM_SetCounter(&htim2, spindlePosition);			// left/right rapid move end, start threading process on this position
}

void unset_lock_nut(void){
	lock_nut = 0;
}

void set_move_left(void){
	move_right = 0;
}

void set_move_right(void){
	move_right = 1;
}

void set_feed_direction_right(void){
	feedDirection = -1;
//	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
}

void set_feed_direction_left(void){
	feedDirection = 1;
//	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
}

float get_pitch_selection(uint32_t value){
	return pitch_selection_values[value];
}

void setFeedVal(float feedv)
{
	feedval = feed * (feedv / LEADSCREW_PITCH);
}

void setDesiredPosition(int32_t steps)
{
    desiredPosition = steps;
}

void incrementCurrentPosition(int32_t increment)
{
    currentPosition += increment;
}

void setCurrentPosition(int32_t position)
{
    currentPosition = position;
}

uint32_t get_spindlePosition(void){
	return spindlePosition;
}

int32_t feedRatio(uint32_t count)
{
#ifdef USE_FLOATING_POINT
    return ((float)count) * feedval * feedDirection;
#else // USE_FLOATING_POINT
    return ((long long)count) * feed->numerator / feed->denominator * feedDirection;
#endif // USE_FLOATING_POINT
}

// Timer update interrupt callback function
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

	if(htim->Instance==TIM3)
	{

		spindlePosition = __HAL_TIM_GetCounter(&htim2);

// FAST MOVEMENT
//		if(lock_nut == 1){
//			spindlePosition = __HAL_TIM_GetCounter(&htim2);
//		}
//		else{
//			if((update_counter % 10) == 0){
//				if (move_right == 1){
//					spindlePosition += 1;
//					if (spindlePosition > 0x00FFFFFF){
//						spindlePosition = 0;
//					}
//				}
//				else{
//					spindlePosition -= 1;
//					if (spindlePosition == 0xFFFFFFFF){
//						spindlePosition = 0x00FFFFFF;
//					}
//				}
//			}
//		}

		// calculate the desired stepper position
		int32_t desiredSteps = feedRatio(spindlePosition);
		setDesiredPosition(desiredSteps);

		// compensate for encoder overflow/underflow
		if( spindlePosition < previousSpindlePosition && previousSpindlePosition - spindlePosition > _ENCODER_MAX_COUNT/2 ) {
			incrementCurrentPosition(-1 * feedRatio(_ENCODER_MAX_COUNT));
		}
		if( spindlePosition > previousSpindlePosition && spindlePosition - previousSpindlePosition > _ENCODER_MAX_COUNT/2 ) {
			incrementCurrentPosition(feedRatio(_ENCODER_MAX_COUNT));
		}

		// if the feed or direction changed, reset sync to avoid a big step
		if( feedval != previousFeed || feedDirection != previousFeedDirection) {
			setCurrentPosition(desiredSteps);
		}

		// remember values for next time
		previousSpindlePosition = spindlePosition;
		previousFeedDirection = feedDirection;
		previousFeed = feedval;

		if(enabled) {

			switch(state) {

			case 0:
				// Step = 0; Dir = 0
				if( desiredPosition < currentPosition ) {
					HAL_GPIO_WritePin(GPIOD, STEPPER_STEP_Pin, GPIO_PIN_SET);
					state = 2;
				}
				else if(desiredPosition > currentPosition) {
	//                GPIO_SET_DIRECTION;
					HAL_GPIO_WritePin(GPIOD, STEPPER_DIR_Pin, GPIO_PIN_SET);
					state = 1;
				}
				break;

			case 1:
				// Step = 0; Dir = 1
				if( desiredPosition > currentPosition ) {
	//                GPIO_SET_STEP;
					HAL_GPIO_WritePin(GPIOD, STEPPER_STEP_Pin, GPIO_PIN_SET);
					state = 3;
				}
				else if( desiredPosition < currentPosition ) {
	//                GPIO_CLEAR_DIRECTION;
					HAL_GPIO_WritePin(GPIOD, STEPPER_DIR_Pin, GPIO_PIN_RESET);
					state = 0;
				}
				break;

			case 2:
				// Step = 1; Dir = 0
	//            GPIO_CLEAR_STEP;
				HAL_GPIO_WritePin(GPIOD, STEPPER_STEP_Pin, GPIO_PIN_RESET);
				currentPosition--;
				state = 0;
				break;

			case 3:
				// Step = 1; Dir = 1
	//            GPIO_CLEAR_STEP;
				HAL_GPIO_WritePin(GPIOD, STEPPER_STEP_Pin, GPIO_PIN_RESET);
				currentPosition++;
				state = 1;
				break;
			}

		} else {
			// not enabled; just keep current position in sync
			currentPosition = desiredPosition;
		}
		update_counter++;
	}
	else if (htim->Instance==TIM4){

		compute_rpm();

	}
}

