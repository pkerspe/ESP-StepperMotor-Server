
#ifndef ESPStepperMotorServer_PositionSwitch_h
#define ESPStepperMotorServer_PositionSwitch_h

#include <Arduino.h>
#include <ESPStepperMotorServer_Logger.h>

#define SWITCHTYPE_STATE_ACTIVE_HIGH_BIT 1
#define SWITCHTYPE_STATE_ACTIVE_LOW_BIT 2
#define SWITCHTYPE_LIMITSWITCH_POS_BEGIN_BIT 3
#define SWITCHTYPE_LIMITSWITCH_POS_END_BIT 4
#define SWITCHTYPE_POSITION_SWITCH_BIT 5
#define SWITCHTYPE_EMERGENCY_STOP_SWITCH_BIT 6

class ESPStepperMotorServer_PositionSwitch
{
  public:
    ESPStepperMotorServer_PositionSwitch();
    ESPStepperMotorServer_PositionSwitch(byte ioPin, byte stepperIndex, byte switchType, String name = "");
    byte getStepperIndex(void);

    byte getIoPinNumber(void);
    byte getSwitchType(void);

    String getPositionName(void);
    void setPositionName(String name);

    bool isActiveHigh();
    bool isTypeBitSet(byte bitToCheck);

    long getSwitchPosition(void);
    void setSwitchPosition(long position);

  private:
    byte _stepperIndex;
    byte _ioPinNumber = 255;
    byte _switchType; //this is a bit mask representing the active state (bit 1 and 2) and the general type (homing/limit/position or emergency stop switch) in one byte
    String _positionName;
    long _switchPosition;
    ESPStepperMotorServer_Logger _logger;
};
#endif