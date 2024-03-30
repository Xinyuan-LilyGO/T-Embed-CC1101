#include <Wire.h>
#include "BQ25896.h"

BQ25896  battery_charging(Wire);
void setup() 
{
  Serial.begin(9600);
  Wire.begin();
  battery_charging.begin();
}

void loop() 
{

  battery_charging.properties();
  Serial.println("Battery Management System Parameter : \n===============================================================");
  Serial.print("VBUS : "); Serial.println(battery_charging.getVBUS());
  Serial.print("VSYS : "); Serial.println(battery_charging.getVSYS());
  Serial.print("VBAT : "); Serial.println(battery_charging.getVBAT());
  Serial.print("ICHG : "); Serial.println(battery_charging.getICHG(),4);
  Serial.print("TSPCT : "); Serial.println(battery_charging.getTSPCT());
  Serial.print("Temperature : "); Serial.println(battery_charging.getTemperature());
  
  Serial.print("FS_Current Limit : "); Serial.println(battery_charging.getFast_Charge_Current_Limit());
  Serial.print("IN_Current Limit : "); Serial.println(battery_charging.getInput_Current_Limit());
  Serial.print("PRE_CHG_Current Limit : "); Serial.println(battery_charging.getPreCharge_Current_Limit());
  Serial.print("TERM_Current Limit : "); Serial.println(battery_charging.getTermination_Current_Limit());

  Serial.print("Charging Status : "); Serial.println(battery_charging.getCHG_STATUS()==BQ25896::CHG_STAT::NOT_CHARGING?" not charging":
                                                          (battery_charging.getCHG_STATUS()==BQ25896::CHG_STAT::PRE_CHARGE ?" pre charging":
                                                          (battery_charging.getCHG_STATUS()==BQ25896::CHG_STAT::FAST_CHARGE?" Fast charging":"charging done"))); 
  
  Serial.print("VBUS Status : "); Serial.println(battery_charging.getVBUS_STATUS()==BQ25896::VBUS_STAT::NO_INPUT?" not input":
                                                          (battery_charging.getVBUS_STATUS()==BQ25896::VBUS_STAT::USB_HOST ?" USB host":
                                                          (battery_charging.getVBUS_STATUS()==BQ25896::VBUS_STAT::ADAPTER?" Adapter":"OTG")));
  
  Serial.print("VSYS Status : "); Serial.println(battery_charging.getVSYS_STATUS()==BQ25896::VSYS_STAT::IN_VSYSMIN?" In VSYSMIN regulation (BAT < VSYSMIN)":
                                                      "Not in VSYSMIN regulation (BAT > VSYSMIN)");
  
  Serial.print("Temperature rank : "); Serial.println(battery_charging.getTemp_Rank()==BQ25896::TS_RANK::NORMAL?" Normal":
                                                          (battery_charging.getTemp_Rank()==BQ25896::TS_RANK::WARM ?" Warm":
                                                          (battery_charging.getTemp_Rank()==BQ25896::TS_RANK::COOL?" Cool":
                                                          (battery_charging.getTemp_Rank()==BQ25896::TS_RANK::COLD?" Cold":"HOT"))));
  
  Serial.print("Charger fault status  : "); Serial.println(battery_charging.getCHG_Fault_STATUS()==BQ25896::CHG_FAULT::NORMAL?" Normal":
                                                          (battery_charging.getCHG_Fault_STATUS()==BQ25896::CHG_FAULT::INPUT_FAULT ?" Input Fault":
                                                          (battery_charging.getCHG_Fault_STATUS()==BQ25896::CHG_FAULT::THERMAL_SHUTDOWN?" Thermal Shutdown":"TIMER_EXPIRED")));
  delay(1000);
}