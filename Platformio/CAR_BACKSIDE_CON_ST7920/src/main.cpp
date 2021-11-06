#include <Arduino.h>
#include <U8g2lib.h>
#include "GyverButton.h"
#include "GyverTimer.h"
#include <OneWire.h>
#include <DallasTemperature.h>

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

//--------- one wire ---------------------------------------------------------------
#define TEMPERATURE_PRECISION 9                                  // точность измерения температуры 9 бит
OneWire oneWire(ONE_WIRE_PIN);                                   // порт шины 1WIRE
DallasTemperature temp_sensors(&oneWire);                        // привязка библиотеки DallasTemperature к библиотеке OneWire
DeviceAddress thermometerID_1, thermometerID_2, thermometerID_3; // резервируем адреса для трёх датчиков


//-------- Butttons --------------------------
GButton buttonUp(BUTTON_UP);
GButton buttonDown(BUTTON_DOWN);
GButton buttonEnter(BUTTON_ENTER_ESC);

//-------- Timers ----------------------------
GTimer timerBlink(MS);

//------ Functions -------------------------------------------------------------------------
void fnPrintSelectionFrame(uint8_t menu_pointer);
void fnPrintMenuItemName(uint8_t num, uint8_t num_line, const char* const* names);
void printMenuSetpoints(void);
void fnPrintMainView(void);
void fnPrintMenuParamView(void);
void fnPrintMenuParamItemVal(uint8_t num_item, uint8_t num_line);
void fnPrintMenu1WireScanner(void);


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

  menu_mode = MENU_MAIN_VIEW;

  timerBlink.setInterval(500); 

  fnPrintMenu1WireScanner();

}

void loop() {
  
  delay(10);

  buttonUp.tick();
  buttonDown.tick();
  buttonEnter.tick();

  //меню----------------------------------------------------------------------
    //определение текущей страницы меню
    if(menu_current_item < display_num_lines) menu_current_page = 0;  
    else if(menu_current_item < display_num_lines*2)menu_current_page = 1 ;
    else if(menu_current_item < display_num_lines*3)menu_current_page = 2 ;
    else if(menu_current_item < display_num_lines*4)menu_current_page = 3 ;
    else if(menu_current_item < display_num_lines*5)menu_current_page = 4 ;
    else if(menu_current_item < display_num_lines*6)menu_current_page = 5 ;
    else if(menu_current_item < display_num_lines*7)menu_current_page = 6 ;

    switch (menu_mode)
    {
      case MENU_MAIN_VIEW:

        fnPrintMainView();

        if(buttonUp.isClick()){
          menu_mode = MENU_SETPOINTS;
          menu_current_item = 0;
        }
        if(buttonDown.isClick()){
          menu_mode = MENU_PARAM_VIEW;
          menu_current_item = 0;
        }
        break;

      case MENU_PARAM_VIEW:

        fnPrintMenuParamView();

        if (buttonUp.isClick() or buttonUp.isHold()) {         // Если кнопку нажали или удерживают
          menu_current_item = constrain(menu_current_item + display_num_lines , 0, MENU_PARAM_VIEW_NUM_ITEMS - 1); // Двигаем указатель в пределах дисплея
          Serial.println(menu_current_item);
        }

        if (buttonDown.isClick() or buttonDown.isHold()) {   
          menu_current_item = constrain(menu_current_item - display_num_lines, 0, MENU_PARAM_VIEW_NUM_ITEMS - 1); 
          Serial.println(menu_current_item);
        }

        if(buttonEnter.isHold()){
          menu_mode = MENU_MAIN_VIEW;
          menu_current_item = 0;
        }

        break;

      case MENU_SETPOINTS:

        printMenuSetpoints();

        if (buttonUp.isClick() or buttonUp.isHold()) {         // Если кнопку нажали или удерживают
          menu_current_item = constrain(menu_current_item + 1, 0, MENU_SETPOINTS_NUM_ITEMS - 1); // Двигаем указатель в пределах дисплея
          Serial.println(menu_current_item);
        }

        if (buttonDown.isClick() or buttonDown.isHold()) {   
          menu_current_item = constrain(menu_current_item - 1, 0, MENU_SETPOINTS_NUM_ITEMS - 1); 
          Serial.println(menu_current_item);
        }

        if(buttonEnter.isClick())menu_mode = MENU_SETPOINTS_EDIT_MODE;
        if(buttonEnter.isHold()){
          menu_mode = MENU_MAIN_VIEW;
          menu_current_item = 0;
        }

        break;

      case MENU_SETPOINTS_EDIT_MODE:

        printMenuSetpoints();

        if (buttonUp.isClick() or buttonUp.isHold()){
          SetpointsUnion.SetpointsArray[menu_current_item] = constrain(SetpointsUnion.SetpointsArray[menu_current_item]-1,param_range_min[menu_current_item],param_range_max[menu_current_item]);
          Serial.println(SetpointsUnion.SetpointsArray[menu_current_item]);
        }

        if (buttonDown.isClick() or buttonDown.isHold()){
          SetpointsUnion.SetpointsArray[menu_current_item] = constrain(SetpointsUnion.SetpointsArray[menu_current_item]+1,param_range_min[menu_current_item],param_range_max[menu_current_item]);
          Serial.println(SetpointsUnion.SetpointsArray[menu_current_item]);
        }

        if(buttonEnter.isClick())menu_mode = MENU_SETPOINTS;

        break;
      

      default:
      
      break;

    }
  //конец меню




}

//********************************************************************************
// 
void fnPrintSelectionFrame(uint8_t item_pointer) {

  uint8_t n = 0;

  if(item_pointer < display_num_lines)n = item_pointer;
  else n = item_pointer % display_num_lines;

  if(menu_mode == MENU_SETPOINTS_EDIT_MODE){
    if(timerBlink.isReady())flag_blink = !flag_blink;
    if(flag_blink)u8g2.drawRFrame(0, n*(LCD_FONT_HIGHT + LCD_LINE_SPACER), display_width-2, (LCD_FONT_HIGHT + LCD_LINE_SPACER), 2);
    else{
      u8g2.setDrawColor(0);
      u8g2.drawRFrame(0, n*(LCD_FONT_HIGHT + LCD_LINE_SPACER), display_width-1, (LCD_FONT_HIGHT + LCD_LINE_SPACER), 2);
      u8g2.setDrawColor(1);
    }
  }
  else{
    u8g2.drawRFrame(0, n*(LCD_FONT_HIGHT + LCD_LINE_SPACER), display_width-2, (LCD_FONT_HIGHT + LCD_LINE_SPACER), 2);
  }
  
}


// -----------Функция печати имени пункта меню из prm (общая для всех меню) --------------
void fnPrintMenuItemName(uint8_t _num_item, uint8_t _num_line, const char* const* _names) {
  
  char buffer[32] = {0,};                            // Буфер на полную строку
  
  uint16_t ptr = 0;
  ptr = pgm_read_word(&(_names[_num_item]));         // Получаем указатель на первый символ строки
  uint8_t i = 0;                                     // Переменная - счетчик

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


//--------- Функция печати значения пункта меню уставок ---------------------------------
void fnPrintMenuSetpointsItemVal(uint8_t num_item, uint8_t num_line){

  //если все параметры одного типа то можно выводить через массив
  //sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
  //u8g2.drawStr(98,(num_line*12)-2,buffer);

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
  
  u8g2.drawStr(107,(num_line*12)-2,buffer);
  
}


//--------- Функция вывода меню уставок ------------------------------------------------
void printMenuSetpoints(void){

  u8g2.clearBuffer();					// 
  u8g2.setFont(u8g2_font_ncenB08_tr);	// 

  for (uint8_t i = 0; i <= display_num_lines; i++) {   // Цикл, выводящий пункты на дисплей  
    
    fnPrintMenuItemName(i+(menu_current_page*display_num_lines)-1, i, setpoints_menu_names); // Выводим название пункта 
    fnPrintMenuSetpointsItemVal(i+(menu_current_page*display_num_lines)-1, i); // Выводим значение пункта меню уставок   
  }  

  //рисуем рамку
  fnPrintSelectionFrame(constrain(menu_current_item, 0, MENU_SETPOINTS_NUM_ITEMS));

  //рисуем боковой скролл бар 
  uint8_t scroll_bar_height = display_height/(MENU_SETPOINTS_NUM_ITEMS/display_num_lines);
  u8g2.drawVLine(127, menu_current_page*scroll_bar_height, scroll_bar_height);
      
  u8g2.sendBuffer();	
}


//-------- Функция вывода главного экрана -----------------------------------------------
void fnPrintMainView(void){

  char buffer[20] = {0,};
  uint8_t float_m, float_n; // переменные для разбития числа на целую и дробную часть

  u8g2.clearBuffer();					// 
  

  u8g2.setFont(u8g2_font_5x7_tr);

  main_data.battery_voltage = 12.5;
  float_m = main_data.battery_voltage * 10;
  float_n = float_m%10;
  float_m = float_m/10;
  sprintf(buffer,"%d.%dv",float_m, float_n);
  u8g2.drawStr(102, 8, buffer);

  main_data.inside_temperature = 24.5;
  float_m = main_data.inside_temperature * 10;
  float_n = float_m%10;
  float_m = float_m/10;
  sprintf(buffer,">%d.%dC",float_m, float_n);
  u8g2.drawStr(98, 18, buffer);

  main_data.outside_temperature = 28.7;
  float_m = main_data.outside_temperature * 10;
  float_n = float_m%10;
  float_m = float_m/10;
  sprintf(buffer,"<%d.%dC",float_m, float_n);
  u8g2.drawStr(98, 28, buffer);

  u8g2.drawBox(64,1,21,8);
  u8g2.setDrawColor(0);
  u8g2.drawStr(65, 8, "PUMP");
  u8g2.setDrawColor(1);

  u8g2.drawBox(64,11,21,8);
  u8g2.setDrawColor(0);
  u8g2.drawStr(65, 18, "CONV");
  u8g2.setDrawColor(1);

  u8g2.drawBox(64,21,21,8);
  u8g2.setDrawColor(0);
  u8g2.drawStr(65, 28, "FRDG");
  u8g2.setDrawColor(1);

  u8g2.drawBox(1,1,16,8);
  u8g2.setDrawColor(0);
  u8g2.drawStr(2, 8, "ERR");
  u8g2.setDrawColor(1);
  

  main_data.water_level_liter = 35;
  sprintf(buffer,"%d L",main_data.water_level_liter);
  u8g2.setFont(u8g2_font_ncenB18_tr);	//
  u8g2.drawStr(50, 55, buffer);


  u8g2.drawXBM(0, 12, 50, 50, water_level_50x50);

  u8g2.sendBuffer();
}

//-------- Функция вывода меню параметров  ---------------------------------------------
void fnPrintMenuParamView(void){

  u8g2.clearBuffer();					// 
  u8g2.setFont(u8g2_font_ncenB08_tr);	// 

  for (uint8_t i = 0; i <= display_num_lines; i++) {   // Цикл, выводящий пункты на дисплей

    fnPrintMenuItemName(i+(menu_current_page*display_num_lines)-1, i, parameters_names); // Выводим название пункта 
    fnPrintMenuParamItemVal(i+(menu_current_page*display_num_lines)-1, i); // Выводим значение пункта меню уставок   
  }

  uint8_t scroll_bar_height = display_height/(MENU_PARAM_VIEW_NUM_ITEMS/display_num_lines);
  u8g2.drawVLine(127, menu_current_page*scroll_bar_height, scroll_bar_height);

  u8g2.sendBuffer();

}


//-------  функция печати значения пункта меню параметров ------------------------------
void fnPrintMenuParamItemVal(uint8_t num_item, uint8_t num_line){

  char buffer[10] = {0,};
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
    
  default:
    break;
  }

  u8g2.drawStr(98,(num_line*12)-2,buffer);
}


//------  Функция печати меню сканнера 1Wire --------------------------------------------
void fnPrintMenu1WireScanner(void){

  char buffer [32] = {0,};

  u8g2.clearBuffer();					// 
  u8g2.setFont(u8g2_font_ncenB08_tr);	// 
  u8g2.drawStr(20,10, "OneWire scanner");
  u8g2.sendBuffer();
  u8g2.clearBuffer();	
  delay(2000);

  while(!flag_ow_scan_to_start){

    buttonUp.tick();
    buttonDown.tick();
    buttonEnter.tick(); 

    if(buttonEnter.isClick()){
      flag_ow_scan_to_start = true;
    }

    if(buttonEnter.isHold()){
      
    }

    if(buttonUp.isClick()){
      
    }

    if(buttonDown.isClick()){
      
    }

  }

  uint8_t address[8];
  String tempString = "";
  String tempString2 = "";

  if (oneWire.search(address))
  {
        do
        {
              temp_sensors_data.num_founded_sensors++;

              switch (temp_sensors_data.num_founded_sensors)
              {
              case 1:
                    for (uint8_t j = 0; j < 8; j++) // заносим адрес первого датчика в массив
                    {
                          temp_sensors_data.sensors_ID_array[INSIDE_SENSOR - 1][j] = address[j];
                          thermometerID_1[j] = address[j];
                          tempString2 = String(address[j], HEX);
                          tempString += tempString2;
                          if (j < 7)
                                tempString += ". ";
                    }
                    tempString.toCharArray(buffer, sizeof(tempString));
                    u8g2.drawStr(20,20, buffer); //   myNex.writeStr("p9t2.txt", tempString); //
                    tempString = "";
                    tempString2 = "";
                    break;

              case 2:
                    for (uint8_t j = 0; j < 8; j++)
                    {
                          temp_sensors_data.sensors_ID_array[OUTSIDE_SENSOR - 1][j] = address[j];
                          thermometerID_2[j] = address[j];
                          tempString2 = String(address[j], HEX);
                          tempString += tempString2;
                          if (j < 7)
                                tempString += ". ";
                    }
                    tempString.toCharArray(buffer, sizeof(tempString));
                    u8g2.drawStr(20,30, buffer); //myNex.writeStr("p9t3.txt", tempString);
                    tempString = "";
                    tempString2 = "";
                    break;
              case 3:
                    for (uint8_t j = 0; j < 8; j++)
                    {
                          temp_sensors_data.sensors_ID_array[FRIDGE_SENSOR - 1][j] = address[j];
                          thermometerID_3[j] = address[j];
                          tempString2 = String(address[j], HEX);
                          tempString += tempString2;
                          if (j < 7)
                                tempString += ". ";
                    }
                    tempString.toCharArray(buffer, sizeof(tempString));
                    u8g2.drawStr(20,40, buffer); //myNex.writeStr("p9t4.txt", tempString);
                    tempString = "";
                    tempString2 = "";
                    break;

              default:
                    break;
              }

              if (temp_sensors_data.num_founded_sensors > 3){
                
                u8g2.drawStr(30,40, "Founded >3 sensors");
                break; // если найдено больше трёх датчиков - выходим из цикла
              }

        } while (oneWire.search(address));
  }

  if(temp_sensors_data.num_founded_sensors == 0)u8g2.drawStr(20,40, "No sensors found");
  
  u8g2.sendBuffer();

  delay(5000);
  


}