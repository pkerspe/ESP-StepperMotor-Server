#include <ESPStepperMotorServer_MacroAction.h>
#include "ESPStepperMotorServer.h"

ESPStepperMotorServer_MacroAction::ESPStepperMotorServer_MacroAction(MacroActionType actionType, int val1, long val2) {
    this->actionType = actionType;
    this->val1 = val1;
    this->val2 = val2;
}

bool ESPStepperMotorServer_MacroAction::execute(ESPStepperMotorServer *serverRef) {
    Serial.println("Execute called for MacroAction");
    switch (this->actionType) {
    case moveBy: {
        ESPStepperMotorServer_StepperConfiguration *stepper = serverRef->getCurrentServerConfiguration()->getStepperConfiguration(this->val1);
        if (stepper && stepper->getFlexyStepper()) {
            stepper->getFlexyStepper()->moveRelativeInSteps(this->val2);
        }
        break;
    }
    case MacroActionType::moveTo: {
        ESPStepperMotorServer_StepperConfiguration *stepper = serverRef->getCurrentServerConfiguration()->getStepperConfiguration(this->val1);
        if (stepper && stepper->getFlexyStepper()) {
            stepper->getFlexyStepper()->setTargetPositionInSteps(this->val2);
        }
        break;
    }
    case MacroActionType::releaseEmergencyStop: {
        serverRef->revokeEmergencyStop();
        break;
    }
    case MacroActionType::triggerEmergencyStop: {
        serverRef->performEmergencyStop();
        break;
    }
    case MacroActionType::setAcceleration: {
        ESPStepperMotorServer_StepperConfiguration *stepper = serverRef->getCurrentServerConfiguration()->getStepperConfiguration(this->val1);
        if (stepper && stepper->getFlexyStepper()) {
            stepper->getFlexyStepper()->setAccelerationInStepsPerSecondPerSecond((float)this->val2);
        }
        break;
    }
    case MacroActionType::setDeceleration: {
        ESPStepperMotorServer_StepperConfiguration *stepper = serverRef->getCurrentServerConfiguration()->getStepperConfiguration(this->val1);
        if (stepper && stepper->getFlexyStepper()) {
            stepper->getFlexyStepper()->setDecelerationInStepsPerSecondPerSecond((float)this->val2);
        }
        break;
    }
    case MacroActionType::setHome: {
        ESPStepperMotorServer_StepperConfiguration *stepper = serverRef->getCurrentServerConfiguration()->getStepperConfiguration(this->val1);
        if (stepper && stepper->getFlexyStepper()) {
            stepper->getFlexyStepper()->setCurrentPositionAsHomeAndStop();
        }
        break;
    }
    case MacroActionType::setLimitA: {
        ESPStepperMotorServer_StepperConfiguration *stepper = serverRef->getCurrentServerConfiguration()->getStepperConfiguration(this->val1);
        if (stepper && stepper->getFlexyStepper()) {
            stepper->getFlexyStepper()->setLimitSwitchActive(ESP_FlexyStepper::LIMIT_SWITCH_BEGIN);
        }
        break;
    }
    case MacroActionType::setLimitB: {
        ESPStepperMotorServer_StepperConfiguration *stepper = serverRef->getCurrentServerConfiguration()->getStepperConfiguration(this->val1);
        if (stepper && stepper->getFlexyStepper()) {
            stepper->getFlexyStepper()->setLimitSwitchActive(ESP_FlexyStepper::LIMIT_SWITCH_END);
        }
        break;
    }
    case MacroActionType::setSpeed: {
        ESPStepperMotorServer_StepperConfiguration *stepper = serverRef->getCurrentServerConfiguration()->getStepperConfiguration(this->val1);
        if (stepper && stepper->getFlexyStepper()) {
            stepper->getFlexyStepper()->setSpeedInStepsPerSecond(this->val2);
        }
        break;
    }
    case MacroActionType::setOutputHigh:
        digitalWrite(this->val1, HIGH);
        break;
    case MacroActionType::setOutputLow:
        digitalWrite(this->val1, LOW);
        break;
    default:
        break;
    }
    return true;
}

void ESPStepperMotorServer_MacroAction::addSerializedInstanceToJsonArray(JsonArray jsonArray) {
    JsonObject nestedMacroAction = jsonArray.createNestedObject();
    nestedMacroAction["type"] = this->actionType;
    nestedMacroAction["val1"] = this->val1;
    nestedMacroAction["val2"] = this->val2;
}

ESPStepperMotorServer_MacroAction *ESPStepperMotorServer_MacroAction::fromJsonObject(JsonObject macroActionJson) {
    int val1 = macroActionJson["val1"];
    long val2 = (long)macroActionJson["val2"];
    MacroActionType type = macroActionJson["type"];
    return new ESPStepperMotorServer_MacroAction(type, val1, val2
    );
}

MacroActionType ESPStepperMotorServer_MacroAction::getType(void) {
    return this->actionType;
}

int ESPStepperMotorServer_MacroAction::getVal1(void) {
    return this->val1;
}

long ESPStepperMotorServer_MacroAction::getVal2(void) {
    return this->val2;
}
