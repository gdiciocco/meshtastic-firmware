#define I2C_SDA 22 // I2C pins for this board
#define I2C_SCL 23

#define LED_PIN 7      // If defined we will blink this LED
#define LED_STATE_ON 0 // State when LED is lit

#define USE_SX1262
#define LORA_SCK 19
#define LORA_MISO 20
#define LORA_MOSI 18
#define LORA_CS 0
#define LORA_RESET 21
#define SX126X_CS LORA_CS
#define SX126X_DIO1 1
#define SX126X_DIO2 2
#define SX126X_BUSY 16
#define SX126X_RESET LORA_RESET
#define SX126X_RXEN 17
#define SX126X_TXEN RADIOLIB_NC
#define SX126X_DIO2_AS_RF_SWITCH
#define SX126X_DIO3_TCXO_VOLTAGE 1.8
