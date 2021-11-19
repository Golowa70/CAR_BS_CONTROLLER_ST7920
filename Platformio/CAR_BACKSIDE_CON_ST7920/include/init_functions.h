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
    default_setpoints_data.pump_off_delay = 10;
    default_setpoints_data.pump_out_mode = AUTO_MODE;  // Режим(авто, вкл, выкл).
    default_setpoints_data.converter_U_off = 115;    // дробное со смещённой вправо точкой 12.7в = 127,  13.2в =132 и т.д.
    default_setpoints_data.converter_T_U_off = 10;  // задержка отключения по низкому напряжению U_off
    default_setpoints_data.converter_U_on = 127;     // напряжение включения (порог)

    default_setpoints_data.converter_T_U_on = 10;   // задержка включения по U_on
    default_setpoints_data.converter_T_IGN_off = 10; // задержка отключения после выключения зажигания
    default_setpoints_data.convertet_out_mode = AUTO_MODE;  // Режим(авто, вкл, выкл). 
    default_setpoints_data.fridge_U_off = 115;     // дробное со смещённой вправо точкой 12.7в = 127,  13.2в =132 и т.д.
    default_setpoints_data.fridge_T_U_off = 10;   // задержка отключения по низкому напряжению U_off

    default_setpoints_data.fridge_U_on = 127;      // напряжение включения (порог)
    default_setpoints_data.fridge_T_U_on = 10;    // задержка включения по U_on
    default_setpoints_data.fridge_T_IGN_off = 10; // задержка отключения после выключения зажигания
    default_setpoints_data.fridge_Temp_on = 10;   // Температура включения Temp_on.
    default_setpoints_data.fridge_Temp_off = 7;  // Температура выключения Temp_off.

    default_setpoints_data.fridge_out_mode = AUTO_MODE;  // Режим(авто, авто с термостатом, вкл, выкл).
    default_setpoints_data.resistive_sensor_correction = 0; // 0- нолевая коррекция
    default_setpoints_data.resistive_sensor_nominal = 190;    // 
    default_setpoints_data.water_sensor_type_selected = PJON_SENSOR_TYPE;
    default_setpoints_data.water_tank_capacity = WATER_TANK_CAPACITY;

    default_setpoints_data.mb_slave_ID = 2;
    default_setpoints_data.mb_baud_rate = MB_RATE_9600;
    default_setpoints_data.buzzer_out_mode = ON_MODE;
    default_setpoints_data.scrreen_off_delay = 10;  //
    default_setpoints_data.shutdown_delay = 2;

    default_setpoints_data.voltage_correction = 127; // 127 - нолевая коррекция
    default_setpoints_data.lcd_brightness = 100;
    default_setpoints_data.logo_selection = 0;
    default_setpoints_data.inside_sensor_id = 1;
    default_setpoints_data.outside_sensor_id = 2;

    default_setpoints_data.fridge_sensor_id = 3;
    default_setpoints_data.spare_1 = 0;
    default_setpoints_data.spare_2 = 0;
    default_setpoints_data.spare_3 = 0;
    default_setpoints_data.spare_4 = 0;
    default_setpoints_data.key = EEPROM_KEY;

  }

#endif