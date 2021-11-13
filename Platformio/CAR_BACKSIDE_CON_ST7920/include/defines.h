#ifndef DEFINES_H
#define DEFINES_H

#include <Arduino.h>

#define DEBUG_GENERAL   1
#define DEBUG_RTOS      0
#define DEBUG_ERROR_LOG 0




//timers
#define TEMP_SENSORS_UPDATE_PERIOD   1000   //ms
#define MENU_UPDATE_PERIOD           500    //ms
#define SENS_SUPPLY_CHECK_PERIOD     100    //ms
#define SENS_SUPPLY_CHECK_TIMES      5
#define SENS_SUPPLY_CHECK_MIN_V      4
#define SENS_SUPPLY_CHECK_START_DELAY 2000  //ms  


//modes
#define OFF_MODE               0
#define AUTO_MODE              1
#define ON_MODE                2

//water sensors types
#define WATER_FLOAT_SENSOR_PJ  0
#define WATER_RESISTIVE_SENSOR 1

//resistive sensor nominal
#define MIN_RESISTANCE          10
#define MAX_RESISTANCE          255

//eeprom
#define EEPROM_SETPOINTS_ADDRESS   0x00
#define EEPROM_1WIRE_ADDRESS       0x40
#define EEPROM_ERROR_LOG_ADDRES    0x80

#define EEPROM_KEY         0x12  //ключь для определения записаны ли уставки в память EEPROM или это первая запись


//communications
#define PJON_BUS_PIN         3   // new 
#define ONE_WIRE_PIN         4
#define DE_RS485_PIN         6   // in RS485.h

// water level sensor
#define WATER_LEVEL_LESS_THEN_25      1      
#define WATER_LEVEL_25                2
#define WATER_LEVEL_50                3
#define WATER_LEVEL_75                4
#define WATER_LEVEL_100               5
#define WATER_LEVEL_SENSOR_DEFECTIVE  6

// onewire
#define MAX_TEMP_SENSORS        3
#define INSIDE_SENSOR           1
#define OUTSIDE_SENSOR          2
#define FRIDGE_SENSOR           3

//inputs
#define DOOR_SWITCH_INPUT_1                 54       //A0
#define PROXIMITY_SENSOR_INPUT_2            55       //A1
#define IGNITION_SWITCH_INPUT_3             56       //A2 
#define LOW_WASHER_WATER_LEVEL_INPUT_4      57       //A3
#define SUPPLY_VOLTAGE_INPUT                A4       //A4
#define SENSORS_VOLTAGE_INPUT               A5       //A5
#define RESISTIVE_SENSOR                    A6       //A6
#define WATER_FLOW_SENSOR                    5       //YF-S201 (пока не используется)
#define BUTTON_ON_BOARD                     10       // кнопка на плате (пока не используется)
#define POWER_OK_FROM_ADM705                41       //
#define BUTTON_UP                           36
#define BUTTON_DOWN                         35
#define BUTTON_ENTER_ESC                    37
//#define ENCODER_SW							35
//#define ENCODER_DT							36
//#define ENCODER_CLK							37


//outputs
#define WATER_PUMP_OUTPUT_1      22  
#define FRIDGE_OUTPUT_2          23  
#define CONVERTER_OUTPUT_3       24  
#define SENSORS_SUPPLY_5v         7  
#define MAIN_SUPPLY_OUT			  9 
#define BUZZER                    2  
#define BUILTIN_LED              13
#define WDT_RESET_OUT             8 

#define LCD_BLA_OUT               44
#define LCD_CS                    53  
#define LCD_RESET                 43

//pjon swbb
#define PJON_MY_ID                    2
#define PJON_MAIN_CONTROLLER_ID       1
#define PJON_WATER_FLOAT_SENSOR_ID    3
#define PJON_WATER_FLOW_SENSOR_ID     4
#define PJON_MAX_NODES                2 // пока два


#define VERSION "ver 4.0"
#define DIVISION_RATIO_VOLTAGE_INPUT      0.0208      // разрешение 0.0025 уможить на коэфициент деления предусилителя 4.85(или делителя)
#define DIVISION_RATIO_SENS_SUPPLY_INPUT  0.0132     // разрешение 0.0025(для TL431) или 0.00256 (для внутреннего опорного 2.56в) уможить на коэфициент деления предусилителя 4.16
#define DIVISION_RATIO_RESIST_SENSOR      0.278      //0.278       //

#define MS_100        100           //  100ms
#define SECOND       1000           //ms  секунда
#define MINUTE       60000          //ms  минута
#define HOUR         3600000        //ms  час


#define START_DELAY                 3000  //ms задержка выполнения некоторых функций после подачи питания (пока датчики запустятся)
#define PRX_SENSOR_FEEDBACK_DELAY   500   //ms задержка реакции датчика приближения (для стабильности)
#define WDT_RESET_PERIOD            500000  // us период сброса ADM705 < 1.6 sec

//MENU
#define MENU_SETPOINTS_NUM_ITEMS        35    // кол-во пунктов меню настроек
#define MENU_PARAM_VIEW_NUM_ITEMS       20    // кол-во пунктов меню просмотра параметров
#define MENU_MAIN_VIEW                  0
#define MENU_PARAM_VIEW                 1
#define MENU_SETPOINTS                  2
#define MENU_SETPOINTS_EDIT_MODE        3


#define LCD_LINE_SPACER                 4
#define LCD_FONT_HIGHT                  8
#define PARAM_LENGHT                    4
#define ITEM_MAX_CHARS                  17

//water sensor types
#define PJON_SENSOR_TYPE               0
#define RES_SENSOR_TYPE                1

//water tank default capacity
#define WATER_TANK_CAPACITY             35

#endif