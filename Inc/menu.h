#ifndef __MENU_H__
#define __MENU_H__

#include "stm32f4xx_hal.h"

typedef void (*Callback_t)(void);

typedef enum {
	none = 0,
	numeric	= 1,
	bar	= 2,
	selection = 3,
	exit_menu = 4,
} MenuType;

typedef struct menu_item_s{
	const char * label;
	struct menu_item_s * parent;
	struct menu_item_s * child;
	struct menu_item_s * previous;
	struct menu_item_s * next;
	Callback_t Callback;
	uint32_t address;
	uint32_t value;
	MenuType type;
}menu_item;

uint32_t encoder_cnt;


void init_display(void);
void display_menu(GPIO_PinState button_state);
void ILI_Init(void);
void render_menu(menu_item * menu_root, GPIO_PinState button_state);
void set_menu1(void);
void set_menu2(void);
void set_menu3(void);
void set_menu4(void);
void set_menu5(void);
void flash_write(uint32_t address, uint32_t value);
void FLASH_init(void);
void ili9341_drawline(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint16_t color);
void data_init(void);
void compute_rpm(void);
void init_stepperdrive(void);

#define NUTLOCK_OFF_LEFT_Pin GPIO_PIN_3
#define NUTLOCK_OFF_LEFT_GPIO_Port GPIOD
#define NUTLOCK_OFF_RIGHT_Pin GPIO_PIN_6
#define NUTLOCK_OFF_RIGHT_GPIO_Port GPIOD
#define JOG_Pin GPIO_PIN_7
#define JOG_GPIO_Port GPIOD

#define FEED_DIR_GPIO_Pin GPIO_PIN_4
#define FEED_DIR_GPIO_Port GPIOC

menu_item * menu_00;
menu_item * menu_01;
menu_item * menu_ex;

menu_item * menu_0;
menu_item * menu_1;
menu_item * menu_2;
menu_item * menu_3;
menu_item * menu_4;
menu_item * menu_5;
menu_item * menu_00_ex;

menu_item * menu_6;
menu_item * menu_7;
menu_item * menu_8;
menu_item * menu_9;
menu_item * menu_10;
menu_item * menu_11;
menu_item * menu_12;
menu_item * menu_13;
menu_item * menu_14;
menu_item * menu_15;
menu_item * menu_01_ex;


#endif
