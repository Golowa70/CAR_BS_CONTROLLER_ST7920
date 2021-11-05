#include <Arduino.h>
#include <U8g2lib.h>
#include "GyverButton.h"
#include "GyverTimer.h"

#include "defines.h"
#include "variables.h"
#include "init_functions.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_ST7920_128X64_F_HW_SPI u8g2(U8G2_R2, /* CS=*/LCD_CS , /* reset=*/ LCD_RESET);

//---------кнопки--------------------------
GButton buttonUp(BUTTON_UP);
GButton buttonDown(BUTTON_DOWN);
GButton buttonEnter(BUTTON_ENTER_ESC);

//--------------
GTimer timerSelectedFrameBlink(MS);

//----------------------------------------------------------------------------------------------------------

void fnPrintSelectionFrame(uint8_t menu_pointer);
void fnPrintMenuItemName(uint8_t num, uint8_t num_line, const char* const* names);
void printMenuSetpoints(void);
void fnPrintMainView(void);
void fnPrintMenuParamView(void);
void fnPrintMenuParamItemVal(uint8_t num_item, uint8_t num_line);

void setup() {

  Serial.begin(115200);
  fnIOInit();
  u8g2.begin();

  display_height = u8g2.getDisplayHeight();
  display_width = u8g2.getDisplayWidth();
  display_num_lines = display_height / (LCD_FONT_HIGHT + LCD_LINE_SPACER);
  Serial.println(display_height);
  Serial.println(display_width);
  Serial.println(display_num_lines);

  //menu_mode = MENU_PARAM_VIEW;

  timerSelectedFrameBlink.setInterval(500); 

}

void loop() {
  
  delay(10);

  buttonUp.tick();
  buttonDown.tick();
  buttonEnter.tick();

  switch (menu_mode)
  {
    case MENU_MAIN_VIEW:

      fnPrintMainView();

      if(buttonUp.isClick()){
        menu_mode = MENU_SETPOINTS;
        menu_pointer = 0;
      }
      if(buttonDown.isClick()){
        menu_mode = MENU_PARAM_VIEW;
        menu_pointer = 0;
      }
      break;

    case MENU_PARAM_VIEW:

      fnPrintMenuParamView();

      if (buttonUp.isClick() or buttonUp.isHold()) {         // Если кнопку нажали или удерживают
        menu_pointer = constrain(menu_pointer + display_num_lines , 0, MENU_PARAM_VIEW_NUM_ITEMS - 1); // Двигаем указатель в пределах дисплея
        Serial.println(menu_pointer);
      }

      if (buttonDown.isClick() or buttonDown.isHold()) {   
        menu_pointer = constrain(menu_pointer - display_num_lines, 0, MENU_PARAM_VIEW_NUM_ITEMS - 1); 
        Serial.println(menu_pointer);
      }

      if(buttonEnter.isHold()){
        menu_mode = MENU_MAIN_VIEW;
        menu_pointer = 0;
      }

      break;

    case MENU_SETPOINTS:

      printMenuSetpoints();

      if (buttonUp.isClick() or buttonUp.isHold()) {         // Если кнопку нажали или удерживают
        menu_pointer = constrain(menu_pointer + 1, 0, MENU_SETPOINTS_NUM_ITEMS - 1); // Двигаем указатель в пределах дисплея
        Serial.println(menu_pointer);
      }

      if (buttonDown.isClick() or buttonDown.isHold()) {   
        menu_pointer = constrain(menu_pointer - 1, 0, MENU_SETPOINTS_NUM_ITEMS - 1); 
        Serial.println(menu_pointer);
      }

      if(buttonEnter.isClick())menu_mode = MENU_SETPOINTS_EDIT_MODE;
      if(buttonEnter.isHold()){
        menu_mode = MENU_MAIN_VIEW;
        menu_pointer = 0;
      }
    break;

    case MENU_SETPOINTS_EDIT_MODE:

      printMenuSetpoints();

      if (buttonUp.isClick() or buttonUp.isHold()){
        //SetpointsUnion.SetpointsArray[menu_pointer] = constrain(SetpointsUnion.SetpointsArray[menu_pointer]-1,0,3);
        SetpointsUnion.SetpointsArray[menu_pointer] = constrain(SetpointsUnion.SetpointsArray[menu_pointer]-1,param_range_min[menu_pointer],param_range_max[menu_pointer]);
        Serial.println(SetpointsUnion.SetpointsArray[menu_pointer]);
      }

      if (buttonDown.isClick() or buttonDown.isHold()){
        //SetpointsUnion.SetpointsArray[menu_pointer] = constrain(SetpointsUnion.SetpointsArray[menu_pointer]+1,0,5);
        SetpointsUnion.SetpointsArray[menu_pointer] = constrain(SetpointsUnion.SetpointsArray[menu_pointer]+1,param_range_min[menu_pointer],param_range_max[menu_pointer]);
        Serial.println(SetpointsUnion.SetpointsArray[menu_pointer]);
      }

      if(buttonEnter.isClick())menu_mode = MENU_SETPOINTS;

    break;
    
    default:
    
    break;

  }

    
  //Serial.println(menu_mode);


}

//********************************************************************************

void fnPrintSelectionFrame(uint8_t item_pointer) {

  uint8_t n = 0;
  static bool flag_blink = false;

  if(item_pointer < display_num_lines)n = item_pointer;
  else n = item_pointer % display_num_lines;

  if(menu_mode == MENU_SETPOINTS_EDIT_MODE){
    if(timerSelectedFrameBlink.isReady())flag_blink = !flag_blink;
    if(flag_blink)u8g2.drawRFrame(0, n*(LCD_FONT_HIGHT + LCD_LINE_SPACER), display_width-1, (LCD_FONT_HIGHT + LCD_LINE_SPACER), 2);
    else{
      u8g2.setDrawColor(0);
      u8g2.drawRFrame(0, n*(LCD_FONT_HIGHT + LCD_LINE_SPACER), display_width-1, (LCD_FONT_HIGHT + LCD_LINE_SPACER), 2);
      u8g2.setDrawColor(1);
    }
  }
  else{
    u8g2.drawRFrame(0, n*(LCD_FONT_HIGHT + LCD_LINE_SPACER), display_width-1, (LCD_FONT_HIGHT + LCD_LINE_SPACER), 2);
  }
  
  // Указатель в виде битмапа (картинки)
 // u8g2.drawBitmap(0, item_pointer * 8, 10, 8, ptr_bmp);
}


// -----------Функция для печати строки имени пункта из prm-----------------
void fnPrintMenuItemName(uint8_t _num_item, uint8_t _num_line, const char* const* _names) {
  
  char buffer[32] = {0,};                                            // Буфер на полную строку
  
  uint16_t ptr = 0;
  ptr = pgm_read_word(&(_names[_num_item]));         // Получаем указатель на первый символ строки
  uint8_t i = 0;                                              // Переменная - счетчик

  do {                                            // Начало цикла
    buffer[i] = (char)(pgm_read_byte(ptr++));     // Прочитать в буфер один символ из PGM и подвинуть указатель на 1
    i++;
  } while (i<ITEM_MAX_CHARS);                  // Если это не конец строки - вернуться в начало цикла
  

  /*
   while (pgm_read_byte(ptr) != NULL) {      // всю строку до нулевого символа
      buffer[i++] = (char)(pgm_read_byte(ptr)); // выводим в монитор или куда нам надо
      ptr++;                                   // следующий символ
    }                                 
 */
  //strcpy_P(buffer, pgm_read_word(&(_names[_num_item]))); 
  u8g2.drawStr(0,(_num_line*12)-2,buffer);          // Вывод готовой строки 
}


//----------- функция печати значения пункта меню уставок --------------------------------------------------
void fnPrintMenuSetpointsItemVal(uint8_t num_item, uint8_t num_line){

  char buffer[10] = {0,};
  switch (num_item)
  {
  case 0:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 1:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 2:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;  

  case 3:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 4:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 5:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 6:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;
  
  case 7:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 8:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 9:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 10:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 11:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 12:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;
  
  case 13:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 14:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 15:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 16:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 17:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 18:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;
  
  case 19:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 20:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 21:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 22:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 23:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 24:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 25:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 26:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 27:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 28:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 29:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 30:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 31:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 32:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 33:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 34:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  

  default:
    break;
  }
  
  //sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);


  u8g2.drawStr(108,(num_line*12)-2,buffer);
  //u8g2.setCursor(108,(num_line*12)-2);
  //u8g2.print(u8x8_u8toa(SetpointsUnion.SetpointsArray[num], 3) );
}


//------------- Функция вывода меню уставок ------------------------------------------------
void printMenuSetpoints(void){

  u8g2.clearBuffer();					// 
  u8g2.setFont(u8g2_font_ncenB08_tr);	// 

  for (uint8_t i = 0; i <= display_num_lines; i++) {   // Цикл, выводящий пункты на дисплей
    uint8_t n = 0;
    
    //определение смещения для вывода нужной страницы
    if(menu_pointer < display_num_lines) n = 0;  
    else if(menu_pointer < display_num_lines*2)n = display_num_lines ;
    else if(menu_pointer < display_num_lines*3)n = display_num_lines*2 ;
    else if(menu_pointer < display_num_lines*4)n = display_num_lines*3 ;
    else if(menu_pointer < display_num_lines*5)n = display_num_lines*4 ;
    else if(menu_pointer < display_num_lines*6)n = display_num_lines*5 ;
    else if(menu_pointer < display_num_lines*7)n = display_num_lines*6 ;
    
    fnPrintMenuItemName(i+n-1, i, setpoints_menu_names); // Выводим название пункта 
    fnPrintMenuSetpointsItemVal(i+n-1, i); // Выводим значение пункта меню уставок   
  }  

  fnPrintSelectionFrame(constrain(menu_pointer, 0, MENU_SETPOINTS_NUM_ITEMS));
    
  u8g2.sendBuffer();	
}


//----------- Функция отрисовки главногоэкрана ------------------------------------
void fnPrintMainView(void){

  char buffer[20] = {0,};
  uint8_t float_m, float_n; // переменные для разбития числа на целую и дробную часть

  float_m = main_data.battery_voltage * 10;
  float_n = float_m%10;
  float_m = float_m/10;
  sprintf(buffer,"%d.%dv",float_m, float_n);

  u8g2.clearBuffer();					// 
  u8g2.setFont(u8g2_font_ncen);	//

  float_m = main_data.battery_voltage * 10;
  float_n = float_m%10;
  float_m = float_m/10;
  sprintf(buffer,"Batery V   %d.%dv",float_m, float_n);
  u8g2.drawStr(5, 20, buffer);
  
  u8g2.sendBuffer();
}

//-------- Функция печати текущих параметров  ------------------------------------------
void fnPrintMenuParamView(void){

  u8g2.clearBuffer();					// 
  u8g2.setFont(u8g2_font_ncenB08_tr);	// 

  for (uint8_t i = 0; i <= display_num_lines; i++) {   // Цикл, выводящий пункты на дисплей
    uint8_t n = 0;
    
    //определение смещения для вывода нужной страницы
    if(menu_pointer < display_num_lines) n = 0;  
    else if(menu_pointer < display_num_lines*2)n = display_num_lines ;
    else if(menu_pointer < display_num_lines*3)n = display_num_lines*2 ;
    else if(menu_pointer < display_num_lines*4)n = display_num_lines*3 ;
    else if(menu_pointer < display_num_lines*5)n = display_num_lines*4 ;
    else if(menu_pointer < display_num_lines*6)n = display_num_lines*5 ;
    else if(menu_pointer < display_num_lines*7)n = display_num_lines*6 ;
    
    fnPrintMenuItemName(i+n-1, i, parameters_names); // Выводим название пункта 
    fnPrintMenuParamItemVal(i+n-1, i); // Выводим значение пункта меню уставок   
  }

  u8g2.sendBuffer();

}


//--------------
void fnPrintMenuParamItemVal(uint8_t num_item, uint8_t num_line){

  char buffer[10] = {0,};

  //sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
  //u8g2.drawStr(108,(num_line*12)-2,buffer);
  uint8_t float_m, float_n; // переменные для разбития числа на целую и дробную часть
  
  switch (num_item)
  {
  case 0:
    float_m = main_data.battery_voltage * 10;
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%dv",float_m, float_n);
    break;

  case 1: 
    sprintf(buffer,"%uL", main_data.water_level_liter);
    break;
  case 2:
    float_m = main_data.outside_temperature * 10;
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%dC",float_m, float_n);
    break;

  case 3:
    float_m = main_data.inside_temperature * 10;
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%dC",float_m, float_n);
    break;

  case 4:
    float_m = main_data.fridge_temperature * 10;
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%dC",float_m, float_n);
    break;

  case 5:
    float_m = main_data.sensors_supply_voltage * 10;
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%dv",float_m, float_n);
    break;

  case 6:
    float_m = main_data.res_sensor_resistance * 10;
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%d",float_m, float_n);
    break;

  case 7:
    sprintf(buffer,"%1u", main_data.door_switch_state);
    
    break;

  case 8:
    sprintf(buffer,"%1u", main_data.proximity_sensor_state);
    break;
  case 9:
    sprintf(buffer,"%1u", main_data.ignition_switch_state);
    break;

  case 10:
    sprintf(buffer,"%1u", main_data.low_washer_water_level);
    break;
  case 11:
    sprintf(buffer,"%1u", main_data.converter_output_state);
    break;
  case 12:
    sprintf(buffer,"%1u", main_data.fridge_output_state);
    break;

  case 13:
    sprintf(buffer,"%1u", main_data.pump_output_state);
    break;
  case 14:
    
    break;
  case 15:
    
    break;

  case 16:
    
    break;
  
  case 17:
    
    break;

  case 18:
    
    break;
  case 19:
    
    break;
    
    break;
  
  default:
    break;
  }

  u8g2.drawStr(102,(num_line*12)-2,buffer);
}


//-------------------