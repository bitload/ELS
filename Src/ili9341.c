/* vim: set ai et ts=4 sw=4: */
#include "stm32f4xx_hal.h"
#include "ili9341.h"
#include "stm32f4xx_it.h"

extern uint8_t spi4_dma_completed;
HAL_StatusTypeDef errr;
uint16_t test = 0;

static void ILI9341_Select() {
    HAL_GPIO_WritePin(ILI9341_CS_GPIO_Port, ILI9341_CS_Pin, GPIO_PIN_RESET);
}

void ILI9341_Unselect() {
    HAL_GPIO_WritePin(ILI9341_CS_GPIO_Port, ILI9341_CS_Pin, GPIO_PIN_SET);
}

static void ILI9341_Reset() {
    HAL_GPIO_WritePin(ILI9341_RES_GPIO_Port, ILI9341_RES_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(ILI9341_RES_GPIO_Port, ILI9341_RES_Pin, GPIO_PIN_SET);
}

static void ILI9341_WriteCommand(uint8_t cmd) {
    HAL_GPIO_WritePin(ILI9341_DC_GPIO_Port, ILI9341_DC_Pin, GPIO_PIN_RESET);
//    HAL_SPI_Transmit(&ILI9341_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
//	while (!spi4_dma_completed)
//	spi4_dma_completed = 0;
    HAL_SPI_Transmit(&ILI9341_SPI_PORT, &cmd, sizeof(cmd), 10);
}

static void ILI9341_WriteData(uint8_t* buff, size_t buff_size) {
    HAL_GPIO_WritePin(ILI9341_DC_GPIO_Port, ILI9341_DC_Pin, GPIO_PIN_SET);

    // split data in small chunks because HAL can't send more then 64K at once
    while(buff_size > 0) {
        uint16_t chunk_size = buff_size > 32768 ? 32768 : buff_size;

		spi4_dma_completed = 0;
		errr = HAL_SPI_Transmit_DMA(&ILI9341_SPI_PORT, buff, chunk_size);
		while (get_spi4_dma_completed() == 0);

        buff += chunk_size;
        buff_size -= chunk_size;
    }
}

static void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // column address set
    ILI9341_WriteCommand(0x2A); // CASET
    {
        uint8_t data[] = { (x0 >> 8) & 0xFF, x0 & 0xFF, (x1 >> 8) & 0xFF, x1 & 0xFF };
        ILI9341_WriteData(data, sizeof(data));
    }

    // row address set
    ILI9341_WriteCommand(0x2B); // RASET
    {
        uint8_t data[] = { (y0 >> 8) & 0xFF, y0 & 0xFF, (y1 >> 8) & 0xFF, y1 & 0xFF };
        ILI9341_WriteData(data, sizeof(data));
    }

    // write to RAM
    ILI9341_WriteCommand(0x2C); // RAMWR
}

void ILI9341_Init() {
    ILI9341_Select();
    ILI9341_Reset();

    // command list is based on https://github.com/martnak/STM32-ILI9341

    // SOFTWARE RESET
    ILI9341_WriteCommand(0x01);
    HAL_Delay(1000);

    // POWER CONTROL A
    ILI9341_WriteCommand(0xCB);
    {
        uint8_t data[] = { 0x39, 0x2C, 0x00, 0x34, 0x02 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // POWER CONTROL B
    ILI9341_WriteCommand(0xCF);
    {
        uint8_t data[] = { 0x00, 0xC1, 0x30 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // DRIVER TIMING CONTROL A
    ILI9341_WriteCommand(0xE8);
    {
        uint8_t data[] = { 0x85, 0x00, 0x78 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // DRIVER TIMING CONTROL B
    ILI9341_WriteCommand(0xEA);
    {
        uint8_t data[] = { 0x00, 0x00 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // POWER ON SEQUENCE CONTROL
    ILI9341_WriteCommand(0xED);
    {
        uint8_t data[] = { 0x64, 0x03, 0x12, 0x81 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // PUMP RATIO CONTROL
    ILI9341_WriteCommand(0xF7);
    {
        uint8_t data[] = { 0x20 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // POWER CONTROL,VRH[5:0]
    ILI9341_WriteCommand(0xC0);
    {
        uint8_t data[] = { 0x23 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // POWER CONTROL,SAP[2:0];BT[3:0]
    ILI9341_WriteCommand(0xC1);
    {
        uint8_t data[] = { 0x10 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // VCM CONTROL
    ILI9341_WriteCommand(0xC5);
    {
        uint8_t data[] = { 0x3E, 0x28 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // VCM CONTROL 2
    ILI9341_WriteCommand(0xC7);
    {
        uint8_t data[] = { 0x86 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // MEMORY ACCESS CONTROL
    ILI9341_WriteCommand(0x36);
    {
        uint8_t data[] = { 0x48 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // PIXEL FORMAT
    ILI9341_WriteCommand(0x3A);
    {
        uint8_t data[] = { 0x55 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // FRAME RATIO CONTROL, STANDARD RGB COLOR
    ILI9341_WriteCommand(0xB1);
    {
        uint8_t data[] = { 0x00, 0x18 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // DISPLAY FUNCTION CONTROL
    ILI9341_WriteCommand(0xB6);
    {
        uint8_t data[] = { 0x08, 0x82, 0x27 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // 3GAMMA FUNCTION DISABLE
    ILI9341_WriteCommand(0xF2);
    {
        uint8_t data[] = { 0x00 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // GAMMA CURVE SELECTED
    ILI9341_WriteCommand(0x26);
    {
        uint8_t data[] = { 0x01 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // POSITIVE GAMMA CORRECTION
    ILI9341_WriteCommand(0xE0);
    {
        uint8_t data[] = { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                           0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // NEGATIVE GAMMA CORRECTION
    ILI9341_WriteCommand(0xE1);
    {
        uint8_t data[] = { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                           0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F };
        ILI9341_WriteData(data, sizeof(data));
    }

    // EXIT SLEEP
    ILI9341_WriteCommand(0x11);
    HAL_Delay(120);

    // TURN ON DISPLAY
    ILI9341_WriteCommand(0x29);

    // MADCTL
    ILI9341_WriteCommand(0x36);
    {
        uint8_t data[] = { ILI9341_ROTATION };
        ILI9341_WriteData(data, sizeof(data));
    }

    ILI9341_Unselect();
}

//draw line
void ili9341_drawline(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint16_t color)
{
	uint32_t y;
	for (uint32_t x = x1; x < x2; x++){
		y = ((y2 * (x - x1)) + (y1 * (x2 - x))) / (x2 - x1);
		ILI9341_DrawPixel(x, y, color);
	}

}

void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT))
        return;

    ILI9341_Select();

    ILI9341_SetAddressWindow(x, y, x+1, y+1);
    uint8_t data[] = { color >> 8, color & 0xFF };
    ILI9341_WriteData(data, sizeof(data));

    ILI9341_Unselect();
}

static void ILI9341_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) {
    uint32_t i, b, j;

    ILI9341_SetAddressWindow(x, y, x+font.width-1, y+font.height-1);
    uint8_t allData[(font.height * font.width) * 2];
    uint32_t line_nr = 0;
    for(i = 0; i < font.height; i++) {

        b = font.data[(ch - 32) * font.height + i];

        for(j = 0; j < font.width; j++) {
            if((b << j) & 0x8000)  {
            	allData[line_nr++] = (color >> 8);
            	allData[line_nr++] = (color & 0xFF);
            } else {
            	allData[line_nr++] = (bgcolor >> 8);
            	allData[line_nr++] = (bgcolor & 0xFF);
            }
        }
    }
    ILI9341_WriteData(allData, sizeof(allData));
}

void ILI9341_WriteString(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor) {
    ILI9341_Select();

    while(*str) {
        if(x + font.width >= ILI9341_WIDTH) {
            x = 0;
            y += font.height;
            if(y + font.height >= ILI9341_HEIGHT) {
                break;
            }

            if(*str == ' ') {
                // skip spaces in the beginning of the new line
                str++;
                continue;
            }
        }

        ILI9341_WriteChar(x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }

    ILI9341_Unselect();
}

static uint16_t ILI9341_WriteChar8(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) {

    uint32_t b, j, d_idx, h, font_size_byte, tr2l, tr2r, nr_byte_in_line;
    uint8_t char_nr, px_idx, trl, trr;

    nr_byte_in_line = 4; //font.width / 8;
    font_size_byte = font.height * nr_byte_in_line;

	char_nr = ch - 32;
	float trlf, trrf;

	if (char_nr == 0){										// space font
		trlf = ((20.0 / 100.0 ) * (float)font.width);			//70% font width is space width
		trrf = ((20.0 / 100.0 ) * (float)font.width);
		trl = (uint8_t)trlf;
		trr = (uint8_t)trrf;
	}
	else{
		trl = 16;
		for(h = 0; h < font.height; h++) {

			// osszefuzi az elso ket bajtot
			b = (font.data[(font_size_byte * char_nr) + (h * nr_byte_in_line)] << 8) | (font.data[(font_size_byte * char_nr) + (h * nr_byte_in_line) + 1] & 0xff);
			tr2l = 0;

			for(j = 0; j < 16; j++) {
				if((b << j) & 0x8000){
					break;
				} else {
					tr2l++;
				}
			}
			if(trl > tr2l){
				trl = tr2l;
			}
		}

		trr = 16;
		for(h = 0; h < font.height; h++) {

			// osszefuzi az utolso ket bajtot
			b = (font.data[(font_size_byte * char_nr) + (h * nr_byte_in_line) + nr_byte_in_line-2] << 8) | (font.data[(font_size_byte * char_nr) + (h * nr_byte_in_line) + nr_byte_in_line-1] & 0xff);

			tr2r = 0;

			for(j = 0; j < 16; j++) {
				if((b >> j) & 0x0001){
					break;
				} else {
					tr2r++;
				}
			}

			if(trr > tr2r){
				trr = tr2r;
			}
		}

		trl = trl - 1;
		trr = trr - 1;
	}

	uint16_t new_width = font.width-trl-trr;

    ILI9341_SetAddressWindow(x, y, x+font.width-trl-trr-1, y+font.height-1);
    uint8_t allData[(font.height * (font.width-trl-trr)) * 2];
    uint32_t line_nr = 0;

	d_idx = 0;
	px_idx = 0;

	for(h = 0; h < font_size_byte / 2; h++) {
		b = (font.data[(font_size_byte * char_nr) + d_idx] << 8) | (font.data[(font_size_byte * char_nr) + d_idx+1] & 0xff);
		for(j = 0; j < 16; j++) {
			// a ket szelet levagja 16bit-ig
			if ((px_idx > 0 || j >= trl) && (px_idx < 1 || j < 16-trr)){
				if((b << j) & 0x8000)  {
					allData[line_nr++] = (color >> 8);
					allData[line_nr++] = (color & 0xFF);
				} else {
					allData[line_nr++] = (bgcolor >> 8);
					allData[line_nr++] = (bgcolor & 0xFF);
				}
			}
		}
		d_idx += 2;
		px_idx++;
		if(d_idx % nr_byte_in_line == 0){
			px_idx = 0;
		}
	}

    ILI9341_WriteData(allData, sizeof(allData));
    return new_width;
}

void ILI9341_WriteString8(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor) {
    ILI9341_Select();
    uint16_t new_width;

    while(*str) {
        if(x + font.width >= ILI9341_WIDTH) {
            x = 0;
            y += font.height;
            if(y + font.height >= ILI9341_HEIGHT) {
                break;
            }

            if(*str == ' ') {
                // skip spaces in the beginning of the new line
                str++;
                continue;
            }
        }

        new_width = ILI9341_WriteChar8(x, y, *str, font, color, bgcolor);
        x += new_width; //font.width;
        str++;
    }

    ILI9341_Unselect();
}


void ILI9341_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // clipping
    if((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT)) return;
    if((x + w - 1) >= ILI9341_WIDTH) w = ILI9341_WIDTH - x;
    if((y + h - 1) >= ILI9341_HEIGHT) h = ILI9341_HEIGHT - y;

    ILI9341_Select();
    ILI9341_SetAddressWindow(x, y, x+w-1, y+h-1);
    uint16_t dlen = w * h * 2;
    if (dlen > 19200) dlen = 19200;
    uint8_t data[dlen]; // = { color >> 8, color & 0xFF };
    HAL_GPIO_WritePin(ILI9341_DC_GPIO_Port, ILI9341_DC_Pin, GPIO_PIN_SET);
    uint32_t d_index = 0;

    for(y = h; y > 0; y--) {
        for(x = w; x > 0; x--) {
        	data[d_index++] = (color >> 8);
        	data[d_index++] = (color & 0xFF);
            if ((d_index % dlen) == 0){
            	ILI9341_WriteData(data, sizeof(data));
            	d_index = 0;
            }
        }
    }
    ILI9341_WriteData(data, sizeof(data));
    ILI9341_Unselect();
}

void ILI9341_FillScreen(uint16_t color) {
    ILI9341_FillRectangle(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, color);
}

void ILI9341_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data) {
    if((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT)) return;
    if((x + w - 1) >= ILI9341_WIDTH) return;
    if((y + h - 1) >= ILI9341_HEIGHT) return;

    ILI9341_Select();
    ILI9341_SetAddressWindow(x, y, x+w-1, y+h-1);
    ILI9341_WriteData((uint8_t*)data, sizeof(uint16_t)*w*h);
    ILI9341_Unselect();
}

void ILI9341_InvertColors(bool invert) {
    ILI9341_Select();
    ILI9341_WriteCommand(invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
    ILI9341_Unselect();
}

void ILI9341_WriteChar_2(uint16_t x, uint16_t y, uint8_t number, FontDef font, uint16_t color, uint16_t bgcolor) {

	const uint16_t * fdata = &font.data[number * ((font.width / 8) * font.height)];

	ILI9341_Select();
    if(x + font.width >= ILI9341_WIDTH) {
        x = 0;
        y += font.height;
        if(y + font.height >= ILI9341_HEIGHT) {
            return;
        }
    }

    uint32_t  b, j, k;
    uint32_t line_nr = 0;

    ILI9341_SetAddressWindow(x, y, x+font.width-1, y+font.height-1);
    uint8_t allData[(font.width * font.height) * 2];

    uint32_t byte_len = (font.width / 8) * font.height;
	for(j = 0; j < byte_len; j++) {
		b = fdata[j];
		for(k = 0; k < 8; k++) {
			if((b << k) & 0x80)  {
				allData[line_nr++] = (color >> 8);
				allData[line_nr++] = (color & 0xFF);
			} else {
				allData[line_nr++] = (bgcolor >> 8);
				allData[line_nr++] = (bgcolor & 0xFF);
			}
		}
	}

    ILI9341_WriteData(allData, sizeof(allData));
    ILI9341_Unselect();
}

void ILI9341_WriteNumber_float(uint16_t x, uint16_t y, uint32_t number_left, uint32_t number_right, uint8_t nr_digit, uint8_t nr_digit_left, FontDef font, uint16_t color, uint16_t bgcolor){

//	uint8_t nrs[10];
//    uint8_t nrs_count = 0;
    uint16_t start_pos_x = x;
    x = font.width * nr_digit;
    uint32_t number_l = number_left;

    while(number_l) {
        if(x + font.width >= ILI9341_WIDTH) {
            x = 0;
            y += font.height;
            if(y + font.height >= ILI9341_HEIGHT) {
                break;
            }
        }
//        nrs[nrs_count++] = number_l % 10;
        number_l /= 10;
    }

	ILI9341_WriteNumber(start_pos_x, y, number_left, nr_digit, nr_digit, angelcode_28x47_7seg, color, ILI9341_BLACK);
	uint16_t rx = start_pos_x + x;
	if (number_right > 0){
		ILI9341_WriteNumber(rx, y, number_right, 2, 0, angelcode_28x47_7seg, color, ILI9341_BLACK);
	}
	else{
		// nullak kiirasa
		while(nr_digit_left){
			nr_digit_left --;
			x = rx + (nr_digit_left * font.width);
			ILI9341_WriteChar_2(x, y, 0, angelcode_28x47_7seg, color, ILI9341_BLACK);
		}
	}

}

void ILI9341_WriteNumber(uint16_t x, uint16_t y, uint32_t number, uint8_t nr_digit, uint8_t dp_pos, FontDef font, uint16_t color, uint16_t bgcolor) {
    ILI9341_Select();

    uint8_t nrs[10];
    uint8_t nrs_count = 0;
    uint16_t start_pos_x = x;
    uint8_t hide_count;

    x = font.width * nr_digit;
    if(number > 0){
		while(number) {
			if(x + font.width >= ILI9341_WIDTH) {
				x = 0;
				y += font.height;
				if(y + font.height >= ILI9341_HEIGHT) {
					break;
				}
			}
			nrs[nrs_count++] = number % 10;
			number /= 10;
		}

		x = font.width * (nr_digit - nrs_count);
		uint8_t nrs_count_disp = nrs_count;
		while(nrs_count_disp){
			nrs_count_disp--;
			ILI9341_WriteChar_2(start_pos_x+x, y, nrs[nrs_count_disp], font, color, ILI9341_BLACK);
			x += font.width;
		}

		hide_count = nr_digit - nrs_count;
    }
    else{
    	x = start_pos_x + ((nr_digit - 1) * font.width);
    	ILI9341_WriteChar_2(x, y, 0, font, color, ILI9341_BLACK);
    	hide_count = nr_digit - 1;
    }

	if (dp_pos > 0){
		uint8_t dpp = font.width * dp_pos;
		ILI9341_FillRectangle(start_pos_x+dpp-4, y+font.height-2, 4, 4, color);
	}

	while(hide_count){
		uint16_t hide_color;
		if (dp_pos == 0){
			hide_color = color;
		}
		else{
			hide_color = ILI9341_COLOR565(25,58,0);
		}
		hide_count--;
//    	ILI9341_FillRectangle(start_pos_x, y, font.width, font.height, ILI9341_BLACK);
		ILI9341_WriteChar_2(start_pos_x, y, 0, font, hide_color, ILI9341_BLACK);
		ILI9341_FillRectangle(start_pos_x+font.width-4, y+font.height-2, 4, 4, ILI9341_BLACK);
		start_pos_x += font.width;
	}

    ILI9341_Unselect();
}

