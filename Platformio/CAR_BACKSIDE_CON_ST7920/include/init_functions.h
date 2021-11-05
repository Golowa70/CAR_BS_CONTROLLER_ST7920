#ifndef INIT_FUNCTIONS_H
#define INIT_FUNCTIONS_H

#include <Arduino.h>
#include "defines.h"
#include "variables.h"


void fnIOInit(void) {

    //inputs declaration
        pinMode(DOOR_SWITCH_INPUT_1, INPUT_PULLUP);
        pinMode(PROXIMITY_SENSOR_INPUT_2, INPUT_PULLUP);
        pinMode(IGNITION_SWITCH_INPUT_3, INPUT);
        pinMode(LOW_WASHER_WATER_LEVEL_INPUT_4, INPUT_PULLUP);//
        pinMode(WATER_FLOW_SENSOR, INPUT_PULLUP);//
        pinMode(POWER_OK_FROM_ADM705, INPUT_PULLUP);//

        pinMode(BUTTON_ON_BOARD, INPUT_PULLUP);//
        pinMode(BUTTON_UP, INPUT_PULLUP);
        pinMode(BUTTON_DOWN, INPUT_PULLUP);
        pinMode(BUTTON_ENTER_ESC, INPUT_PULLUP);

        analogReference(INTERNAL2V56);      // внутренний исочник опорного напряжения 2.56в
        //analogReference(EXTERNAL);          // внешний исочник опорного напряжения 2.5в(TL431)

    //outputs declaration
        pinMode(WATER_PUMP_OUTPUT_1, OUTPUT);
        pinMode(FRIDGE_OUTPUT_2, OUTPUT);
        pinMode(CONVERTER_OUTPUT_3, OUTPUT);
        pinMode(SENSORS_SUPPLY_5v, OUTPUT);
        pinMode(MAIN_SUPPLY_OUT, OUTPUT);
        pinMode(BUZZER , OUTPUT);
        pinMode(BUILTIN_LED, OUTPUT); 
        pinMode(WDT_RESET_OUT, OUTPUT); 
        pinMode(DE_RS485_PIN, OUTPUT);
        pinMode(LCD_BLA_OUT, OUTPUT);

    //outputs start state
        digitalWrite(WATER_PUMP_OUTPUT_1, LOW);
        digitalWrite(FRIDGE_OUTPUT_2, LOW);
        digitalWrite(CONVERTER_OUTPUT_3, LOW);
        digitalWrite(SENSORS_SUPPLY_5v, LOW);
        digitalWrite(MAIN_SUPPLY_OUT, LOW);
        digitalWrite(BUZZER, LOW);
        digitalWrite(BUILTIN_LED, LOW);
        digitalWrite(WDT_RESET_OUT, LOW);
        analogWrite(LCD_BLA_OUT, 127);
}


void fnDefaultSetpointsInit(void){

  //setpoints_data.magic_key = MAGIC_KEY ;
    /*
  setpoints_data.pump_off_delay = 5; // сек;
  setpoints_data.pump_out_mode = AUTO_MODE ; // Режим(авто, вкл, выкл).

  setpoints_data.converter_U_off = 115;     // 11.5 вольт дробное со смещённой вправо точкой 12.7в = 127,  13.2в =132 и т.д.
  setpoints_data.converter_T_U_off = 3;     // min задержка отключения по низкому напряжению U_off
  setpoints_data.converter_U_on  = 130;     //13 вольт напряжение включения (порог)
  setpoints_data.converter_T_U_on = 5;          // задержка включения по U_on
  setpoints_data.converter_T_IGN_off = 50;  // min задержка отключения после выключения зажигания
  setpoints_data.convertet_out_mode = AUTO_MODE ;  // Режим(авто, вкл, выкл).
  
  setpoints_data.fridge_U_off = 115;     // дробное со смещённой вправо точкой 12.7в = 127,  13.2в =132 и т.д.
  setpoints_data.fridge_T_U_off = 1;   // задержка отключения по низкому напряжению U_off
  setpoints_data.fridge_U_on = 127;      // напряжение включения (порог)
  setpoints_data.fridge_T_U_on = 3;    // задержка включения по U_on
  setpoints_data.fridge_T_IGN_off = 60; // задержка отключения после выключения зажигания
  setpoints_data.fridge_Temp_on = 10;   // Температура включения Temp_on.
  setpoints_data.fridge_Temp_off = 7;  // Температура выключения Temp_off.
  setpoints_data.fridge_out_mode = AUTO_MODE;  // Режим(авто, авто с термостатом, вкл, выкл).

  setpoints_data.resistive_sensor_correction = 127; // 0- нолевая коррекция
  setpoints_data.resistive_sensor_nominal = 192;   // 

  setpoints_data.water_sensor_type_selected  = WATER_RESISTIVE_SENSOR;;
  setpoints_data.water_tank_capacity = 35;  // литр;
  setpoints_data.water_level_liter_memory = 0; // актуально только с датчиком протечки

  setpoints_data.mb_slave_ID = 2;
  setpoints_data.mb_baud_rate = 2; //1-9600, 2-19200, 3-38400 ...

  setpoints_data.buzzer_out_mode = ON_MODE;
  setpoints_data.scrreen_off_delay = 5;        // 5 секунд 
  setpoints_data.shutdown_delay = 1;           // час
 
  setpoints_data.voltage_correction = 96;    //127 - нолевая коррекция
  setpoints_data.lcd_brightness = 90;
  setpoints_data.logo_selection = 0;
  
  setpoints_data.num_founded_temp_sensors = 0;     // 
  
  //setpoints_data.sensors_select_array[INSIDE_SENSOR - 1]; // ???
  //setpoints_data.sensors_select_array[OUTSIDE_SENSOR - 1];
  //setpoints_data.sensors_select_array[FRIDGE_SENSOR - 1]; 
    
 */

}

#endif