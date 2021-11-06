#ifndef VARIABLES_H
#define VARIABLES_H

//*********** Pjon variables ********************************************************
uint16_t receive_from_ID;                //   идентификатор устройства от которого пришли данные
uint16_t pjon_TX_water_sensor_response; //  результат передачи PJON
uint16_t pjon_RX_response;               //  результат приёма PJON
uint8_t pjon_sensor_fault_cnt;     // 

bool flag_pjon_water_sensor_connected;
bool flag_pjon_water_sensor_connected_old_state;

//pjon receive from water level sensor 
struct PjonReceive
{ // структура принятых данных от датчиков уровня воды по протоколу PJON
  uint8_t value;
} pjon_sensor_receive_data;

//pjon transmitt
/*
  struct PjonSend {                     // структура для отправки данных по протоколу PJON
    
    } data_send_to_main_controller;
  */

//*****************************************************************************************************


//*********** Setpoints variables *********************************************************************

struct SetpointsStruct { // структура для уставок

  uint8_t pump_off_delay;
  uint8_t pump_out_mode;  // Режим(авто, вкл, выкл).

  uint8_t converter_U_off;    // дробное со смещённой вправо точкой 12.7в = 127,  13.2в =132 и т.д.
  uint8_t converter_T_U_off;  // задержка отключения по низкому напряжению U_off
  uint8_t converter_U_on;     // напряжение включения (порог)
  uint8_t converter_T_U_on;   // задержка включения по U_on
  uint8_t converter_T_IGN_off; // задержка отключения после выключения зажигания
  uint8_t convertet_out_mode;  // Режим(авто, вкл, выкл).
  
  uint8_t fridge_U_off;     // дробное со смещённой вправо точкой 12.7в = 127,  13.2в =132 и т.д.
  uint8_t fridge_T_U_off;   // задержка отключения по низкому напряжению U_off
  uint8_t fridge_U_on;      // напряжение включения (порог)
  uint8_t fridge_T_U_on;    // задержка включения по U_on
  uint8_t fridge_T_IGN_off; // задержка отключения после выключения зажигания
  uint8_t fridge_Temp_on;   // Температура включения Temp_on.
  uint8_t fridge_Temp_off;  // Температура выключения Temp_off.
  uint8_t fridge_out_mode;  // Режим(авто, авто с термостатом, вкл, выкл).

  uint8_t resistive_sensor_correction; // 0- нолевая коррекция
  uint8_t resistive_sensor_nominal;    // 

  uint8_t water_sensor_type_selected;
  uint8_t water_tank_capacity;

  uint8_t mb_slave_ID;
  uint8_t mb_baud_rate;

  uint8_t buzzer_out_mode;
  uint8_t scrreen_off_delay;  //
  uint8_t shutdown_delay;

  uint8_t voltage_correction; // 127 - нолевая коррекция
  uint8_t lcd_brightness;
  uint8_t logo_selection;

  uint8_t inside_sensor_id;
  uint8_t outside_sensor_id;
  uint8_t fridge_sensor_id;
  uint8_t spare_1;
  uint8_t spare_2;
  uint8_t spare_3;
  uint8_t spare_4;
  //uint8_t water_level_liter_memory;
  
} default_setpoints_data, old_setpoints_data;

struct {

  uint8_t sensors_ID_array[MAX_TEMP_SENSORS][8];
  uint8_t num_founded_temp_sensors; //   
    
}temp_sensors_data;


union 
{ 
  
  SetpointsStruct setpoints_data;
  uint8_t SetpointsArray[35];

}SetpointsUnion;
// 35 byte
//********** end setpoints variables ******************************************************************

//*********** Main data *******************************************************************************
struct MyData
{
  float battery_voltage;          // напряжени бортсети ( например 124 это 12.4в)
  float outside_temperature;      //  наружная температура
  float inside_temperature;       // температура внутри
  float fridge_temperature;        // температура третьего датчика(пока не используется)
  float sensors_supply_voltage;   // напряжение питания датчиков 5в
  float res_sensor_resistance;    // сопротивление резистивного датчика
  
  uint8_t water_level_percent;    // уровень воды в процентах
  uint8_t water_level_liter;      // уровень воды в литрах

  bool door_switch_state;         // состояние концевика задней двери
  bool proximity_sensor_state;    // состояние датчика приближения
  bool ignition_switch_state;     // состояние входа зажигания
  bool converter_output_state;    // состояние выхода управления инвертором 12/220в
  bool fridge_output_state;        //состояние выхода освещения
  bool pump_output_state;         //состояние выхода насоса
  bool sensors_supply_output_state; // состояние выхода управления питанием сенсоров 5в
  bool main_supply_output_state;  // состояние выхода управления общим питанием 7.5в
  bool wdt_reset_output_state;    //состояние выхода сброса внешнего WDT
  bool screen_sleep_mode;         // флаг спящего режима экрана Nextion
  bool low_washer_water_level;    // низкий уровень воды в бачке омывателя
  bool flag_system_started;
  uint16_t mb_rates[6] = {4800, 7200, 9600, 19200, 38400, 57600};   // скорость связи по протоколу ModBus
} main_data;

//****** end main data **************************************************************

//********** MENU VARIABLES *********************************************************

  // Битмап с картинкой стрелочки (Если нужен)
  const uint8_t ptr_bmp[] PROGMEM = {
    0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0xFF, 0xFF, 0x7E, 0x3C, 0x18,
  };

  //
  const char p0[] PROGMEM = " Battery V:"; 
  const char p1[] PROGMEM = " Water level:";
  const char p2[] PROGMEM = " Temp outside:";
  const char p3[] PROGMEM = " Temp inside:";
  const char p4[] PROGMEM = " Temp fridge:"; 
  const char p5[] PROGMEM = " Sensors V:";
  const char p6[] PROGMEM = " Door:";
  const char p7[] PROGMEM = " Prx sensor:";
  const char p8[] PROGMEM = " IGN";
  const char p9[] PROGMEM = " LWW level:";
  const char p10[] PROGMEM = " Conv relay:";
  const char p11[] PROGMEM = " Fridge relay:";
  const char p12[] PROGMEM = " Pump relay:";
  const char p13[] PROGMEM = " Spare:";
  const char p14[] PROGMEM = " Spare:";
  const char p15[] PROGMEM = " 1Wire error :";
  const char p16[] PROGMEM = " Water sens error:";
  const char p17[] PROGMEM = " Sens supply error:";
  const char p18[] PROGMEM = " Spare:";
  const char p19[] PROGMEM = " Spare:";
  
  

  const char* const parameters_names[] PROGMEM =           
  {
    p0,p1,p2,p3,p4,
    p5,p6,p7,p8,p9,
    p10,p11,p12,p13,p14,
    p15,p16,p17,p18,p19,
  };
  
  

  /* массивы строк с именами пунктов меню настроек */
  const char i0[] PROGMEM = " Pump off delay:";  
  const char i1[] PROGMEM = " Pump out mode:";
  const char i2[] PROGMEM = " Conv U off:";
  const char i3[] PROGMEM = " Conv T U off:";
  const char i4[] PROGMEM = " Conv U on:";
  const char i5[] PROGMEM = " Conv T U on:";
  const char i6[] PROGMEM = " Conv T IGN off:";
  const char i7[] PROGMEM = " Conv out mode:";
  const char i8[] PROGMEM = " Fridge U off:";
  const char i9[] PROGMEM = " Fridge T U off:";
  const char i10[] PROGMEM = " Fridge U on:";
  const char i11[] PROGMEM = " Fridge T U on:";
  const char i12[] PROGMEM = " Fridge T IGN off:";
  const char i13[] PROGMEM = " Fridge temp on:";
  const char i14[] PROGMEM = " Fridge temp off:";
  const char i15[] PROGMEM = " Fridge out mode:";
  const char i16[] PROGMEM = " Res sens corr:";
  const char i17[] PROGMEM = " Res sens nom:";
  const char i18[] PROGMEM = " Water sens type:";
  const char i19[] PROGMEM = " Water tank cap:";
  const char i20[] PROGMEM = " MB slave ID:";
  const char i21[] PROGMEM = " MB baud rate:";
  const char i22[] PROGMEM = " Buzzer out mode:";
  const char i23[] PROGMEM = " Screen off delay:";
  const char i24[] PROGMEM = " Shutdown delay:";
  const char i25[] PROGMEM = " U correction:";
  const char i26[] PROGMEM = " Brightness:";
  const char i27[] PROGMEM = " Logo:";
  const char i28[] PROGMEM = " Inside sid:";
  const char i29[] PROGMEM = " Outside sid:";
  const char i30[] PROGMEM = " Fridge sid:";
  const char i31[] PROGMEM = " Parametr 32:";
  const char i32[] PROGMEM = " Parametr 33:";
  const char i33[] PROGMEM = " Parametr 34:";
  const char i34[] PROGMEM = " Parametr 35:";
  
  /*Массив ссылок на имена пунктов меню, обращение к названию пунктов по их номеру*/
  const char* const setpoints_menu_names[] PROGMEM =           
  {
    i0, i1, i2, i3, i4, 
    i5, i6, i7, i8, i9,
    i10, i11, i12, i13, i14,
    i15, i16, i17, i18, i19,
    i20, i21, i22, i23, i24, 
    i25, i26, i27, i28, i29, 
    i30, i31, i32, i33, i34,
  };

  //Массив минимальных значений параметров
  uint8_t param_range_min[MENU_SETPOINTS_NUM_ITEMS]  =
  {
    3,0,5,1,5,
    1,0,0,5,1,
    5,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
  };

  //Массив максимальных значений параметров
  uint8_t param_range_max[MENU_SETPOINTS_NUM_ITEMS]  =
  {
   20,3,13,255,255,
   255,255,3,255,255,
   255,255,255,255,255,
   3,255,255,255,255,
   255,255,255,255,255,
   255,255,255,255,255,
   255,255,255,255,255,
  };
  
  int8_t menu_pointer = 0; // Переменная указатель на текущий пункт меню
  uint8_t menu_mode = MENU_SETPOINTS;
  bool flag_blink = false;

  uint8_t menu_current_item;
  uint8_t menu_current_page = 0;
  
//***********************************************************************************

//********** ONE WIRE VARIABLES *****************************************************
  bool flag_ow_scanned;
  bool flag_ow_scan_to_start;
  bool flag_ds18b20_update; // флаг обновлени чтения температуры с ds18b20
//***********************************************************************************

//****** DISPLAY VARIABLES ********************************************************
uint8_t display_height;
uint8_t display_width;
uint8_t display_num_lines;

//********* OTHER VARIABLES *********************************************************
bool flag_door_switch_old_state; // предыдущее состояние двери
bool proximity_sensor_old_state; // предыдущее состояние датчика приближения

bool flag_convOff_due_voltage;    // флаг что конветер был выключен по напряжению
bool flag_convOff_due_ign_switch; // флаг что конветер был выключен по таймеру после выключения зажигания

uint8_t inputs_undebounced_sample = 0;
uint8_t inputs_debounced_state = 0;

//***********************************************************************************

//********* ERROR LOG ***************************************************************
struct ErrLog
{
  uint16_t sens_supply_error_cnt;      // счетчик ошбок питания сенсоров
  uint16_t pj_water_sensor_error_cnt;  // счетчик запросов без ответа датчика уровня
  uint16_t temp_sensors_error_cnt;     // счетчик ошибок шины 1Wire
  uint16_t resist_sensor_error_cnt;    // счетчик ошибок 
} ErrorLog;

struct Alarms
{
  bool sens_supply;
  bool pj_water_sensor;
  bool temp_sensors;
  bool resist_sensor;
  bool common;
} present_alarms,old_alarms;



static unsigned char water_level_50x50[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x00, 0x00, 0x00,
   0x00, 0x00, 0xc0, 0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0,
   0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xc0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x00,
   0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0,
   0x07, 0x00, 0x00, 0x00, 0xc0, 0x0f, 0xc0, 0x07, 0xe0, 0x0f, 0x00, 0xc0,
   0x1f, 0xd8, 0x37, 0xf0, 0x0f, 0x00, 0x00, 0x30, 0xcc, 0x37, 0x18, 0x00,
   0x00, 0x00, 0xe0, 0xcf, 0xe7, 0x0e, 0x00, 0x00, 0x00, 0xc0, 0xc3, 0xc7,
   0x07, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xc0, 0x07, 0x00, 0x00, 0x00, 0xc0, 0x0f, 0xc0, 0x07, 0xe0, 0x0f, 0x00,
   0xc0, 0x1f, 0xd8, 0x37, 0xf0, 0x07, 0x00, 0x00, 0x30, 0xcc, 0x37, 0x18,
   0x00, 0x00, 0x00, 0xe0, 0xcf, 0xe7, 0x0f, 0x00, 0x00, 0x00, 0xc0, 0xc3,
   0x87, 0x07, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xe0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xf0, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x1f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xe0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00 };

#endif
