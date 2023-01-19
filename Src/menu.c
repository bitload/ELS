/* Includes ------------------------------------------------------------------*/
#include "menu.h"
#include "ili9341.h"
#include "testimg.h"
#include "testimg2.h"
#include <stdio.h>
#include <stdlib.h>
#include "stepperdrive.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;

#define FLASH_TYPEERASEDATA_BYTE         (0x00U)  // Erase 1 byte
#define FLASH_TYPEERASEDATA_HALFWORD     (0x01U)  // Erase 2 bytes
#define FLASH_TYPEERASEDATA_WORD         (0x02U)  // Erase 4 bytes

#define FLASH_TYPEPROGRAMDATA_BYTE         (0x00U)  // Write 1 byte
#define FLASH_TYPEPROGRAMDATA_HALFWORD     (0x01U)  // Write 2 bytes
#define FLASH_TYPEPROGRAMDATA_WORD         (0x02U)  // Write 4 bytes
#define FLASH_TYPEPROGRAMDATA_FASTBYTE     (0x04U)  // Write 1 byte in Fast mode
#define FLASH_TYPEPROGRAMDATA_FASTHALFWORD (0x08U)  // Write 2 bytes in Fast mode
#define FLASH_TYPEPROGRAMDATA_FASTWORD     (0x10U)  // Write 4 bytes in Fast mode

menu_item * display_menu_head;

uint32_t nr_menuitem_visible = 8;
uint32_t nr_menuitem;
uint8_t menu_page = 0;

menu_item * set_actual_menu;

char buttonDown;
char edit_state = 0;
char previous_edit_state = 0;
char was_pressed = 0;
uint32_t encoder_pos_raw = 0;
uint32_t encoder_pos = 0;
uint32_t menu_encoder_pos = 0;
uint32_t encoder_pos_menu = 0;
const uint32_t storedata;
HAL_StatusTypeDef writestate;
uint8_t max_menuitem = 0;
uint8_t menu_top = 0;
uint8_t menu_bottom = 0;
uint8_t menu_shift = 0;
uint8_t selected_pos = 0;
uint32_t encoder_pos_menu_visible = 0;
uint32_t prev_encoder_pos = 0;
uint32_t value;
uint32_t parent_menu_encoder_state;
uint32_t counter = 0;
float pitch;
uint32_t previous_encoder_value = 0;
uint32_t count;
uint32_t previous_count;
uint32_t rpm = 0;
char previous_fast_btn_state = 3;	// init 3 != 0
char previous_feed_dir_btn_state = 0;
float slow_pitch = 1.0;
float fast_pitch = 5.0;				// pitch for fast movement

FLASH_ProcessTypeDef Flash;

void flash_write(uint32_t address, uint32_t value){
    HAL_FLASH_Unlock();
    FLASH_Erase_Sector(FLASH_SECTOR_4, VOLTAGE_RANGE_3);
//    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR | FLASH_FLAG_PGPERR);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, value);
    HAL_FLASH_Lock();
}

uint32_t readFromEEPROM(uint32_t address)
{
  return (*(__IO uint32_t *)address);
}

void ILI_Init(void){
	ILI9341_Init();
}

void init_display(void){
	ILI9341_FillScreen(ILI9341_BLACK);
}


//void edit_menu_item(GPIO_PinState button_state, menu_item * actual_menu, uint16_t x, uint16_t y, uint16_t counter_pos){
//
//	if (systemState != previousSystemState){
//		__HAL_TIM_SetCounter(&htim1, readFromEEPROM(set_actual_menu->address));
//		previousSystemState = systemState;
//	}
//
//	if ((button_state == 1) & (buttonDown == 0)){
//		edit_state ^= 1;
//	}
//	if(edit_state == 1){
//		char disp_str[20];
//
//		if(set_actual_menu->type == selection){
//			float pitch_selection = get_pitch_selection(encoder_pos);
//			int pitchlefts = (int)pitch_selection;
//			int pitchrights = abs(((int)(pitch_selection * 100)) % 100);
//			snprintf(disp_str, sizeof(disp_str), "%d.%.2d", pitchlefts, pitchrights);
//			ILI9341_WriteString(x, y, disp_str, Font_16x26, ILI9341_RED, ILI9341_WHITE);
//		}
//		else{
//			snprintf(disp_str, sizeof(disp_str), "X%10lu", (unsigned long)encoder_pos);
//			ILI9341_WriteString(x, y, disp_str, Font_16x26, ILI9341_RED, ILI9341_WHITE);
//		}
//	}
//	if((button_state == 1) & (buttonDown == 0) & (edit_state == 0)){
//		if (encoder_pos_raw > 800){
//			encoder_pos_raw = 800;
//		}
//		set_actual_menu->value = encoder_pos_raw;
//
//		menu_item * menus_to_save[] = {menu_0, menu_6};
//		flash_write(menus_to_save);
//		__HAL_TIM_SetCounter(&htim1, counter_pos);
//		systemState = Idle;
//	}
//}

void data_init(void){
	__HAL_TIM_SetCounter(&htim1, readFromEEPROM(0x08010000));
	encoder_pos_raw = __HAL_TIM_GetCounter(&htim1);
	menu_encoder_pos = encoder_pos_raw / 4;
	pitch = get_pitch_selection(menu_encoder_pos);
	setFeedVal(pitch);
}

void render(menu_item * menu_root, GPIO_PinState button_state){
	char disp_str[20];

	snprintf(disp_str, sizeof(disp_str), "%s", "Feed:");
	ILI9341_WriteString(0, 0, disp_str, Font_11x18, ILI9341_WHITE, ILI9341_BLACK);

	if (edit_state == 0){
		if (previous_edit_state != edit_state){
			flash_write(0x08010000, encoder_pos_raw);
			setFeedVal(pitch);
			previous_edit_state = edit_state;
		}
		uint32_t pitchlefts = (uint32_t)pitch;
		uint32_t pitchrights = abs(((uint32_t)(pitch * 100)) % 100);
		ILI9341_WriteNumber_float(88, 0, pitchlefts, pitchrights, 2, 2, angelcode_28x47_7seg, ILI9341_GREEN, ILI9341_BLACK);
	}else{
		if ((previous_edit_state != edit_state)){
			__HAL_TIM_SetCounter(&htim1, readFromEEPROM(0x08010000));
			previous_edit_state = edit_state;
		}
		encoder_pos_raw = __HAL_TIM_GetCounter(&htim1);
		menu_encoder_pos = encoder_pos_raw / 4;

		if (menu_encoder_pos > 50){
			__HAL_TIM_SetCounter(&htim1, 0);
			menu_encoder_pos = 0;
		}

		if (menu_encoder_pos > 25 && menu_encoder_pos <= 50){
			__HAL_TIM_SetCounter(&htim1, 25*4);
			menu_encoder_pos = 25;
		}

		pitch = get_pitch_selection(menu_encoder_pos);
		uint32_t pitchlefts = (uint32_t)pitch;
		uint32_t pitchrights = abs(((uint32_t)(pitch * 100)) % 100);
		ILI9341_WriteNumber_float(88, 0, pitchlefts, pitchrights, 2, 2, angelcode_28x47_7seg, ILI9341_RED, ILI9341_BLACK);
	}

	snprintf(disp_str, sizeof(disp_str), "%s", "Speed:");
	ILI9341_WriteString(0, 50, disp_str, Font_11x18, ILI9341_WHITE, ILI9341_BLACK);

	snprintf(disp_str, sizeof(disp_str), "%s", "Encoder:");
	ILI9341_WriteString(0, 100, disp_str, Font_11x18, ILI9341_WHITE, ILI9341_BLACK);

	ILI9341_WriteNumber(88, 50, rpm, 4, 4, angelcode_28x47_7seg, ILI9341_GREEN, ILI9341_BLACK);

	snprintf(disp_str, sizeof(disp_str), "%10lu", (get_spindlePosition()));
	ILI9341_WriteString(88, 100, disp_str, Font_16x26, ILI9341_BLACK, ILI9341_WHITE);

//	snprintf(disp_str, sizeof(disp_str), "%10lu", (lathe_encoder_pos_raw));
//	ILI9341_WriteString(88, 100, disp_str, Font_16x26, ILI9341_BLACK, ILI9341_WHITE);


	if (!HAL_GPIO_ReadPin(GPIOD, NUTLOCK_OFF_LEFT_Pin) && (previous_fast_btn_state != 1)){
			HAL_GPIO_WritePin(GPIOD, JOG_Pin, GPIO_PIN_SET);
			setFeedVal(slow_pitch);
//			set_move_left();
//			unset_lock_nut();
			snprintf(disp_str, sizeof(disp_str), "%s", "SLOW JOG");
			ILI9341_WriteString8(0, 150, disp_str, FontLiberationMono50, ILI9341_WHITE, ILI9341_BLACK);
			previous_fast_btn_state = 1;
	}

	if (!HAL_GPIO_ReadPin(GPIOD, NUTLOCK_OFF_RIGHT_Pin) && (previous_fast_btn_state != 2)){
			HAL_GPIO_WritePin(GPIOD, JOG_Pin, GPIO_PIN_SET);
			setFeedVal(fast_pitch);
//			set_move_right();
//			unset_lock_nut();
			snprintf(disp_str, sizeof(disp_str), "%s", "FAST JOG");
			ILI9341_WriteString8(0, 150, disp_str, FontLiberationMono50, ILI9341_WHITE, ILI9341_BLACK);
			previous_fast_btn_state = 2;
	}

	if (HAL_GPIO_ReadPin(GPIOD, NUTLOCK_OFF_LEFT_Pin) && HAL_GPIO_ReadPin(GPIOD, NUTLOCK_OFF_RIGHT_Pin) && (previous_fast_btn_state != 0)){
			HAL_GPIO_WritePin(GPIOD, JOG_Pin, GPIO_PIN_RESET);
			setFeedVal(pitch);
//			set_lock_nut();
			snprintf(disp_str, sizeof(disp_str), "%s", "Jog off ");
			ILI9341_WriteString8(0, 150, disp_str, FontLiberationMono50, ILI9341_WHITE, ILI9341_BLACK);
			previous_fast_btn_state = 0;
	}

/*
	if (!HAL_GPIO_ReadPin(GPIOC, FEED_DIR_GPIO_Pin) && (previous_feed_dir_btn_state != 0)){
		set_feed_direction_right();
		snprintf(disp_str, sizeof(disp_str), "%s", "FEED RIGHT");
		ILI9341_WriteString(0, 200, disp_str, Font_11x18, ILI9341_WHITE, ILI9341_RED);
		previous_feed_dir_btn_state = 0;
	}

	if (HAL_GPIO_ReadPin(GPIOC, FEED_DIR_GPIO_Pin) && (previous_feed_dir_btn_state != 1)){
		set_feed_direction_left();
		snprintf(disp_str, sizeof(disp_str), "%s", "FEED LEFT ");
		ILI9341_WriteString(0, 200, disp_str, Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
		previous_feed_dir_btn_state = 1;
	}
*/

//	snprintf(disp_str, sizeof(disp_str), "%s", "Az aki");
//	ILI9341_WriteString8(0, 150, disp_str, FontLiberationMono50, ILI9341_WHITE, ILI9341_BLACK);

	if ((button_state == 1) & (buttonDown == 0)){
		edit_state ^= 1;
	}

	buttonDown = (button_state == 1);
}

void display_menu(GPIO_PinState button_state){

	render(display_menu_head, button_state);


}

void compute_rpm(void){
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
	uint32_t lathe_encoder_pos_raw = __HAL_TIM_GetCounter(&htim2);
	count = (lathe_encoder_pos_raw > previous_encoder_value) ? lathe_encoder_pos_raw - previous_encoder_value : previous_encoder_value - lathe_encoder_pos_raw;
	// deal with over/underflow
	if( count > _ENCODER_MAX_COUNT/2 ) {
		count = _ENCODER_MAX_COUNT - count; // just subtract from max value
	}

	if (count < previous_count + 100){					// avoid big number display switching between rapid move and threading

		rpm = count * 60 * 10 / ENCODER_RESOLUTION;		// 10 = calc rate in Hz
		if (rpm > 9999){								// avoid display extra digits
			rpm = 9999;
		}
	}
	previous_encoder_value = lathe_encoder_pos_raw;
	previous_count = count;

}

void init_stepperdrive(void){
	set_lock_nut();
}

