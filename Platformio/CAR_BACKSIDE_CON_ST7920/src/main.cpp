#include <Arduino.h>
#include <U8g2lib.h>
#include "GyverButton.h"
#include "GyverTimer.h"
#include <GyverFilters.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <GyverTimers.h>
#include <EEPROMex.h>
#include <EEPROMVar.h>
#include "PJONSoftwareBitBang.h"

PJONSoftwareBitBang bus;


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

uint32_t time;
uint32_t old_time;

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
GTimer timerPumpOffDelay(MS);
GTimer timerPrxSensorFeedbackDelay(MS);
GTimer timerPeriodSensUpdate(MS);
GTimer timerPjonSender(MS);
GTimer timerPjonResponse(MS);
GTimer timerMenuUpdate(MS);

//-------- Filters ---------------------------
GFilterRA ps_voltage_filter;
GFilterRA sens_voltage_filter;
GFilterRA resistive_sensor_filter;

//------ Functions -------------------------------------------------------------------------
void fnPrintSelectionFrame(uint8_t menu_pointer);
void fnPrintMenuItemName(uint8_t num, uint8_t num_line, const char* const* names);
void printMenuSetpoints(void);
void fnPrintMainView(void);
void fnPrintMenuParamView(void);
void fnPrintMenuParamItemVal(uint8_t num_item, uint8_t num_line);
void fnPrintMenu1WireScanner(void);
void fnOneWireScanner(void);
bool fnEEpromSetpointsInit(void);
bool fnEEpromWriteDefaultSetpoints(void);
bool fnEEpromInit(void);
void fnPumpControl(MyData &data, SetpointsStruct &setpoints);
void fnPumpControl_2(MyData &data, SetpointsStruct &setpoints);
uint8_t fnDebounce(uint8_t sample);
void fnInputsUpdate(void);
void fnOutputsUpdate(MyData &data);  // функция обновления выходов
void fnTempSensorsUpdate(void);
void pj_receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info);
void fnPjonSender(void);
void fnWaterLevelControl(MyData &data, PjonReceive &pj_sensor_receive_data, SetpointsStruct &setpoints, Alarms &alarms);


//обработчик прерывания от Timer3 
ISR(TIMER3_A)
{
  buttonUp.tick();
  buttonDown.tick();
  buttonEnter.tick();
}
//**********************************************************************************

void setup() {

  Serial.begin(115200);
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  display_height = u8g2.getDisplayHeight();
  display_width = u8g2.getDisplayWidth();
  display_num_lines = display_height / (LCD_FONT_HIGHT + LCD_LINE_SPACER);
  fnDefaultSetpointsInit();
  fnIOInit();

  if(fnEEpromInit()){
    u8g2.clearBuffer();
    u8g2.drawStr(40,30,"READY!");
    u8g2.sendBuffer();
    delay(1000);
  } 
  else {
    u8g2.clearBuffer();
    u8g2.drawStr(40,30,"ERROR!");
    u8g2.sendBuffer();
    memcpy(&SetpointsUnion.setpoints_data, &default_setpoints_data, sizeof(SetpointsUnion.setpoints_data));//
    delay(1000);    
  }
  
  temp_sensors.begin();
  temp_sensors.setResolution(thermometerID_1, TEMPERATURE_PRECISION);
  temp_sensors.setResolution(thermometerID_2, TEMPERATURE_PRECISION);
  temp_sensors.setResolution(thermometerID_3, TEMPERATURE_PRECISION);
  temp_sensors.setWaitForConversion(false);

  timerPumpOffDelay.setMode(MANUAL);
  timerPrxSensorFeedbackDelay.setMode(MANUAL);
  timerPrxSensorFeedbackDelay.setInterval(PRX_SENSOR_FEEDBACK_DELAY);
  timerBlink.setInterval(500); 
  timerPeriodSensUpdate.setInterval(500);
  timerPjonSender.setInterval(500);
  timerMenuUpdate.setInterval(200);
  timerPjonResponse.setTimeout(PJON_RESPONSE_TIMEOUT);

  Timer3.setPeriod(10000); // Устанавливаем период таймера опроса кнопок
  Timer3.enableISR(CHANNEL_A);

  ps_voltage_filter.setCoef(0.1); // установка коэффициента фильтрации (0.0... 1.0). Чем меньше, тем плавнее фильтр
  ps_voltage_filter.setStep(20);  // установка шага фильтрации (мс). Чем меньше, тем резче фильтр
  sens_voltage_filter.setCoef(0.1);
  sens_voltage_filter.setStep(50);
  resistive_sensor_filter.setCoef(0.02);
  resistive_sensor_filter.setStep(1000);

  if(!digitalRead(BUTTON_DOWN) && !digitalRead(BUTTON_UP)){
    fnOneWireScanner();
  }
  
  menu_mode = MENU_MAIN_VIEW;

  bus.begin();  
  bus.strategy.set_pin(PJON_BUS_PIN); // выбор пина дя передачи данных
  bus.set_id(PJON_MY_ID); //  установка собственного ID   
  bus.set_receiver(pj_receiver_function);

  digitalWrite(SENSORS_SUPPLY_5v, HIGH);
  
}
//**********************************************************************************************


void loop() {
  
  digitalWrite(SENSORS_SUPPLY_5v, HIGH);
  fnInputsUpdate();
  
  main_data.battery_voltage = (analogRead(SUPPLY_VOLTAGE_INPUT) - 127 + SetpointsUnion.setpoints_data.voltage_correction) * DIVISION_RATIO_VOLTAGE_INPUT;
  main_data.sensors_supply_voltage = (analogRead(SENSORS_VOLTAGE_INPUT) * DIVISION_RATIO_SENS_SUPPLY_INPUT);
  main_data.res_sensor_resistance = (uint16_t) (resistive_sensor_filter.filtered(analogRead(RESISTIVE_SENSOR) -127 + SetpointsUnion.setpoints_data.resistive_sensor_correction) * DIVISION_RATIO_RESIST_SENSOR);

  
  //меню----------------------------------------------------------------------
  if(timerMenuUpdate.isReady()){
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
          menu_current_item = constrain(menu_current_item - display_num_lines , 0, MENU_PARAM_VIEW_NUM_ITEMS - 1); // Двигаем указатель в пределах дисплея
          Serial.println(menu_current_item);
        }

        if (buttonDown.isClick() or buttonDown.isHold()) {   
          menu_current_item = constrain(menu_current_item + display_num_lines, 0, MENU_PARAM_VIEW_NUM_ITEMS - 1); 
          Serial.println(menu_current_item);
        }

        if(buttonEnter.isHold()){
          menu_mode = MENU_MAIN_VIEW;
          menu_current_item = 0;
        }

        break;

      case MENU_SETPOINTS:

        printMenuSetpoints();

        if (buttonDown.isClick() or buttonDown.isHold()) {         // Если кнопку нажали или удерживают
          menu_current_item = constrain(menu_current_item + 1, 0, MENU_SETPOINTS_NUM_ITEMS - 1); // Двигаем указатель в пределах дисплея
          Serial.println(menu_current_item);
        }

        if (buttonUp.isClick() or buttonUp.isHold()) {   
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
          SetpointsUnion.SetpointsArray[menu_current_item] = constrain(SetpointsUnion.SetpointsArray[menu_current_item]+1,param_range_min[menu_current_item],param_range_max[menu_current_item]);
          Serial.println(SetpointsUnion.SetpointsArray[menu_current_item]);
        }

        if (buttonDown.isClick() or buttonDown.isHold()){
          SetpointsUnion.SetpointsArray[menu_current_item] = constrain(SetpointsUnion.SetpointsArray[menu_current_item]-1,param_range_min[menu_current_item],param_range_max[menu_current_item]);
          Serial.println(SetpointsUnion.SetpointsArray[menu_current_item]);
        }

        if(buttonEnter.isClick()){
          EEPROM.updateBlock(EEPROM_SETPOINTS_ADDRESS, SetpointsUnion.setpoints_data);
          Serial.println("eeprom updated!");
          menu_mode = MENU_SETPOINTS;
        }

        break;
      

      default:
      
      break;

    }

  }
  //конец меню


  fnPumpControl_2(main_data, SetpointsUnion.setpoints_data);
  //fnPumpControl(main_data, SetpointsUnion.setpoints_data);
  fnTempSensorsUpdate();
  fnWaterLevelControl(main_data, pjon_sensor_receive_data, SetpointsUnion.setpoints_data, present_alarms);

  if(timerPjonSender.isReady())fnPjonSender();
  if(timerPjonResponse.isReady())flag_pjon_water_sensor_connected = false;
  bus.receive(1000);  
  bus.update();

  fnOutputsUpdate(main_data);

}

//************************************************************************************************************
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
  u8g2.setFont(u8g2_font_6x12_tr); 
  u8g2.drawStr(3,(_num_line*12)-2,buffer);          // Вывод готовой строки
  u8g2.setFont(u8g2_font_ncenB08_tr);	// 
  
}


//--------- Функция печати значения пункта меню уставок ---------------------------------
void fnPrintMenuSetpointsItemVal(uint8_t num_item, uint8_t num_line){

  //если все параметры одного типа то можно выводить через массив
  //sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
  //u8g2.drawStr(98,(num_line*12)-2,buffer);

  char buffer[10] = {0,};
  uint8_t float_m, float_n; // переменные для разбития числа на целую и дробную часть

  switch (num_item)
  {
  case 0:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 1:

    switch (SetpointsUnion.SetpointsArray[num_item])
    {
    case OFF_MODE:
      sprintf(buffer, "off");
      break;    
    case ON_MODE:
      sprintf(buffer, "on");
      break;
    case AUTO_MODE:
      sprintf(buffer, "auto");
      break;      
    default:
      break;
    }
    
    break;

  case 2:
    float_m = SetpointsUnion.SetpointsArray[num_item];
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%d",float_m, float_n);
    break;  

  case 3:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 4:
    float_m = SetpointsUnion.SetpointsArray[num_item];
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%d",float_m, float_n);
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
    float_m = SetpointsUnion.SetpointsArray[num_item];
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%d",float_m, float_n);
    break;

  case 9:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 10:
    float_m = SetpointsUnion.SetpointsArray[num_item];
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%d",float_m, float_n);
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
    
    switch (SetpointsUnion.SetpointsArray[num_item])
    {
      case PJON_SENSOR_TYPE:
        sprintf(buffer, "PJn");
        break;

      case RES_SENSOR_TYPE:
        sprintf(buffer, "Res");
        break;
      
      default:
      break;
    }

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
    float_m = SetpointsUnion.SetpointsArray[num_item];
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%d",float_m, float_n);
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
  
  u8g2.drawStr(102,(num_line*12)-2,buffer);
  
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

  u8g2.drawBox(98,1,31,8);
  u8g2.drawBox(98,11,31,8);
  u8g2.drawBox(98,21,31,8);

  u8g2.setDrawColor(0);
  
  float_m = (uint8_t)(main_data.battery_voltage * 10);
  float_n = float_m%10;
  float_m = float_m/10;
  sprintf(buffer,"%d.%dv",float_m, float_n);
  u8g2.drawStr(102, 8, buffer);

  sprintf(buffer,"> %dC", (int)main_data.inside_temperature);
  u8g2.drawStr(98, 18, buffer);

  sprintf(buffer,"< %dC", (int)main_data.outside_temperature);
  u8g2.drawStr(98, 28, buffer);

  u8g2.setDrawColor(1);

  if(main_data.pump_output_state){
    u8g2.drawBox(64,1,21,8);
    u8g2.setDrawColor(0);
    u8g2.drawStr(65, 8, "PUMP");
    u8g2.setDrawColor(1);
  }
  else{
    u8g2.setDrawColor(0);
    u8g2.drawBox(64,1,21,8);
    u8g2.setDrawColor(1);
  }

  if(main_data.converter_output_state){
    u8g2.drawBox(64,11,21,8);
    u8g2.setDrawColor(0);
    u8g2.drawStr(65, 18, "CONV");
    u8g2.setDrawColor(1);
  }
  else{
    u8g2.setDrawColor(0);
    u8g2.drawBox(64,11,21,8);
    u8g2.setDrawColor(1);
  }

  if(main_data.fridge_output_state){
    u8g2.drawBox(64,21,21,8);
    u8g2.setDrawColor(0);
    u8g2.drawStr(65, 28, "FRDG");
    u8g2.setDrawColor(1);
  }
  else{
    u8g2.setDrawColor(0);
    u8g2.drawBox(64,21,21,8);
    u8g2.setDrawColor(1);
  }

  if(present_alarms.common){
    u8g2.drawBox(1,1,16,8);
    u8g2.setDrawColor(0);
    u8g2.drawStr(2, 8, "ERR");
    u8g2.setDrawColor(1);
  }
  else{
    u8g2.setDrawColor(0);
    u8g2.drawBox(1,1,16,8);
    u8g2.setDrawColor(1);
  }

  sprintf(buffer,"%d L",main_data.water_level_liter);
  u8g2.setFont(u8g2_font_ncenB18_tr);	//
  u8g2.drawStr(55, 55, buffer);


  u8g2.drawXBM(5, 12, 50, 50, water_level_50x50);

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
  int float_m, float_n; // переменные для разбития числа на целую и дробную часть
  
  switch (num_item)
  {
  case 0:
    float_m = (int)(main_data.battery_voltage * 10);
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%dv",float_m, float_n);
    break;

  case 1: 
    sprintf(buffer,"%uL", main_data.water_level_liter);
    break;

  case 2:
    float_m = (int)(main_data.outside_temperature * 10);
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%dC",float_m, float_n);
    break;

  case 3:
    float_m = (int)(main_data.inside_temperature * 10);
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%dC",float_m, float_n);
    break;

  case 4:
    float_m = (int)(main_data.fridge_temperature * 10);
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%dC",float_m, float_n);
    break;

  case 5:
    float_m = (int)(main_data.sensors_supply_voltage * 10);
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%dv",float_m, float_n);
    break;

  case 6:
    sprintf(buffer,"%d",main_data.res_sensor_resistance);
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


//-------1Wire scanner ------------------------------------------------------

void fnOneWireScanner(void){

  Serial.println("fnOneWireScanner");

  uint8_t address[8] = {0,};
  uint8_t tmp_thermometerID_1[8] = {0,};
  uint8_t tmp_thermometerID_2[8] = {0,};
  uint8_t tmp_thermometerID_3[8] = {0,};
  char buffer [32] = {0,};
  uint8_t num_founded_sensors = 0;

  enum fsm_state {start, scanner, founded_no_more_three, not_founded, founded_more_three, save, exit};
  enum fsm_state current_state = start;
  bool flag_scanned = false;

  digitalWrite(SENSORS_SUPPLY_5v, HIGH);
  delay(200);
 
  while(!flag_scanned){

    switch (current_state)
    {
    case start:
      u8g2.clearBuffer();					// 
      u8g2.setFont(u8g2_font_ncenB08_tr);	// 
      u8g2.drawStr(20,10, "OneWire scanner");
      u8g2.setFont(u8g2_font_6x10_tr);
      sprintf(buffer,"Saved %d sensors", temp_sensors_data.num_saved_sensors);
      u8g2.drawStr(20,30, buffer);
      u8g2.drawStr(7,50, "OK-> scan");
      u8g2.drawStr(7,60, "DOWN-> exit");
      u8g2.sendBuffer();
      if(buttonEnter.isClick())current_state = scanner;
      if(buttonDown.isClick())current_state = exit;
      break;
    
    case scanner:

      num_founded_sensors = 0;

      if (oneWire.search(address)){
        do
        {
          num_founded_sensors++;

          switch (num_founded_sensors){
            case 1:
              for (uint8_t j = 0; j < 8; j++) // заносим адрес первого датчика в массив
              {
                tmp_thermometerID_1[j] = address[j];
              } 
              break;

            case 2:
              for (uint8_t j = 0; j < 8; j++)
              {
                tmp_thermometerID_2[j] = address[j];                         
              } 
              break;

            case 3:
              for (uint8_t j = 0; j < 8; j++)
              {
                tmp_thermometerID_3[j] = address[j];                                                    
              }

              break;

            default:
              break;

          }

        } while (oneWire.search(address));
      }

      if(num_founded_sensors == 0)current_state = not_founded;
      else if(num_founded_sensors <= 3)current_state = founded_no_more_three;
      else if(num_founded_sensors > 3)current_state = founded_more_three;

      Serial.print("Founded sensors ");
      Serial.println(num_founded_sensors);

      break;

    case founded_no_more_three:

      u8g2.clearBuffer();				
      sprintf(buffer,"Founded %d sensors", num_founded_sensors);
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(20,20, buffer);
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(7,40, "UP-> scan");
      u8g2.drawStr(7,50, "OK-> continue");
      u8g2.drawStr(7,60, "DOWN-> exit");
      u8g2.sendBuffer();      
      if(buttonDown.isClick())current_state = exit;
      if(buttonEnter.isClick())current_state = save;
      if(buttonUp.isClick())current_state = scanner;
      break;
    
    case not_founded:

      u8g2.clearBuffer();		
      u8g2.setFont(u8g2_font_ncenB08_tr);		
      u8g2.drawStr(15,20, "No sensors founded");
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(7,50, "UP-> scan");
      u8g2.drawStr(7,60, "DOWN-> exit");
      u8g2.sendBuffer();
      if(buttonUp.isClick())current_state = scanner;
      if(buttonDown.isClick())current_state = exit;
      break;

    case founded_more_three:
    
      u8g2.clearBuffer();		
      u8g2.setFont(u8g2_font_ncenB08_tr);		
      u8g2.drawStr(30,40, "founded > 3");
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(7,50, "UP-> scan");
      u8g2.drawStr(7,60, "DOWN-> exit");
      u8g2.sendBuffer();
      if(buttonUp.isClick())current_state = scanner;
      if(buttonDown.isClick())current_state = exit;
      break;

    case save:

      u8g2.clearBuffer();	

      u8g2.drawStr(2, 10, "ID1: ");
      u8g2.drawStr(2, 20, "ID2: ");
      u8g2.drawStr(2, 30, "ID3: ");

      sprintf(buffer,"%x%x%x%x%x%x%x%x", tmp_thermometerID_1[0],tmp_thermometerID_1[1],tmp_thermometerID_1[2],tmp_thermometerID_1[3],tmp_thermometerID_1[4],tmp_thermometerID_1[5],tmp_thermometerID_1[6],tmp_thermometerID_1[7]);
      Serial.println(buffer);
      u8g2.drawStr(30,10, buffer); //  

      sprintf(buffer,"%x%x%x%x%x%x%x%x",tmp_thermometerID_2[0],tmp_thermometerID_2[1],tmp_thermometerID_2[2],tmp_thermometerID_2[3],tmp_thermometerID_2[4],tmp_thermometerID_2[5],tmp_thermometerID_2[6],tmp_thermometerID_2[7]);
      Serial.println(buffer);
      u8g2.drawStr(30,20, buffer);

      sprintf(buffer,"%x%x%x%x%x%x%x%x",tmp_thermometerID_3[0],tmp_thermometerID_3[1],tmp_thermometerID_3[2],tmp_thermometerID_3[3],tmp_thermometerID_3[4],tmp_thermometerID_3[5],tmp_thermometerID_3[6],tmp_thermometerID_3[7]);
      Serial.println(buffer);
      u8g2.drawStr(30,30, buffer); 			
      
      u8g2.drawStr(7,40, "UP-> scan");
      u8g2.drawStr(7,50, "OK-> save");
      u8g2.drawStr(7,60, "DOWN-> exit");
      u8g2.sendBuffer();

      if(buttonEnter.isClick()){

        for(uint8_t i=0;i<8;i++){

          thermometerID_1[i] = tmp_thermometerID_1[i];
          thermometerID_2[i] = tmp_thermometerID_2[i];
          thermometerID_3[i] = tmp_thermometerID_3[i];

          temp_sensors_data.sensors_ID_array[INSIDE_SENSOR - 1][i] = tmp_thermometerID_1[i];
          temp_sensors_data.sensors_ID_array[OUTSIDE_SENSOR - 1][i] = tmp_thermometerID_2[i];
          temp_sensors_data.sensors_ID_array[FRIDGE_SENSOR - 1][i] = tmp_thermometerID_3[i];

        }

        temp_sensors_data.num_saved_sensors = num_founded_sensors;

        //update EEPROM
        EEPROM.updateBlock(EEPROM_1WIRE_ADDRESS, temp_sensors_data);

        //копирование адресов датчиков из структуры уставок которая сохранена в EEPROM
        memcpy(&thermometerID_1, &temp_sensors_data.sensors_ID_array[SetpointsUnion.setpoints_data.inside_sensor_id-1][0], sizeof(thermometerID_1));  //
        memcpy(&thermometerID_2, &temp_sensors_data.sensors_ID_array[SetpointsUnion.setpoints_data.outside_sensor_id-1][0], sizeof(thermometerID_2)); //
        memcpy(&thermometerID_3, &temp_sensors_data.sensors_ID_array[SetpointsUnion.setpoints_data.fridge_sensor_id-1][0], sizeof(thermometerID_3));   //

        
        u8g2.clearBuffer();	
        u8g2.setFont(u8g2_font_ncenB08_tr);			
        u8g2.drawStr(40,20, "SAVED!");
        u8g2.sendBuffer();
        delay(2000);    
        current_state = exit;
      }

      if(buttonDown.isClick())current_state = exit;
      if(buttonUp.isClick())current_state = scanner;
      break;

    case exit:
      flag_scanned = true;
      digitalWrite(SENSORS_SUPPLY_5v, LOW);
      break;

    default:
      break;
    }
  }

}

//----------------------------------------------------------------

//EEPROM Init
bool fnEEpromSetpointsCheck(void){

  EEPROM.readBlock(EEPROM_SETPOINTS_ADDRESS, SetpointsUnion.setpoints_data); // считываем уставки из eeprom
  if (SetpointsUnion.setpoints_data.key == EEPROM_KEY) // если ключ совпадает значит не первый запуск
  {      
    Serial.println(EEPROM_KEY);            
    return true; // возвращаем один
  }
  else // если ключ  не совпадает значит первый запуск
  {
    Serial.println(EEPROM_KEY); 
    return false;
  }

}
//*******************************************************************************

//------ 
bool fnEEpromWriteDefaultSetpoints(void){

  EEPROM.writeBlock(EEPROM_SETPOINTS_ADDRESS, default_setpoints_data); // записываем
  delay(100);
  EEPROM.readBlock(EEPROM_SETPOINTS_ADDRESS, SetpointsUnion.setpoints_data);  // считываем 

  if (SetpointsUnion.setpoints_data.key == EEPROM_KEY){ // проверяем ключ 
    return true;
  }
  else // если не совпадает значит проблемы с EEPROM
  {
    return false; // возвращаем ноль
  }

}
//***********************************************************************************************

//----------
bool fnEEpromInit(void){

  Serial.println("fnEEpromInit");

  char buffer [32] = {0,};
  enum fsm_state {check_saved_setpoints, no_saved_setpoints, write_default_setpoints,\
  copy_setpoints_from_eepprom, check_saved_sensors, copy_sensors_address_from_eepprom, successfully, fault, exit};

  enum fsm_state step = check_saved_setpoints;
  bool flag_check_ok = false;
  bool flag_checked = false;

  while(!flag_checked){

    switch (step){

      case check_saved_setpoints:
        Serial.println("Check saved setpoints");
        if(fnEEpromSetpointsCheck())step = copy_setpoints_from_eepprom;
        else step = no_saved_setpoints;
        break;

      case no_saved_setpoints:
        Serial.println("no saved setpoints");
        step = write_default_setpoints;
        break;

      case write_default_setpoints:
        Serial.println("write default ");
        if(fnEEpromWriteDefaultSetpoints())step = copy_setpoints_from_eepprom;
        else step = fault;
        break;
      
      case copy_setpoints_from_eepprom:
        Serial.println("copy setpoints from eepprom");
        EEPROM.readBlock(EEPROM_SETPOINTS_ADDRESS, SetpointsUnion.setpoints_data);
        step = check_saved_sensors;
        break;

      case check_saved_sensors:
        Serial.println("check saved sensors");
        step = copy_sensors_address_from_eepprom;
        break;

      case copy_sensors_address_from_eepprom:
        Serial.println("copy sensors address from eepprom");
        EEPROM.readBlock(EEPROM_1WIRE_ADDRESS, temp_sensors_data);

        // копирование адресов датчиков из структуры уставок которая сохранена в EEPROM
        memcpy(&thermometerID_1, &temp_sensors_data.sensors_ID_array[SetpointsUnion.setpoints_data.inside_sensor_id-1][0], sizeof(thermometerID_1));  //
        memcpy(&thermometerID_2, &temp_sensors_data.sensors_ID_array[SetpointsUnion.setpoints_data.outside_sensor_id-1][0], sizeof(thermometerID_2)); //
        memcpy(&thermometerID_3, &temp_sensors_data.sensors_ID_array[SetpointsUnion.setpoints_data.fridge_sensor_id-1][0], sizeof(thermometerID_3));   //
        
        sprintf(buffer,"%x%x%x%x%x%x%x%x",thermometerID_1[0],thermometerID_1[1],thermometerID_1[2],thermometerID_1[3],thermometerID_1[4],thermometerID_1[5],thermometerID_1[6],thermometerID_1[7]);
        Serial.println(buffer);
        sprintf(buffer,"%x%x%x%x%x%x%x%x",thermometerID_2[0],thermometerID_2[1],thermometerID_2[2],thermometerID_2[3],thermometerID_2[4],thermometerID_2[5],thermometerID_2[6],thermometerID_2[7]);
        Serial.println(buffer);
        sprintf(buffer,"%x%x%x%x%x%x%x%x",thermometerID_3[0],thermometerID_3[1],thermometerID_3[2],thermometerID_3[3],thermometerID_3[4],thermometerID_3[5],thermometerID_3[6],thermometerID_3[7]);
        Serial.println(buffer);

        step = successfully;
        break;

      case successfully:
        Serial.println("successfully");
        Serial.println("");
        flag_check_ok = true;
        step = exit;
        break;

      case fault:
        Serial.println("fault");
        Serial.println("");
        flag_check_ok = false;
        step = exit;
        break;

      case exit:
        flag_checked = true;
        break;

      default:
        break;
    }
  }

  return flag_check_ok;  
}
//***********************************************************************************

//--------
void fnPumpControl(MyData &data, SetpointsStruct &setpoints){

  Serial.println("fnPumpControl");

  static bool proximity_sensor_old_state; // предыдущее состояние датчика приближения
  static uint8_t pump_out_mode_old = setpoints.pump_out_mode;

  if(setpoints.pump_out_mode != pump_out_mode_old){
    pump_out_mode_old = setpoints.pump_out_mode;
    data.pump_output_state = false;
    timerPumpOffDelay.stop();
  }

  switch (setpoints.pump_out_mode){

    case OFF_MODE:
      data.pump_output_state = false;
      timerPumpOffDelay.stop();
      break;

    case ON_MODE:
      data.pump_output_state = true;
      timerPumpOffDelay.stop();
      break;

    case AUTO_MODE:

      if (data.door_switch_state)
      {
        if ((data.proximity_sensor_state == HIGH) && (proximity_sensor_old_state == LOW) && (timerPrxSensorFeedbackDelay.isReady()))
        {
          data.pump_output_state = 1 - data.pump_output_state;
          timerPrxSensorFeedbackDelay.setInterval(PRX_SENSOR_FEEDBACK_DELAY);
          
          if (data.pump_output_state)
            timerPumpOffDelay.setInterval(setpoints.pump_off_delay * SECOND);
          else
            timerPumpOffDelay.stop();
        }
      }
      else
      {
        data.pump_output_state = LOW;
        timerPumpOffDelay.stop();
      }

      proximity_sensor_old_state = data.proximity_sensor_state;

      if (timerPumpOffDelay.isReady())
      {
        data.pump_output_state = LOW;
        timerPumpOffDelay.stop();
      }
      break;

    default:
    break;

  }  
}
//************************************************************************************

//---------------
void fnInputsUpdate(void){

  if (!digitalRead(DOOR_SWITCH_INPUT_1))inputs_undebounced_sample |= (1 << 0);
  else  inputs_undebounced_sample &= ~(1 << 0);

  if (!digitalRead(PROXIMITY_SENSOR_INPUT_2))inputs_undebounced_sample |= (1 << 1);
  else inputs_undebounced_sample &= ~(1 << 1);

  if (digitalRead(IGNITION_SWITCH_INPUT_3))inputs_undebounced_sample |= (1 << 2);
  else inputs_undebounced_sample &= ~(1 << 2);

  if (!digitalRead(LOW_WASHER_WATER_LEVEL_INPUT_4))inputs_undebounced_sample |= (1 << 3);
  else inputs_undebounced_sample &= ~(1 << 3);

  inputs_debounced_state = fnDebounce(inputs_undebounced_sample);

  main_data.door_switch_state = (inputs_debounced_state & (1 << 0));
  main_data.proximity_sensor_state = (inputs_debounced_state & (1 << 1));
  main_data.ignition_switch_state = (inputs_debounced_state & (1 << 2));
  main_data.low_washer_water_level = (inputs_debounced_state & (1 << 3));

}
//**********************************************************************************8

// Debounce
uint8_t fnDebounce(uint8_t sample) // антидребезг на основе вертикального счетчика
{
      static uint8_t state, cnt0, cnt1;
      uint8_t delta, toggle;

      delta = sample ^ state;
      cnt1 = cnt1 ^ cnt0;
      cnt0 = ~cnt0;

      cnt0 &= delta;
      cnt1 &= delta;

      toggle = cnt0 & cnt1;
      state ^= toggle;
      return state;
}
//*************************************************************************

//Outputs Update
void fnOutputsUpdate(MyData &data)
{     
  digitalWrite(WATER_PUMP_OUTPUT_1, data.pump_output_state); //
  digitalWrite(FRIDGE_OUTPUT_2, data.fridge_output_state);     //
  digitalWrite(CONVERTER_OUTPUT_3, data.converter_output_state);
  digitalWrite(SENSORS_SUPPLY_5v, data.sensors_supply_output_state);
  digitalWrite(MAIN_SUPPLY_OUT, data.main_supply_output_state);  
}
//*******************************************************************************

//-----------
void fnTempSensorsUpdate(void){

  static uint8_t temp_cnt;
  static bool flag_ds18b20_update = false;

  if(timerPeriodSensUpdate.isReady()){
  
    if (!flag_ds18b20_update){
      flag_ds18b20_update = true;
      temp_sensors.requestTemperatures(); //команда начала преобразования
    }
    else
    {
      switch (temp_cnt){

        case 0:
          main_data.inside_temperature = temp_sensors.getTempC(thermometerID_1);
          Serial.println(main_data.inside_temperature);
          temp_cnt++;
          break;

        case 1:
          main_data.outside_temperature = temp_sensors.getTempC(thermometerID_2);
          Serial.println(main_data.outside_temperature);
          temp_cnt++;
          break;

        case 2:
          main_data.fridge_temperature = temp_sensors.getTempC(thermometerID_3);
          Serial.println(main_data.fridge_temperature);
          temp_cnt = 0;
          flag_ds18b20_update = false;
          break;

        default:
          temp_cnt = 0;
        break;
      }
    }
  }
}
//******************************************************************************

//--------------
void fnPumpControl_2(MyData &data, SetpointsStruct &setpoints){

  //Serial.println("fnPumpControl_2");

  bool prx_trigged = false;
  static bool prx_old_state = data.proximity_sensor_state;
  enum fsm_state {off, wait_for_prx,wait_for_T_off};
  static enum fsm_state step = off;

  if((data.proximity_sensor_state == true) && (prx_old_state == false) && (timerPrxSensorFeedbackDelay.isReady())){

    timerPrxSensorFeedbackDelay.setTimeout(PRX_SENSOR_FEEDBACK_DELAY);
    prx_trigged = true;
  }

  prx_old_state = data.proximity_sensor_state;


  switch (setpoints.pump_out_mode){

    case OFF_MODE:
      data.pump_output_state = false;
      break;

    case ON_MODE:
      if(!data.door_switch_state)step = off;
      else data.pump_output_state = true;
      break;

    case AUTO_MODE:

      switch (step)
      {
      case off:
        data.pump_output_state = false;        
        timerPumpOffDelay.stop();
        if(data.door_switch_state)step = wait_for_prx;
        else if(!data.door_switch_state)step = off;
        
        break;

      case wait_for_prx:
        if(prx_trigged){
          timerPumpOffDelay.setTimeout(setpoints.pump_off_delay * 1000);
          data.pump_output_state = HIGH;
          step = wait_for_T_off;          
        }        
        if(!data.door_switch_state)step = off;   
        break;

      case wait_for_T_off:
        if(!data.door_switch_state || prx_trigged || timerPumpOffDelay.isReady() )step = off;        
        break;      
      
      default:
      break;
      }

      break;

    default:
    break;
  }
}
//************************************************************************************

// fnPjonReceiver
void pj_receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info){

  receive_from_ID = packet_info.tx.id; // от кого пришли данные
  Serial.print("receive_from_ID");
  Serial.println(receive_from_ID);
  
  if (receive_from_ID == PJON_WATER_FLOAT_SENSOR_ID) {
    memcpy(&pjon_sensor_receive_data, payload, sizeof(pjon_sensor_receive_data)); //... копируем данные в соответствующую структуру
    flag_pjon_water_sensor_connected = true;
    timerPjonResponse.setTimeout(PJON_RESPONSE_TIMEOUT);
    Serial.println(pjon_sensor_receive_data.value);
  }
}
//************************************************************************************************

// fnPjonSender
void fnPjonSender(void){   
    
  pjon_TX_water_sensor_response = bus.send_packet(PJON_WATER_FLOAT_SENSOR_ID, "R", 1); //отправляем запрос к датчику уровня воды    

}
//*******************************************************************************************

// fnWaterLevelControl

void fnWaterLevelControl(MyData &data, PjonReceive &pj_sensor_receive_data, SetpointsStruct &setpoints, Alarms &alarms)
{
  float water_tank_capacity_temp_value = (float)setpoints.water_tank_capacity; // для вычисления дробных значений

  switch (setpoints.water_sensor_type_selected)
  {

  case WATER_FLOAT_SENSOR_PJ:

    alarms.pj_water_sensor = !flag_pjon_water_sensor_connected;
    alarms.resist_sensor = false;

    switch (pj_sensor_receive_data.value)
    {

    case WATER_LEVEL_LESS_THEN_25: // если меньше четверти ...
      data.water_level_percent = 1;
      break;

    case WATER_LEVEL_25:
      data.water_level_percent = 25;
      // tone(BUZZER,1000, 100);
      break;

    case WATER_LEVEL_50:
      data.water_level_percent = 50;

      //tone(BUZZER,1000, 100);
      break;

    case WATER_LEVEL_75:
      data.water_level_percent = 75;

      //tone(BUZZER,1000, 100);
      break;

    case WATER_LEVEL_100: //если максимум...
      data.water_level_percent = 100;

      //tone(BUZZER,1000, 100);
      break;

    case WATER_LEVEL_SENSOR_DEFECTIVE: // если сенсор неисправен...
      data.water_level_percent = 0;
      break;

    default:
      data.water_level_percent = 0;
      break;
    }

    data.water_level_liter = (uint8_t)(setpoints.water_tank_capacity * data.water_level_percent * 0.01);
    if(!flag_pjon_water_sensor_connected){
      data.water_level_liter = 0;
      data.water_level_percent = 0;
    }

    break;

  case WATER_RESISTIVE_SENSOR: // резистивный датчик

        alarms.pj_water_sensor = false;

        data.water_level_liter = data.res_sensor_resistance * (water_tank_capacity_temp_value / setpoints.resistive_sensor_nominal);
        if (data.res_sensor_resistance > (setpoints.resistive_sensor_nominal + 10))
        {
          data.water_level_liter = 0;  //
          alarms.resist_sensor = true; //
        }
        else
        {
          alarms.resist_sensor = false;
        }

        data.water_level_percent = (uint8_t)data.water_level_liter / (setpoints.water_tank_capacity * 0.01); // литры в проценты
        break;

  default:
        break;
  }
}
//*******************************************************************************************