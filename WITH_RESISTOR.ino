#include <EEPROM.h>

const bool SERIAL_LOGGING_ENABLED = false;
const int IN_PORT = 0;
const int OUT_PORT_0 = 2;
const int OUT_PORTS_COUNT = 12;

class Logger {
  public:
    Logger(bool enabled) : enabled(enabled), initialized(false) {
    }

    void initialize() {
      if(enabled) {
        Serial.begin(9600);
        Serial.println("logging initialized");
        initialized = true;
      }
    }

    void log(char * message) {
      if(enabled && initialized) {
        Serial.println(message);
      }
    }

  private:
    bool enabled;
    bool initialized;
    
} LOGGER(SERIAL_LOGGING_ENABLED);

class LedMemory {
  public:    
    LedMemory(int offset) : offset(offset), bytesPerLed(BYTES_PER_LED) {
      LOGGER.log("memory obj created");
    }
  
    void storeByte(int port, int adjustedValue) {
      const int offset = mapPortToOffset(port);
      LOGGER.log("writing to EEPROM");
      EEPROM.write(offset, adjustedValue);
    }

    int loadByte(int port) {
      const int offset = mapPortToOffset(port);
      LOGGER.log("reading from EEPROM");
      return EEPROM.read(offset);
    }

  private:
    int mapPortToOffset(int port) {
      return offset + bytesPerLed * port;
    }
  
    int bytesPerLed;
    int offset;   
    
    int BYTES_PER_LED = 1;
};

class Led {
  public:
    Led(int ledPort, LedMemory * ledMemory) : ledPort(ledPort), resolution(RESOLUTION), memory(ledMemory) {
      LOGGER.log("LED obj created");
    }
    
    void initialize() {
      LOGGER.log("initializing led");
      const int storedValue = memory->loadByte(this->ledPort);
      pinMode(this->ledPort, OUTPUT);
      analogWrite(this->ledPort, storedValue);
      LOGGER.log("led initialized");
    }

    void setIntensity(float value) {
      LOGGER.log("setting led intensity");
      const int adjustedValue = max(min(value, 1), 0) * resolution;
      analogWrite(this->ledPort, adjustedValue);
      memory->storeByte(this->ledPort, adjustedValue);
    }

  private:
    int ledPort;  
    int resolution;
    LedMemory * memory;

    const int RESOLUTION = 255;
};

class ControlsListener {
  public:  
  virtual void knobUpdated(float value) = 0;
};

class Controls {
  public:
    Controls(int knobPin) : 
      knobPin(knobPin), initialized(false), listener(0), 
      jitterThreshold(JITTER_THRESHOLD), resolution(RESOLUTION) {
      LOGGER.log("controls obj created");
    }
  
    void update() {
      int valueRead = analogRead(this->knobPin);
      bool fireListeners = false;
      if(this->initialized) {
        if(abs(valueRead - this->knobValue) > this->jitterThreshold) {
          LOGGER.log("change in knob value detected");
          this->knobValue = valueRead;
          fireListeners = true;
        }
      } else {
        LOGGER.log("reading knob value first time");
        this->initialize();
        this->knobValue = valueRead;
        fireListeners = true;
      }
  
      if(fireListeners) {
        this->fireListeners();
      }
    }

    void setListener(ControlsListener * listener) {
      this->listener = listener;
    }

  protected:
    void initialize() {
      LOGGER.log("initializing controllers");
      this->initialized = true;
    }

    void fireListeners() {
      LOGGER.log("firing listeners");
      if(this->listener) {
        this->listener->knobUpdated(((float) this->knobValue) / this->resolution);
      }
    }

  private:
    bool initialized;
    int knobPin;
    int knobValue;
    int jitterThreshold;
    int resolution;
    ControlsListener * listener;
    
    const int JITTER_THRESHOLD = 4;
    const int RESOLUTION = 1023;
};

LedMemory * memory;
Led** ledArray;
Controls * controls;

class ControlsListenerImpl : public ControlsListener {
   
  virtual void knobUpdated(float value){
/************* ADD YOUR CODE HERE ;) *****************/
    
    for(int i = 0 ; i < OUT_PORTS_COUNT ; i += 1) {
      ledArray[i]->setIntensity(value);
    }

/*****************************************************/
  }

};

void setup() {
  LOGGER.initialize();
  LOGGER.log("target acquired");
  
  memory = new LedMemory(0);
  ledArray = new Led *[OUT_PORTS_COUNT];
  for(int i = 0 ; i < OUT_PORTS_COUNT ; i += 1) {
    ledArray[i] = new Led(OUT_PORT_0 + i, memory);
    ledArray[i]->initialize();
  }
  controls = new Controls(IN_PORT);
  controls->setListener(new ControlsListenerImpl());
}
 
void loop() {
  controls->update();
  delay(1);      
}
