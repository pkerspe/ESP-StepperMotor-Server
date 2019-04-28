
#ifndef ESPStepperMotorServer_PositionSwitch_h
#define ESPStepperMotorServer_PositionSwitch_h

#include <Arduino.h>
#include <ESPStepperMotorServer_Logger.h>

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

    long getSwitchPosition(void);
    void setSwitchPosition(long position);

  private:
    byte _stepperIndex;
    byte _ioPinNumber = 255;
    byte _switchType;
    String _positionName;
    long _switchPosition;
    ESPStepperMotorServer_Logger _logger;
};
#endif