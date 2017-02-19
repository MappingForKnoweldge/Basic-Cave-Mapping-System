#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

#include <BLE_API.h>

#define TXRX_BUF_LEN                     20

//Pin For Nano
#ifdef BLE_NANO

//#define DIGITAL_OUT_PIN   		 D1
//#define DIGITAL_IN_PIN     		 D2
//#define PWM_PIN           	         D0
//#define SERVO_PIN        		 D3
//#define ANALOG_IN_PIN      		 A3

#endif

#define STATUS_CHECK_TIME                APP_TIMER_TICKS(200, 0)

/* Set the delay between fresh samples */
#define BNO055_SAMPLERATE_DELAY_MS (100)

BLEDevice  ble;

Adafruit_BNO055 bno = Adafruit_BNO055(55);

boolean bno_ok = true;

static app_timer_id_t                    m_status_check_id;
static boolean analog_enabled = false;
static byte old_state = LOW;
// The Nordic UART Service
static const uint8_t uart_base_uuid[]     = {0x71, 0x3D, 0, 0, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t uart_tx_uuid[]       = {0x71, 0x3D, 0, 3, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t uart_rx_uuid[]       = {0x71, 0x3D, 0, 2, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t uart_base_uuid_rev[] = {0x1E, 0x94, 0x8D, 0xF1, 0x48, 0x31, 0x94, 0xBA, 0x75, 0x4C, 0x3E, 0x50, 0, 0, 0x3D, 0x71};
uint8_t txPayload[TXRX_BUF_LEN] = {0,};
uint8_t rxPayload[TXRX_BUF_LEN] = {0,};

GattCharacteristic  txCharacteristic (uart_tx_uuid, txPayload, 1, TXRX_BUF_LEN,
                                      GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE);

GattCharacteristic  rxCharacteristic (uart_rx_uuid, rxPayload, 1, TXRX_BUF_LEN,
                                      GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

GattCharacteristic *uartChars[] = {&txCharacteristic, &rxCharacteristic};
GattService         uartService(uart_base_uuid, uartChars, sizeof(uartChars) / sizeof(GattCharacteristic *));

boolean flagDist = false;

void disconnectionCallback(void)
{
  ble.startAdvertising();
}

void onDataWritten(uint16_t charHandle)
{
  uint8_t buf[TXRX_BUF_LEN];
  uint16_t bytesRead;

  if (charHandle == txCharacteristic.getHandle())
  {
    ble.readCharacteristicValue(txCharacteristic.getHandle(), buf, &bytesRead);

    memset(txPayload, 0, TXRX_BUF_LEN);
    memcpy(txPayload, buf, TXRX_BUF_LEN);

    if (buf[0] == 0xA0) // Command is to enable analog in reading
    {
      if (buf[1] == 0x01) {
        // Switch ON the laser module
        Serial.print("O");
        analog_enabled = true;
      }
      else {
        // Request for switch OFF the module
        Serial.print("C");
        analog_enabled = false;
      }
    }
    else if (buf[0] == 0xA1) // Measure distance
    { 
      if (buf[1] == 0x01) {
        // Request for Distance
        Serial.print("D");
        // Request for switch OFF the module
        //Serial.print("C");
        flagDist = true;
      }
    }
  }
}

void m_status_check_handle(void * p_context)
{
  /* Get a new sensor event */
  sensors_event_t event;
  boolean getE = bno.getEvent(&event);

  float X = 10.0 * (float)event.orientation.x;
  float Y = 10.0 * (float)event.orientation.y;
  float Z = 10.0 * (float)event.orientation.z;

  uint8_t buf[9];
  //if (analog_enabled)  // if analog reading enabled
  if (true)  // if analog reading enabled
  {
    // Read and send out
    int16_t valX = 0;
    int16_t valY = 0;
    int16_t valZ = 0;
    if (bno_ok && getE) {
      valX = (int16_t)X;
      valY = (int16_t)Y;
      valZ = (int16_t)Z;
    } else {
      valX = 9999;
      valY = 9999;
      valZ = 9999;
    }
    buf[0] = (0x0B);
    buf[1] = (valX >> 8);
    buf[2] = (valX);
    buf[3] = (valY >> 8);
    buf[4] = (valY);
    buf[5] = (valZ >> 8);
    buf[6] = (valZ);
    buf[7] = 0;
    buf[8] = 0;
    ble.updateCharacteristicValue(rxCharacteristic.getHandle(), buf, 9);
  }
}

void setup(void)
{
  uint32_t err_code = NRF_SUCCESS;

  delay(1000);
  Serial.begin(19200); // comm with laser

  ble.init();
  ble.onDisconnection(disconnectionCallback);
  ble.onDataWritten(onDataWritten);

  // setup advertising
  ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED);
  ble.setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
  ble.accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                   (const uint8_t *)"Biscuit", sizeof("Biscuit") - 1);
  ble.accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS,
                                   (const uint8_t *)uart_base_uuid_rev, sizeof(uart_base_uuid));
  // 100ms; in multiples of 0.625ms.
  ble.setAdvertisingInterval(160);

  ble.addService(uartService);

  ble.startAdvertising();

  err_code = app_timer_create(&m_status_check_id, APP_TIMER_MODE_REPEATED, m_status_check_handle);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_start(m_status_check_id, STATUS_CHECK_TIME, NULL);
  APP_ERROR_CHECK(err_code);
  
  if (!bno.begin())
  //if (!bno.begin(Adafruit_BNO055::OPERATION_MODE_NDOF_FMC_OFF))
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    bno_ok = false;
  }
  delay(1000);
  bno.setExtCrystalUse(true);
}

void loop(void)
{
  char c;
  int i;
  float dist_meters_serial;
  char LEE_string[16];

  if (Serial.available() > 0) {
    i = 0;                                      // i is an indexer for the string storage variable and...
    c = 0;                                      // c holds the latest ASCII character from the SF02

    while (c != 10)                             // Read the ASCII string from the SF02 until a line feed character (\n) is detected
    {
      while (!Serial.available());              // Wait here for the next character
      c = Serial.read();                        // Fetch the character and store it in c
      LEE_string[i] = c;                        // Add the character to the existing string from the SF02
      i++;                                      // Point to the next character storage location in the string
    }                                           // Once the string has been captured...
    LEE_string[i - 2] = 0;                      // Create a null terminated string and remove the \r\n characters from the end

    if (flagDist) {
      String dStr(LEE_string);
      String distStr = dStr.substring(3, dStr.indexOf('m'));
      distStr.toCharArray(LEE_string, 16);
      dist_meters_serial = atof(LEE_string);
      
      float Z = 100.0 * dist_meters_serial;
  
      uint8_t buf[9];
  
      int16_t valZ = 0;
      valZ = (int16_t)Z;
  
      buf[0] = (0x0C);
      buf[1] = 0;
      buf[2] = 0;
      buf[3] = 0;
      buf[4] = 0;
      buf[5] = 0;
      buf[6] = 0;
      buf[7] = (valZ >> 8);
      buf[8] = (valZ);
      ble.updateCharacteristicValue(rxCharacteristic.getHandle(), buf, 9);
      
      flagDist = false;
    }
  }

  ble.waitForEvent();
}
