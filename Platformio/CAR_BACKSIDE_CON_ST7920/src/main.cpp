#include <Arduino.h>
#include <U8g2lib.h>
#include "buttonMinim.h"
#include "GyverTimer.h"
#include <GyverFilters.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <GyverTimers.h>
#include <EEPROMex.h>
#include <EEPROMVar.h>
#include "PJONSoftwareBitBang.h"
#include <ArduinoRS485.h>
#include <ArduinoModbus.h>
#include <avr/wdt.h>

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

//--------- one wire ---------------------------------------------------------------
#define TEMPERATURE_PRECISION 9                                  // точность измерения температуры 9 бит
OneWire oneWire(ONE_WIRE_PIN);                                   // порт шины 1WIRE
DallasTemperature temp_sensors(&oneWire);                        // привязка библиотеки DallasTemperature к библиотеке OneWire
DeviceAddress thermometerID_1, thermometerID_2, thermometerID_3; // резервируем адреса для трёх датчиков

//-------- Butttons --------------------------
buttonMinim buttonUp(BUTTON_UP);
buttonMinim buttonDown(BUTTON_DOWN);
buttonMinim buttonEnter(BUTTON_ENTER_ESC);

//-------- Timers ----------------------------
GTimer timerBlink(MS);
GTimer timerPumpOffDelay(MS);
GTimer timerPrxSensorFeedbackDelay(MS);
GTimer timerPeriodSensUpdate(MS);
GTimer timerPjonSender(MS);
GTimer timerPjonResponse(MS);
GTimer timerMenuUpdate(MS);
GTimer timerLowUConverterOffDelay(MS);
GTimer timerConverterShutdownDelay(MS);
GTimer timerLowUFridgeOffDelay(MS);
GTimer timerFridgeShutdownDelay(MS);
GTimer timerShutdownDelay(MS);
GTimer timerSensSupplyCheck(MS);
GTimer timerStartDelay(MS);   // таймер задержки опроса входов после старта
GTimer timerBrightnessOff(MS);
GTimer timerDebugPrint(MS);
GTimer timerMbCheckConn(MS);
GTimer timerMBdelay(MS);
GTimer timerScreenSaver(MS);

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
void fnConverterControl(MyData &data, SetpointsStruct &setpoints);
void fnFridgeControl(MyData &data, SetpointsStruct &setpoints);
void fnMainPowerControl(MyData &data, SetpointsStruct &setpoints, GTimer &timer);
void fnSensorsSupplyControl(MyData &data, GTimer &timer, Alarms &alarms);
void fnAlarms(MyData &data, Alarms &alarms);
void fnBuzzerProcess(MyData &data, Alarms &alarms);
void fnLcdBrightnessControl(MyData &data, SetpointsStruct &setpoints, GTimer &timer);
void fnMenuProcess(void);
void fnDebugPrint(void);


//**********************************************************************************

void setup() {
  
  old_time = millis();
  Serial.begin(115200);
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  display_height = u8g2.getDisplayHeight();
  display_width = u8g2.getDisplayWidth();
  display_num_lines = display_height / (LCD_FONT_HIGHT + LCD_LINE_SPACER);
  fnDefaultSetpointsInit();
  fnIOInit();

  digitalWrite(WDT_RESET_OUT, !digitalRead(WDT_RESET_OUT));

  u8g2.clearBuffer();
  u8g2.drawXBMP(33,5,64,55,FK_logo_64x55);
  u8g2.sendBuffer();
  delay(1000);

  if(fnEEpromInit()){
    u8g2.clearBuffer();
    u8g2.drawStr(40,30,"READY!");
    u8g2.sendBuffer();
    tone(BUZZER,500,200);
    delay(500);
  } 
  else {
    u8g2.clearBuffer();
    u8g2.drawStr(40,30,"ERROR!");
    u8g2.sendBuffer();
    memcpy(&SetpointsUnion.setpoints_data, &default_setpoints_data, sizeof(SetpointsUnion.setpoints_data));//
    tone(BUZZER,1000,200);
    delay(400);
    tone(BUZZER,1000,200);
    delay(400);    
  }
  
  digitalWrite(WDT_RESET_OUT, !digitalRead(WDT_RESET_OUT));

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
  timerLowUConverterOffDelay.setMode(MANUAL);
  timerLowUConverterOffDelay.setInterval(((uint32_t)SetpointsUnion.setpoints_data.converter_T_U_off) * SECOND);
  timerConverterShutdownDelay.setMode(MANUAL);
  timerConverterShutdownDelay.setInterval(((uint32_t)SetpointsUnion.setpoints_data.converter_T_IGN_off) * MINUTE);
  timerLowUFridgeOffDelay.setMode(MANUAL);
  timerLowUFridgeOffDelay.setInterval(((uint32_t)SetpointsUnion.setpoints_data.fridge_T_U_off) * SECOND);
  timerFridgeShutdownDelay.setMode(MANUAL);
  timerFridgeShutdownDelay.setInterval(((uint32_t)SetpointsUnion.setpoints_data.fridge_T_IGN_off) * MINUTE);
  timerShutdownDelay.setMode(MANUAL);
  timerShutdownDelay.setTimeout(SetpointsUnion.setpoints_data.shutdown_delay * HOUR);
  timerSensSupplyCheck.setMode(MANUAL);
  timerSensSupplyCheck.setInterval(SENS_SUPPLY_CHECK_START_DELAY);
  timerStartDelay.setMode(MANUAL);
  timerBrightnessOff.setMode(MANUAL);
  timerBrightnessOff.setTimeout(SetpointsUnion.setpoints_data.scrreen_off_delay * SECOND);
  timerDebugPrint.setInterval(500);
  timerMbCheckConn.setMode(MANUAL);
  timerMbCheckConn.setTimeout(MB_CHECK_CONN_TIME);
  timerMBdelay.setInterval(50);
  timerScreenSaver.setTimeout(SetpointsUnion.setpoints_data.screensaver_timeout * MINUTE);

  digitalWrite(WDT_RESET_OUT, !digitalRead(WDT_RESET_OUT));

  ps_voltage_filter.setCoef(0.1); // установка коэффициента фильтрации (0.0... 1.0). Чем меньше, тем плавнее фильтр
  ps_voltage_filter.setStep(20);  // установка шага фильтрации (мс). Чем меньше, тем резче фильтр
  sens_voltage_filter.setCoef(0.1);
  sens_voltage_filter.setStep(20);
  resistive_sensor_filter.setCoef(0.02);
  resistive_sensor_filter.setStep(1000);


  if(!digitalRead(BUTTON_DOWN) && !digitalRead(BUTTON_UP)){
    fnOneWireScanner();
  }
  
  menu_mode = MENU_MAIN_VIEW;

  digitalWrite(SENSORS_SUPPLY_5v, HIGH);
  fnTempSensorsUpdate();

  bus.begin();  
  bus.strategy.set_pin(PJON_BUS_PIN); // выбор пина дя передачи данных
  bus.set_id(PJON_MY_ID); //  установка собственного ID   
  bus.set_receiver(pj_receiver_function);

  ModbusRTUServer.begin(SetpointsUnion.setpoints_data.mb_slave_ID, main_data.mb_rates[SetpointsUnion.setpoints_data.mb_baud_rate]); // настройка порта в файле RS485.cpp в конце
  ModbusRTUServer.configureCoils(0x00, 10);
  ModbusRTUServer.configureHoldingRegisters(0x00, 10);

  timerStartDelay.setInterval(START_DELAY);
 
  wdt_enable(WDTO_500MS);

  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
    Serial.print("Setup time  ");
    Serial.print(millis()-old_time);
    Serial.println(" ms");
  }
  
}
//**********************************************************************************************


void loop() {
  
  loop_old_time = millis();
  wdt_reset();
  
  digitalWrite(WDT_RESET_OUT, !digitalRead(WDT_RESET_OUT));
    
  main_data.sensors_supply_voltage = (analogRead(SENSORS_VOLTAGE_INPUT) * DIVISION_RATIO_SENS_SUPPLY_INPUT);
  main_data.res_sensor_resistance = (uint16_t) (resistive_sensor_filter.filtered(analogRead(RESISTIVE_SENSOR)) * (DIVISION_RATIO_RESIST_SENSOR + ((float)SetpointsUnion.setpoints_data.resistive_sensor_correction/1000))); 

  if(timerStartDelay.isReady()){
    timerStartDelay.stop();
    main_data.flag_system_started = true;
    if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
      Serial.println(F("timerStartDelay is Ready"));
    }
  }

  if(main_data.flag_system_started == true){

    fnInputsUpdate();

    switch (main_process_step)
    {
    case 0:
      fnMenuProcess();
      main_process_step++;
      break;

    case 1:
      main_data.battery_voltage = (analogRead(SUPPLY_VOLTAGE_INPUT)) * (DIVISION_RATIO_VOLTAGE_INPUT + ((float)SetpointsUnion.setpoints_data.voltage_correction/10000));
      main_process_step++; 
      break;

    case 2:
      fnSensorsSupplyControl(main_data, timerSensSupplyCheck, present_alarms);
      fnMainPowerControl(main_data, SetpointsUnion.setpoints_data, timerShutdownDelay);
      fnTempSensorsUpdate();
      main_process_step++;
      break;

    case 3:
      fnWaterLevelControl(main_data, pjon_sensor_receive_data, SetpointsUnion.setpoints_data, present_alarms);
      main_process_step++;
      break;

    case 4:
      fnConverterControl(main_data, SetpointsUnion.setpoints_data);
      fnFridgeControl(main_data, SetpointsUnion.setpoints_data);
      fnPumpControl_2(main_data, SetpointsUnion.setpoints_data);
      main_process_step++;
      break;

    case 5:
      fnAlarms(main_data, present_alarms);
      fnBuzzerProcess(main_data, present_alarms);
      fnLcdBrightnessControl(main_data, SetpointsUnion.setpoints_data,timerBrightnessOff);
      main_process_step++;
      break;

    case 6:
      // ********* ModBus registers update ******************************
      ModbusRTUServer.coilWrite(0x00, main_data.ignition_switch_state);
      ModbusRTUServer.coilWrite(0x01, main_data.door_switch_state);
      ModbusRTUServer.coilWrite(0x02, main_data.proximity_sensor_state);
      ModbusRTUServer.coilWrite(0x03, main_data.pump_output_state);
      ModbusRTUServer.coilWrite(0x04, main_data.converter_output_state);
      ModbusRTUServer.coilWrite(0x05, main_data.fridge_output_state);
      ModbusRTUServer.coilWrite(0x06, flag_pjon_water_sensor_connected);     
      ModbusRTUServer.coilWrite(0x08, main_data.sensors_supply_output_state);
      ModbusRTUServer.coilWrite(0x09, main_data.low_washer_water_level); //

      wdt_reset();

      ModbusRTUServer.holdingRegisterWrite(0x00, main_data.battery_voltage * 10);
      ModbusRTUServer.holdingRegisterWrite(0x01, main_data.inside_temperature * 10);
      ModbusRTUServer.holdingRegisterWrite(0x02, main_data.outside_temperature * 10);
      ModbusRTUServer.holdingRegisterWrite(0x03, main_data.water_level_percent);
      ModbusRTUServer.holdingRegisterWrite(0x04, main_data.water_level_liter);

      ModbusRTUServer.holdingRegisterWrite(0x05, main_data.sensors_supply_voltage * 10);
      ModbusRTUServer.holdingRegisterWrite(0x06, main_data.res_sensor_resistance);
      ModbusRTUServer.holdingRegisterWrite(0x07, 0);
      ModbusRTUServer.holdingRegisterWrite(0x08, 0);
      ModbusRTUServer.holdingRegisterWrite(0x09, 0);

      // проверка подключения по modbus. Мастер должен записывать еденицу в coil 0x07 не реже 1 раз в пять сек
      if(ModbusRTUServer.coilRead(0x07) == 1){  
        timerMbCheckConn.setTimeout(MB_CHECK_CONN_TIME);
        flag_mb_connected = true;
        ModbusRTUServer.coilWrite(0x07, 0);   
      }  
      if(timerMbCheckConn.isReady()){
        flag_mb_connected = false;
      }

      main_process_step++;
      break;

    case 7:
      fnOutputsUpdate(main_data);
      if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
        if(timerDebugPrint.isReady())fnDebugPrint();
      }
      main_process_step = 0;
      break;  
    
    default:
      main_process_step = 0;
      break;
    }

  }

  wdt_reset();
  //pjon
  if(timerPjonSender.isReady())fnPjonSender();
  if(timerPjonResponse.isReady())flag_pjon_water_sensor_connected = false;
  bus.receive(300);  
  bus.update();

  //delay(1);
  if(timerMBdelay.isReady()){
    old_time = millis();
    ModbusRTUServer.poll();
    if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
      Serial.print(F("ModBusPool time  "));
      Serial.print(millis()-old_time);
      Serial.println(F(" ms"));
    }
  }
  

  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
    Serial.println(F(""));
    Serial.print(F("**************************** Timing "));
    Serial.print(millis());
    Serial.println(F(" ms ****************************"));
    Serial.print(F("Loop  "));
    Serial.print(millis()-loop_old_time);
    Serial.println(F(" ms"));
  }

}
//****** end loop **************************************************************************************************

//----------- Функция печати рамки навигации по меню ------------------- 
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


//-----------Функция печати имени пункта меню из progmem (общая для всех меню) --------------
void fnPrintMenuItemName(uint8_t _num_item, uint8_t _num_line, const char* const* _names) {
  
  char buffer[32] = {0,};                            // Буфер на полную строку
  
  uint16_t ptr = 0;
  ptr = pgm_read_word(&(_names[_num_item]));         // Получаем указатель на первый символ строки
  uint8_t i = 0;                                     // Переменная - счетчик

  do {                                            // Начало цикла
    buffer[i] = (char)(pgm_read_byte(ptr++));     // Прочитать в буфер один символ из PGM и подвинуть указатель на 1
    i++;
  } while (i<ITEM_MAX_CHARS);                     // Если это не конец строки - вернуться в начало цикла
  

  /*
   while (pgm_read_byte(ptr) != NULL) {           // всю строку до нулевого символа
      buffer[i++] = (char)(pgm_read_byte(ptr));   // выводим 
      ptr++;                                      // следующий символ
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
    sprintf(buffer, "%ds", SetpointsUnion.SetpointsArray[num_item]);
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
    sprintf(buffer, "%ds", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 4:
    float_m = SetpointsUnion.SetpointsArray[num_item];
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%d",float_m, float_n);
    break;

  case 5:
    sprintf(buffer, "%ds", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 6:
    sprintf(buffer, "%dm", SetpointsUnion.SetpointsArray[num_item]);
    break;
  
  case 7:
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

  case 8:
    float_m = SetpointsUnion.SetpointsArray[num_item];
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%d",float_m, float_n);
    break;

  case 9:
    sprintf(buffer, "%ds", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 10:
    float_m = SetpointsUnion.SetpointsArray[num_item];
    float_n = float_m%10;
    float_m = float_m/10;
    sprintf(buffer,"%d.%d",float_m, float_n);
    break;

  case 11:
    sprintf(buffer, "%ds", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 12:
    sprintf(buffer, "%dm", SetpointsUnion.SetpointsArray[num_item]);
    break;
  
  case 13:
    sprintf(buffer, "%dC", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 14:
    sprintf(buffer, "%dC", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 15:
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
  
  case 20:
    sprintf(buffer, "%dL", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 21:
    sprintf(buffer, "%d", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 22:

    switch (SetpointsUnion.SetpointsArray[num_item])
    {
    case MB_RATE_4800:
      sprintf_P(buffer,PSTR("4k8"));
      break;

    case MB_RATE_7200:
      sprintf_P(buffer,PSTR("7k2"));
      break;

    case MB_RATE_9600:
      sprintf_P(buffer,PSTR("9k6"));
      break;

    case MB_RATE_19200:
      sprintf_P(buffer,PSTR("19k2"));
      break;

    case MB_RATE_38400:
      sprintf_P(buffer,PSTR("38k4"));
      break;

    case MB_RATE_57600:
      sprintf_P(buffer,PSTR("57k6"));
      break;
    
    default:
      sprintf_P(buffer,PSTR("?"));
      break;
    }
    
    break;

  case 23:

    switch (SetpointsUnion.SetpointsArray[num_item])
    {
    case 0:
      sprintf_P(buffer,PSTR("off"));
      break;
    case 1:
      sprintf(buffer,"on");
      break;
    
    default:
      break;
    }
    
    break;

  case 24:
    sprintf(buffer, "%ds", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 25:
    sprintf(buffer, "%dh", SetpointsUnion.SetpointsArray[num_item]);
    break;

  case 26:
    sprintf(buffer,"%d",SetpointsUnion.SetpointsArray[num_item]);
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

    switch (SetpointsUnion.SetpointsArray[num_item])
    {
    case DEBUG_KEY_0:
      sprintf(buffer, "off");
      break;

    case DEBUG_KEY_1:
      sprintf(buffer, "on");
      break;
    
    default:
      break;
    }
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

  if(flag_mb_connected){
    u8g2.drawBox(19,1,11,8);
    u8g2.setDrawColor(0);
    u8g2.drawStr(20, 8, "MB");
    u8g2.setDrawColor(1);
  }
  else{
    u8g2.setDrawColor(0);
    u8g2.drawBox(19,1,16,8);
    u8g2.setDrawColor(1);
  }

  sprintf(buffer,"%d L",main_data.water_level_liter);
  u8g2.setFont(u8g2_font_ncenB18_tr);	//
  u8g2.drawStr(60, 58, buffer);


  // u8g2.drawXBMP(5, 12, 50, 50, water_level_50x50);
  u8g2.drawXBMP(2, 12, 50, 50, water_level_tap_50x50);

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


//----- функция печати значения пункта меню просмотра параметров ------------------------------
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
    if(main_data.res_sensor_resistance <= MAX_RESISTANCE)sprintf(buffer,"%d",main_data.res_sensor_resistance);
    else sprintf(buffer,"xxx");
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
    sprintf(buffer,"%1u", present_alarms.temp_sensors);
    break;

  case 16:
    switch (SetpointsUnion.setpoints_data.water_sensor_type_selected)
    {
    case WATER_FLOAT_SENSOR_PJ:
      sprintf(buffer,"%1u", present_alarms.pj_water_sensor);
      break;

    case WATER_RESISTIVE_SENSOR:
      sprintf(buffer,"%1u", present_alarms.resist_water_sensor);
      break;  
    
    default:
      sprintf(buffer,"?");
      break;
    }
    
    break;
  
  case 17:
    sprintf(buffer,"%1u", present_alarms.sens_supply);
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

  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1)Serial.println(F("fnOneWireScanner"));

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
      if(buttonEnter.clicked())current_state = scanner;
      if(buttonDown.clicked())current_state = exit;
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

      if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
        Serial.print(F("Founded sensors "));
        Serial.println(num_founded_sensors);
      }
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
      if(buttonDown.clicked())current_state = exit;
      if(buttonEnter.clicked())current_state = save;
      if(buttonUp.clicked())current_state = scanner;
      break;
    
    case not_founded:

      u8g2.clearBuffer();		
      u8g2.setFont(u8g2_font_ncenB08_tr);		
      u8g2.drawStr(15,20, "No sensors founded");
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(7,50, "UP-> scan");
      u8g2.drawStr(7,60, "DOWN-> exit");
      u8g2.sendBuffer();
      if(buttonUp.clicked())current_state = scanner;
      if(buttonDown.clicked())current_state = exit;
      break;

    case founded_more_three:
    
      u8g2.clearBuffer();		
      u8g2.setFont(u8g2_font_ncenB08_tr);		
      u8g2.drawStr(30,40, "founded > 3");
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(7,50, "UP-> scan");
      u8g2.drawStr(7,60, "DOWN-> exit");
      u8g2.sendBuffer();
      if(buttonUp.clicked())current_state = scanner;
      if(buttonDown.clicked())current_state = exit;
      break;

    case save:

      u8g2.clearBuffer();	

      u8g2.drawStr(2, 10, "ID1: ");
      u8g2.drawStr(2, 20, "ID2: ");
      u8g2.drawStr(2, 30, "ID3: ");

      sprintf(buffer,"%x%x%x%x%x%x%x%x", tmp_thermometerID_1[0],tmp_thermometerID_1[1],tmp_thermometerID_1[2],tmp_thermometerID_1[3],tmp_thermometerID_1[4],tmp_thermometerID_1[5],tmp_thermometerID_1[6],tmp_thermometerID_1[7]);
      if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1)Serial.println(buffer);
      u8g2.drawStr(30,10, buffer); //  

      sprintf(buffer,"%x%x%x%x%x%x%x%x",tmp_thermometerID_2[0],tmp_thermometerID_2[1],tmp_thermometerID_2[2],tmp_thermometerID_2[3],tmp_thermometerID_2[4],tmp_thermometerID_2[5],tmp_thermometerID_2[6],tmp_thermometerID_2[7]);
      if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1)Serial.println(buffer);
      u8g2.drawStr(30,20, buffer);

      sprintf(buffer,"%x%x%x%x%x%x%x%x",tmp_thermometerID_3[0],tmp_thermometerID_3[1],tmp_thermometerID_3[2],tmp_thermometerID_3[3],tmp_thermometerID_3[4],tmp_thermometerID_3[5],tmp_thermometerID_3[6],tmp_thermometerID_3[7]);
      if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1)Serial.println(buffer);
      u8g2.drawStr(30,30, buffer); 			
      
      u8g2.drawStr(7,40, "UP-> scan");
      u8g2.drawStr(7,50, "OK-> save");
      u8g2.drawStr(7,60, "DOWN-> exit");
      u8g2.sendBuffer();

      if(buttonEnter.clicked()){

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

      if(buttonDown.clicked())current_state = exit;
      if(buttonUp.clicked())current_state = scanner;
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
    if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
      Serial.print(F("EEPROM KEY OK!  "));  
      Serial.println(EEPROM_KEY); 
    }           
    return true; // возвращаем один
  }
  else // если ключ  не совпадает значит первый запуск
  {
    if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
      Serial.print(F("EEPROM KEY IS NOT VALID!  "));  
      Serial.println(EEPROM_KEY); 
    }          
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

  old_time = millis();
  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1)Serial.println(F("fnEEpromInit"));

  char buffer [32] = {0,};
  enum fsm_state {check_saved_setpoints, no_saved_setpoints, write_default_setpoints,\
  copy_setpoints_from_eepprom, check_saved_sensors, copy_sensors_address_from_eepprom, successfully, fault, exit};

  enum fsm_state step = check_saved_setpoints;
  bool flag_check_ok = false;
  bool flag_checked = false;

  while(!flag_checked){

    switch (step){

      case check_saved_setpoints:
        if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1)Serial.println(F("Check saved setpoints"));
        if(fnEEpromSetpointsCheck())step = copy_setpoints_from_eepprom;
        else step = no_saved_setpoints;
        break;

      case no_saved_setpoints:
        if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1)Serial.println(F("no saved setpoints"));
        step = write_default_setpoints;
        break;

      case write_default_setpoints:
        if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1)Serial.println(F("write default "));
        u8g2.clearBuffer();
        u8g2.drawStr(20,30,"WRITE DEFAULT...");
        u8g2.sendBuffer();
        delay(2000);
        if(fnEEpromWriteDefaultSetpoints())step = copy_setpoints_from_eepprom;
        else step = fault;
        break;
      
      case copy_setpoints_from_eepprom:
        if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1)Serial.println(F("copy setpoints from eepprom"));
        EEPROM.readBlock(EEPROM_SETPOINTS_ADDRESS, SetpointsUnion.setpoints_data);
        step = check_saved_sensors;
        break;

      case check_saved_sensors:
        if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1)Serial.println(F("check saved sensors"));
        step = copy_sensors_address_from_eepprom;
        break;

      case copy_sensors_address_from_eepprom:
        if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1)Serial.println(F("Copy sensors address from eepprom..."));
        EEPROM.readBlock(EEPROM_1WIRE_ADDRESS, temp_sensors_data);

        // копирование адресов датчиков из структуры которая сохранена в EEPROM
        memcpy(&thermometerID_1, &temp_sensors_data.sensors_ID_array[SetpointsUnion.setpoints_data.inside_sensor_id-1][0], sizeof(thermometerID_1));  //
        memcpy(&thermometerID_2, &temp_sensors_data.sensors_ID_array[SetpointsUnion.setpoints_data.outside_sensor_id-1][0], sizeof(thermometerID_2)); //
        memcpy(&thermometerID_3, &temp_sensors_data.sensors_ID_array[SetpointsUnion.setpoints_data.fridge_sensor_id-1][0], sizeof(thermometerID_3));   //
        
        sprintf(buffer,"%x%x%x%x%x%x%x%x",thermometerID_1[0],thermometerID_1[1],thermometerID_1[2],thermometerID_1[3],thermometerID_1[4],thermometerID_1[5],thermometerID_1[6],thermometerID_1[7]);
        if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1){
          Serial.print(F("thermometerID_1 "));Serial.println(buffer);
        } 
        sprintf(buffer,"%x%x%x%x%x%x%x%x",thermometerID_2[0],thermometerID_2[1],thermometerID_2[2],thermometerID_2[3],thermometerID_2[4],thermometerID_2[5],thermometerID_2[6],thermometerID_2[7]);
        if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1){
          Serial.print(F("thermometerID_2 "));Serial.println(buffer);
        }
        sprintf(buffer,"%x%x%x%x%x%x%x%x",thermometerID_3[0],thermometerID_3[1],thermometerID_3[2],thermometerID_3[3],thermometerID_3[4],thermometerID_3[5],thermometerID_3[6],thermometerID_3[7]);
        if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1){
          Serial.print(F("thermometerID_3 "));Serial.println(buffer);
        } 

        step = successfully;
        break;

      case successfully:
        if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1){
          Serial.println(F("successfully"));
          Serial.println(F(""));
        }
        flag_check_ok = true;
        step = exit;
        break;

      case fault:
        if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1){
          Serial.println(F("fault"));
          Serial.println(F(""));
        }
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

  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
    Serial.print(F("EepromInit time  "));
    Serial.print(millis()-old_time);
    Serial.println(F(" ms"));
  }

  return flag_check_ok;  
}
//***********************************************************************************

//--------
void fnPumpControl(MyData &data, SetpointsStruct &setpoints){

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

  old_time = millis();
  static uint8_t inputs_undebounced_sample = 0;
  static uint8_t inputs_debounced_state = 0;

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

  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
    Serial.print(F("InputsUpdate time  "));
    Serial.print(millis()-old_time);
    Serial.println(F(" ms"));
  }

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
  old_time = millis();
  digitalWrite(WATER_PUMP_OUTPUT_1, data.pump_output_state); //
  digitalWrite(FRIDGE_OUTPUT_2, data.fridge_output_state);     //
  digitalWrite(CONVERTER_OUTPUT_3, data.converter_output_state);
  digitalWrite(SENSORS_SUPPLY_5v, data.sensors_supply_output_state);
  digitalWrite(MAIN_SUPPLY_OUT, data.main_supply_output_state); 

  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
    Serial.print(F("OutputsUpdate time  "));
    Serial.print(millis()-old_time);
    Serial.println(F(" ms"));
  } 
}
//*******************************************************************************

//-----------
void fnTempSensorsUpdate(void){

  old_time = millis();
  static uint8_t temp_cnt = 1;
  static bool flag_ds18b20_update = false;

  if (temp_sensors_data.num_saved_sensors > 0){
 
    if(timerPeriodSensUpdate.isReady()){
    
      if (!flag_ds18b20_update){
        flag_ds18b20_update = true;
        temp_sensors.requestTemperatures(); //команда начала преобразования
      }
      else
      {
        present_alarms.temp_sensors = false;  
           
        switch (temp_cnt){

          case 1:
            main_data.inside_temperature = temp_sensors.getTempC(thermometerID_1);
            if (main_data.inside_temperature == DEVICE_DISCONNECTED_C)present_alarms.temp_sensors = true;
            if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1)Serial.println(main_data.inside_temperature);
            break;

          case 2:
            main_data.outside_temperature = temp_sensors.getTempC(thermometerID_2);
            if (main_data.outside_temperature == DEVICE_DISCONNECTED_C)present_alarms.temp_sensors = true;
            if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1)Serial.println(main_data.outside_temperature);
            break;

          case 3:
            main_data.fridge_temperature = temp_sensors.getTempC(thermometerID_3);
            if (main_data.fridge_temperature == DEVICE_DISCONNECTED_C)present_alarms.temp_sensors = true;
            if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1)Serial.println(main_data.fridge_temperature);
            break;

          default:
            temp_cnt = 1;
          break;
        }

        temp_cnt++ ;
        if(temp_cnt > temp_sensors_data.num_saved_sensors){
          temp_cnt = 1;
          flag_ds18b20_update = false;
        }

      }
    }
  }

  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
    Serial.print(F("TempSensorsUpdate time  "));
    Serial.print(millis()-old_time);
    Serial.println(F(" ms"));
  }
}
//******************************************************************************

//--------------
void fnPumpControl_2(MyData &data, SetpointsStruct &setpoints){

  old_time = millis();
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
  
  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
    Serial.print(F("PumpControl time  "));
    Serial.print(millis()-old_time);
    Serial.println(F(" ms"));
  }
}
//************************************************************************************

// fnPjonReceiver
void pj_receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info){

  old_time = millis();
  receive_from_ID = packet_info.tx.id; // от кого пришли данные
  if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1){
    Serial.print(F("PJ receive_from_ID "));
    Serial.println(receive_from_ID);
  }
  
  if (receive_from_ID == PJON_WATER_FLOAT_SENSOR_ID) {
    memcpy(&pjon_sensor_receive_data, payload, sizeof(pjon_sensor_receive_data)); //... копируем данные в соответствующую структуру
    flag_pjon_water_sensor_connected = true;
    timerPjonResponse.setTimeout(PJON_RESPONSE_TIMEOUT);
    if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1){
      Serial.print(F("PJ receive data "));Serial.println(pjon_sensor_receive_data.value);
    }
    
  }

  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
    Serial.print(F("pj_receiver_function time  "));
    Serial.print(millis()-old_time);
    Serial.println(F(" ms"));
  }
}
//************************************************************************************************

// fnPjonSender
void fnPjonSender(void){   

  old_time = millis();  
  pjon_TX_water_sensor_response = bus.send_packet(PJON_WATER_FLOAT_SENSOR_ID, "R", 1); //отправляем запрос к датчику уровня воды    
  
  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
    Serial.print(F("PjonSender time  "));
    Serial.print(millis()-old_time);
    Serial.println(F(" ms"));
  }

}
//*******************************************************************************************

// fnWaterLevelControl
void fnWaterLevelControl(MyData &data, PjonReceive &pj_sensor_receive_data, SetpointsStruct &setpoints, Alarms &alarms)
{
  old_time = millis();

  float water_tank_capacity_temp_value = (float)setpoints.water_tank_capacity; // для вычисления дробных значений

  switch (setpoints.water_sensor_type_selected)
  {

  case WATER_FLOAT_SENSOR_PJ:

    alarms.pj_water_sensor = !flag_pjon_water_sensor_connected;
    alarms.resist_water_sensor = false;

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

        data.water_level_liter =  map((long)data.res_sensor_resistance, (long)setpoints.resistive_sensor_min,(long)setpoints.resistive_sensor_max,0,(long)setpoints.water_tank_capacity);
        
        if (data.res_sensor_resistance > ((uint16_t)setpoints.resistive_sensor_max + 10))
        {
          data.water_level_liter = 0;  //
          alarms.resist_water_sensor = true; //
        }
        else
        {
          alarms.resist_water_sensor = false;
        }

        data.water_level_percent = (uint8_t)data.water_level_liter / (setpoints.water_tank_capacity * 0.01); // литры в проценты
        break;

  default:
        break;
  }

  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
    Serial.print(F("WaterLevelControl time  "));
    Serial.print(millis()-old_time);
    Serial.println(F(" ms"));
  }
}
//*******************************************************************************************

//convreter control
void fnConverterControl(MyData &data, SetpointsStruct &setpoints)
{
  old_time = millis();

  uint8_t voltage = (uint8_t)data.battery_voltage * 10;
  static bool flag_convOff_due_voltage;    // флаг что конветер был выключен по напряжению
  static bool flag_convOff_due_ign_switch; // флаг что конветер был выключен по таймеру после выключения зажигания

  static bool state = HIGH; // изначально (после старта) включен

  switch (setpoints.convertet_out_mode){
      
    case OFF_MODE:
      state = LOW;
      timerLowUConverterOffDelay.stop();  // останавливаем таймер выключения
      timerConverterShutdownDelay.stop(); 
      break;

    case ON_MODE:
      state = HIGH;
      timerLowUConverterOffDelay.stop();  // останавливаем таймер выключения
      timerConverterShutdownDelay.stop();
      break;

    case AUTO_MODE:

      if (voltage >= SetpointsUnion.setpoints_data.converter_U_on)
      { 
        if (!flag_convOff_due_ign_switch)state = HIGH; // если напряжение в пределах нормы включаем преобразователь
        flag_convOff_due_voltage = LOW;    // флаг что было отключение по низкому напряжению
        timerLowUConverterOffDelay.stop(); // останавливваем таймер выключения
      }

      if (voltage > SetpointsUnion.setpoints_data.converter_U_off)
      {                                                                                                     
        timerLowUConverterOffDelay.setInterval(((uint32_t)SetpointsUnion.setpoints_data.converter_T_U_off) * SECOND); // заряжаем таймер на выключение
      }

      else
      {
        if (timerLowUConverterOffDelay.isReady())
        {
          state = LOW;
          flag_convOff_due_voltage = HIGH;   // флаг что было отключение по низкому напряжению
          timerLowUConverterOffDelay.stop(); // останавливаем таймер выключения
        }
      }

      // отключение по таймеру после выключения зажигания
      if (data.ignition_switch_state)
      {
        flag_convOff_due_ign_switch = LOW; //сброс флага что было отключение по ignition switch
        if (!flag_convOff_due_voltage)state = HIGH;
        timerConverterShutdownDelay.setInterval(((uint32_t)SetpointsUnion.setpoints_data.converter_T_IGN_off) * MINUTE);
      }
      else
      {
        if (timerConverterShutdownDelay.isReady())
        {
          state = LOW;
          flag_convOff_due_ign_switch = HIGH; //установка флага что было отключение по ignition switch
        }
      }

      break;

    default:
    break;
  }

  data.converter_output_state = state;

  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
    Serial.print(F("ConverterControl time  "));
    Serial.print(millis()-old_time);
    Serial.println(F(" ms"));
  }

}
//*****************************************************************************************

//fridge control
void fnFridgeControl(MyData &data, SetpointsStruct &setpoints)
{
  old_time = millis();

  uint8_t voltage = (uint8_t)data.battery_voltage * 10;
  static bool flag_fridgeOff_due_voltage;    // флаг что конветер был выключен по напряжению
  static bool flag_fridgeOff_due_ign_switch; // флаг что конветер был выключен по таймеру после выключения зажигания

  static bool state = HIGH; // изначально (после старта) включен

  switch (setpoints.fridge_out_mode){
      
    case OFF_MODE:
      state = LOW;
      timerLowUFridgeOffDelay.stop();  // останавливаем таймер выключения
      timerFridgeShutdownDelay.stop(); 
      break;

    case ON_MODE:
      state = HIGH;
      timerLowUFridgeOffDelay.stop();  // останавливаем таймер выключения
      timerFridgeShutdownDelay.stop();
      break;

    case AUTO_MODE:

      if (voltage >= SetpointsUnion.setpoints_data.fridge_U_on)
      { 
        if (!flag_fridgeOff_due_ign_switch)state = HIGH; // если напряжение в пределах нормы включаем преобразователь
        flag_fridgeOff_due_voltage = LOW;    // флаг что было отключение по низкому напряжению
        timerLowUFridgeOffDelay.stop(); // останавливваем таймер выключения
      }

      if (voltage > SetpointsUnion.setpoints_data.fridge_U_off)
      {                                                                                                     
        timerLowUFridgeOffDelay.setInterval(((uint32_t)SetpointsUnion.setpoints_data.fridge_T_U_off) * SECOND); // заряжаем таймер на выключение
      }

      else
      {
        if (timerLowUFridgeOffDelay.isReady())
        {
          state = LOW;
          flag_fridgeOff_due_voltage = HIGH;   // флаг что было отключение по низкому напряжению
          timerLowUFridgeOffDelay.stop(); // останавливаем таймер выключения
        }
      }

      // отключение по таймеру после выключения зажигания
      if (data.ignition_switch_state)
      {
        flag_fridgeOff_due_ign_switch = LOW; //сброс флага что было отключение по ignition switch
        if (!flag_fridgeOff_due_voltage)state = HIGH;
        timerFridgeShutdownDelay.setInterval(((uint32_t)SetpointsUnion.setpoints_data.fridge_T_IGN_off) * MINUTE);
      }
      else
      {
        if (timerFridgeShutdownDelay.isReady())
        {
          state = LOW;
          flag_fridgeOff_due_ign_switch = HIGH; //установка флага что было отключение по ignition switch
        }
      }

      break;

    default:
    break;
  }

  data.fridge_output_state = state;

  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
    Serial.print(F("FridgeControl time  "));
    Serial.print(millis()-old_time);
    Serial.println(F(" ms"));
  }

}
//*****************************************************************************************

// Main power control 
void fnMainPowerControl(MyData &data, SetpointsStruct &setpoints, GTimer &timer)
{
  static bool state = true;

  if(data.ignition_switch_state)
  {
    timer.setTimeout((uint32_t)setpoints.shutdown_delay * HOUR);
    state = true;
  }
  else
  {
    if(timer.isReady())state = false;
  }

  data.main_supply_output_state = state;
}
//**********************************************************************

// Sensors Supply Control
void fnSensorsSupplyControl(MyData &data, GTimer &timer, Alarms &alarms)
{
  static uint8_t step = 0;
  static uint8_t sens_supply_check_cnt = 0;
  static bool state = true;

  if (timer.isReady())
  {
    switch (step)
    {
      case 0:
        state = true;
        timer.setInterval(SENS_SUPPLY_CHECK_PERIOD);
        step = 1;
        break;

      case 1:
        if (data.sensors_supply_voltage < SENS_SUPPLY_CHECK_MIN_V)
        {
          sens_supply_check_cnt++;
          timer.setInterval(SENS_SUPPLY_CHECK_PERIOD);
        }
        else
        {
          if (sens_supply_check_cnt)sens_supply_check_cnt--;
        }

        if (sens_supply_check_cnt > SENS_SUPPLY_CHECK_TIMES)step = 2;
        break;

      case 2:
        state = false;
        alarms.sens_supply = true;
        timer.setInterval(SENS_SUPPLY_CHECK_PERIOD * 3);
        step = 3;
        break;

      case 3:
        if (!alarms.sens_supply)
        {
          state = true;
          step = 0;
          sens_supply_check_cnt = 0;
        }

        timer.setInterval(SENS_SUPPLY_CHECK_START_DELAY); // задержка как при старте
        break;

      default:
      break;
    }
  }

  data.sensors_supply_output_state = state;
}
//******************************************************************

//-------
void fnAlarms(MyData &data, Alarms &alarms){

  if(main_data.flag_system_started){

    if (alarms.sens_supply || alarms.resist_water_sensor || alarms.pj_water_sensor || alarms.temp_sensors)
    {
      alarms.common = true;
    }
    else
    {          
      alarms.common = false;
    }
  }

}
//**************************************************************************************


// Buzzer Process
void fnBuzzerProcess(MyData &data, Alarms &alarms){

  old_time = millis();

  static bool door_old_state = data.door_switch_state;
  static bool pump_out_old_state = data.pump_output_state;
  static bool conv_out_old_state = data.converter_output_state;
  static bool fridge_out_old_state = data.fridge_output_state;
  static bool common_alarm_old_state = alarms.common;

  if(SetpointsUnion.setpoints_data.buzzer_out_mode){
    

    if(door_old_state != data.door_switch_state && data.door_switch_state == true){

      if(data.water_level_liter < (SetpointsUnion.setpoints_data.water_tank_capacity/4)){
        tone(BUZZER,1000,1000);
      }
      else{
        tone(BUZZER,1000,50);
      }     
    }
    door_old_state = data.door_switch_state;
 
    if(pump_out_old_state != data.pump_output_state){
      tone(BUZZER,1000,200);
      pump_out_old_state = data.pump_output_state;
    }

    if(conv_out_old_state != data.converter_output_state){
      tone(BUZZER,1000,200);
      conv_out_old_state = data.converter_output_state;
    }

    if(fridge_out_old_state != data.fridge_output_state){
      tone(BUZZER,1000,200);
      fridge_out_old_state = data.fridge_output_state;
    }

    if(common_alarm_old_state != alarms.common && alarms.common == true){
      tone(BUZZER,5000,100);
    }
    common_alarm_old_state = alarms.common;

  }
  else {
    //noTone(BUZZER);
  }

  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
    Serial.print(F("BuzzerProcess time  "));
    Serial.print(millis()-old_time);
    Serial.println(F(" ms"));
  }

}
//********************************************************************************

//---- brightness -------------
void fnLcdBrightnessControl(MyData &data, SetpointsStruct &setpoints, GTimer &timer){

  if(data.door_switch_state){
    timer.setTimeout(setpoints.scrreen_off_delay * SECOND);
    analogWrite(LCD_BLA_OUT, 255-(setpoints.lcd_brightness * 2.5));
  }
  else {
    if(timer.isReady())analogWrite(LCD_BLA_OUT, 255);
  }
}
//********************************************************************************

void fnMenuProcess(void){
  
  /*------------ Menu -----------------*/
  old_time = millis();

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

        if(buttonUp.holded()){
          menu_mode = MENU_SETPOINTS;
          menu_current_item = 0;
          tone(BUZZER,500,200);
        }

        if(buttonEnter.clicked()){
          menu_mode = MENU_LOGO_VIEW;
          menu_current_item = 0;
        }
        break;

      case MENU_PARAM_VIEW:

        fnPrintMenuParamView();

        if (buttonUp.clicked() or buttonUp.holding()) {         // Если кнопку нажали или удерживают
          menu_current_item = constrain(menu_current_item - display_num_lines , 0, MENU_PARAM_VIEW_NUM_ITEMS - 1); // Двигаем указатель в пределах дисплея
          if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1)Serial.println(menu_current_item);
        }

        if (buttonDown.clicked() or buttonDown.holding()) {   
          menu_current_item = constrain(menu_current_item + display_num_lines, 0, MENU_PARAM_VIEW_NUM_ITEMS - 1); 
          if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1)Serial.println(menu_current_item);
        }

        if(buttonEnter.clicked()){
          menu_mode = MENU_MAIN_VIEW;
          menu_current_item = 0;
        }

        break;

      case MENU_SETPOINTS:

        printMenuSetpoints();

        if (buttonDown.clicked() or buttonDown.holding()) {         // Если кнопку нажали или удерживают
          menu_current_item = constrain(menu_current_item + 1, 0, MENU_SETPOINTS_NUM_ITEMS - 1); // Двигаем указатель в пределах дисплея
          Serial.println(menu_current_item);
        }

        if (buttonUp.clicked() or buttonUp.holding()) {   
          menu_current_item = constrain(menu_current_item - 1, 0, MENU_SETPOINTS_NUM_ITEMS - 1); 
          if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1)Serial.println(menu_current_item);
        }

        if(buttonEnter.clicked())menu_mode = MENU_SETPOINTS_EDIT_MODE;

        if(buttonEnter.holded()){
          menu_mode = MENU_MAIN_VIEW;
          menu_current_item = 0;
          tone(BUZZER,500,200);
        }

        break;

      case MENU_SETPOINTS_EDIT_MODE:

        printMenuSetpoints();

        if (buttonUp.clicked() or buttonUp.holding()){
          SetpointsUnion.SetpointsArray[menu_current_item] = constrain(SetpointsUnion.SetpointsArray[menu_current_item]+1,param_range_min[menu_current_item],param_range_max[menu_current_item]);
          Serial.println(SetpointsUnion.SetpointsArray[menu_current_item]);
        }

        if (buttonDown.clicked() or buttonDown.holding()){
          SetpointsUnion.SetpointsArray[menu_current_item] = constrain(SetpointsUnion.SetpointsArray[menu_current_item]-1,param_range_min[menu_current_item],param_range_max[menu_current_item]);
          if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1)Serial.println(SetpointsUnion.SetpointsArray[menu_current_item]);
        }

        if(buttonEnter.clicked()){
          EEPROM.updateBlock(EEPROM_SETPOINTS_ADDRESS, SetpointsUnion.setpoints_data);
          tone(BUZZER,500,200);
          if(SetpointsUnion.setpoints_data.debug_key== DEBUG_KEY_1)Serial.println(F("eeprom updated!"));
          menu_mode = MENU_SETPOINTS;
        }

        break;
      
      case MENU_LOGO_VIEW:

        switch (SetpointsUnion.setpoints_data.logo_selection)
        {
        case 0:
          u8g2.clearBuffer();
          u8g2.drawXBMP(33,5,64,55,FK_logo_64x55);
          u8g2.sendBuffer();
          break;

        case 1:
          u8g2.clearBuffer();
          //u8g2.drawXBM(1,1,128,64,dfwu_128x64);
          u8g2.drawXBMP(0,0,128,64,dfwu_128x64);
          u8g2.sendBuffer();
          break;

        case 2:
          u8g2.clearBuffer();
          u8g2.drawXBMP(41,0,46,64,Tryzub1_46x64);
          u8g2.sendBuffer();
          break;
        
        default:
          u8g2.clearBuffer();
          //пусто
          u8g2.sendBuffer();
          break;
        }
        

        if(buttonEnter.clicked()){
          menu_mode = MENU_PARAM_VIEW;
          menu_current_item = 0;
        }

        break;

      default:
      
      break;

    }

    if(!digitalRead(BUTTON_UP) || !digitalRead(BUTTON_DOWN) || !digitalRead(BUTTON_ENTER_ESC)){  //reset of screensaver timer 
      timerScreenSaver.setTimeout(SetpointsUnion.setpoints_data.screensaver_timeout * MINUTE);
    }

    if(menu_mode == MENU_MAIN_VIEW || menu_mode == MENU_PARAM_VIEW){   // go to screensaver
      if(timerScreenSaver.isReady()) menu_mode = MENU_LOGO_VIEW;    
    }

  }
  //end menu

  if(SetpointsUnion.setpoints_data.debug_key == DEBUG_KEY_1){
    Serial.print(F("MenuProcess time  "));
    Serial.print(millis()-old_time);
    Serial.println(F(" ms"));
  }
}

//***********************************************************************************

//---- Debug ------------

void fnDebugPrint(void){

  Serial.println(F(""));
  Serial.println(F("*************************** Inputs ********************************"));
  Serial.print(F("Door input state "));Serial.println(main_data.door_switch_state);
  Serial.print(F("Ignition input state "));Serial.println(main_data.ignition_switch_state);
  Serial.print(F("Proximity sensor input state "));Serial.println(main_data.proximity_sensor_state);
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.println(F("*************************** Outputs ********************************"));
  Serial.print(F("Pump output state "));Serial.println(main_data.pump_output_state);
  Serial.print(F("Converter output state "));Serial.println(main_data.converter_output_state);
  Serial.print(F("Fridge output state "));Serial.println(main_data.fridge_output_state);
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.println(F("*************************** Parameters ********************************"));
  Serial.print(F("Battery voltage "));Serial.println(main_data.battery_voltage);
  Serial.print(F("Sensors supply voltage "));Serial.println(main_data.sensors_supply_voltage);
  Serial.print(F("Inside temperature "));Serial.println(main_data.inside_temperature);
  Serial.print(F("Outside temperature "));Serial.println(main_data.outside_temperature);
  Serial.print(F("Fridge temperature "));Serial.println(main_data.fridge_temperature);
  Serial.print(F("Resestive sensor resistance "));Serial.println(main_data.res_sensor_resistance);
  Serial.print(F("Water level L "));Serial.println(main_data.water_level_liter);
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.println(F("*************************** Alarms ********************************"));
  Serial.print(F("Error PJ water sensor "));Serial.println(present_alarms.pj_water_sensor);
  Serial.print(F("Error Resestive water sensor "));Serial.println(present_alarms.resist_water_sensor);
  Serial.print(F("Error sensors supply "));Serial.println(present_alarms.sens_supply);
  Serial.print(F("Error temp sensors "));Serial.println(present_alarms.temp_sensors);
  Serial.println(F(""));
  Serial.println(F("********************************************************************"));

}