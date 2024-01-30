#ifndef ESPStepperMotorServer_MacroAction_h
#define ESPStepperMotorServer_MacroAction_h

#include <ArduinoJson.h>
#include <ESP_FlexyStepper.h>

class ESPStepperMotorServer;

enum MacroActionType {
    moveTo, moveBy, setSpeed, setAcceleration, setDeceleration, setHome, setLimitA, setLimitB, setOutputHigh, setOutputLow, triggerEmergencyStop, releaseEmergencyStop
};

class ESPStepperMotorServer_MacroAction
{
public:
    ESPStepperMotorServer_MacroAction(MacroActionType actionType, int val1, long val2 = 0);
    bool execute(ESPStepperMotorServer *serverRef);
    void addSerializedInstanceToJsonArray(JsonArray jsonArray);
    static ESPStepperMotorServer_MacroAction * fromJsonObject(JsonObject macroActionJson);
    MacroActionType getType(void);
    int getVal1(void);
    long getVal2(void);
private:
    MacroActionType actionType;
    int val1 = 0;
    long val2 = 0;
};
#endif
