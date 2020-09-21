
#ifndef ESPStepperMotorServer_PositionSwitch_h
#define ESPStepperMotorServer_PositionSwitch_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPStepperMotorServer_Logger.h>
#include <ESPStepperMotorServer_MacroAction.h>

#define SWITCHTYPE_STATE_ACTIVE_HIGH_BIT 1
#define SWITCHTYPE_STATE_ACTIVE_LOW_BIT 2
#define SWITCHTYPE_LIMITSWITCH_POS_BEGIN_BIT 3
#define SWITCHTYPE_LIMITSWITCH_POS_END_BIT 4
#define SWITCHTYPE_POSITION_SWITCH_BIT 5
#define SWITCHTYPE_EMERGENCY_STOP_SWITCH_BIT 6
#define SWITCHTYPE_LIMITSWITCH_COMBINED_BEGIN_END_BIT 7

//size calculated using https://arduinojson.org/v6/assistant/
#define RESERVED_JSON_SIZE_ESPStepperMotorServer_PositionSwitch 170

class ESPStepperMotorServer_MacroAction;

class ESPStepperMotorServer_PositionSwitch
{
    friend class ESPStepperMotorServer;
public:
    ESPStepperMotorServer_PositionSwitch();
    ~ESPStepperMotorServer_PositionSwitch();
    ESPStepperMotorServer_PositionSwitch(byte ioPin, int stepperIndex, byte switchType, String name = "", long switchPosition = 0);

    /**
     * setter to set the id of this switch.
     * Only use this if you know what you are doing
     */
    void setId(byte id);

    /**
      * get the id of the switch
      */
    byte getId();

    int getStepperIndex(void);

    byte getIoPinNumber(void);
    byte getSwitchType(void);

    String getPositionName(void);
    void setPositionName(String name);

    bool isActiveHigh();
    bool isEmergencySwitch();
    bool isLimitSwitch();
    bool isTypeBitSet(byte bitToCheck);

    long getSwitchPosition(void);
    void setSwitchPosition(long position);

    void addMacroAction(ESPStepperMotorServer_MacroAction *macroAction);
    std::vector<ESPStepperMotorServer_MacroAction*> getMacroActions();
    bool hasMacroActions(void);
    void clearMacroActions(void);
    int serializeMacroActionsToJsonArray(JsonArray macroActionsJsonArray);

private:
    byte _stepperIndex;
    byte _switchIndex;
    byte _ioPinNumber = 255;
    byte _switchType = 0; //this is a bit mask representing the active state (bit 1 and 2) and the general type (homing/limit/position or emergency stop switch) in one byte
    String _positionName;
    long _switchPosition;
    ESPStepperMotorServer_Logger _logger;
    std::vector<ESPStepperMotorServer_MacroAction*> _macroActions;
};
#endif