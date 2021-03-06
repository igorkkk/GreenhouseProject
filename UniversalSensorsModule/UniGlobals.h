#ifndef _UNI_GLOBALS_H
#define _UNI_GLOBALS_H
//----------------------------------------------------------------------------------------------------------------
#define NO_TEMPERATURE_DATA -128 // нет данных с датчика температуры или влажности
#define NO_LUMINOSITY_DATA - 1 // нет данных с датчика освещённости

#define UNUSED(expr) do { (void)(expr); } while (0)
//----------------------------------------------------------------------------------------------------------------
typedef struct
{
  byte Type; // тип датчика
  byte Pin; // пин, на котором висит датчик
  
} SensorSettings; // настройки датчиков, подключённых к модулю
//----------------------------------------------------------------------------------------------------------------
typedef enum
{
  mstNone, // ничего нету
  mstDS18B20, // температурный DS18B20
  mstBH1750, // цифровой освещённости BH1750
  mstSi7021, // цифровой влажности Si7021
  mstChinaSoilMoistureMeter, // китайский датчик влажности почвы
  mstDHT11, // датчик семейства DHT11
  mstDHT22, // датчик семейства DHT2x
  mstPHMeter, // датчик PH
  mstFrequencySoilMoistureMeter, // частотный датчик влажности почвы
  
} ModuleSensorType; // тип датчика, подключенного к модулю
//----------------------------------------------------------------------------------------------------------------
typedef enum
{
  ptSensorsData = 1, // данные с датчиков
  ptNextionDisplay = 2, // дисплей Nextion, подключённый по шине 1-Wire
  ptExecuteModule = 3 // исполнительный модуль 
  
} PacketTypes;
//----------------------------------------------------------------------------------------------------------------
//Структура передаваемая мастеру и обратно
//----------------------------------------------------------------------------------------------------------------
struct sensor
{
    byte index;
    byte type;
    byte data[4];
    
};
//----------------------------------------------------------------------------------------------------------------
typedef struct
{
    byte packet_type;
    byte packet_subtype;
    byte config;
    byte controller_id;
    byte rf_id;
    byte battery_status;
    byte calibration_factor1;
    byte calibration_factor2;
    byte query_interval_min;
    byte query_interval_sec;
    byte reserved;

    sensor sensor1,sensor2,sensor3;

    byte crc8;
} t_scratchpad;
//----------------------------------------------------------------------------------------------------------------
typedef enum
{
  uniNone = 0, // ничего нет
  uniTemp = 1, // только температура, значащие - два байта
  uniHumidity = 2, // влажность (первые два байта), температура (вторые два байта) 
  uniLuminosity = 3, // освещённость, 4 байта
  uniSoilMoisture = 4, // влажность почвы (два байта)
  uniPH = 5 // показания pH (два байта)
  
} UniSensorType; // тип датчика
//----------------------------------------------------------------------------------------------------------------
typedef struct
{
  byte samplesDone;
  bool inMeasure;
  unsigned long samplesTimer;
  unsigned long data;
  
} PHMeasure;
//----------------------------------------------------------------------------------------------------------------
#define PH_NUM_SAMPLES 10 // кол-во замеров
#define PH_SAMPLES_INTERVAL 20 // интервал между замерами
//----------------------------------------------------------------------------------------------------------------
#define MEASURE_MIN_TIME 1000 // через сколько минимум можно читать с датчиков после запуска конвертации
//----------------------------------------------------------------------------------------------------------------
enum {RS485FromMaster = 1, RS485FromSlave = 2};
enum {RS485ControllerStatePacket = 1, RS485SensorDataPacket = 2};
//----------------------------------------------------------------------------------------------------------------
typedef struct
{
  unsigned long WindowsState; // состояние каналов окон, 4 байта = 32 бита = 16 окон)
  byte WaterChannelsState; // состояние каналов полива, 1 байт, (8 каналов)
  byte LightChannelsState; // состояние каналов досветки, 1 байт (8 каналов)
  byte PinsState[16]; // состояние пинов, 16 байт, 128 пинов
  
} ControllerState; // состояние контроллера
//----------------------------------------------------------------------------------------------------------------
typedef struct
{
  byte header1;
  byte header2;

  byte direction; // направление: 1 - от меги, 2 - от слейва
  byte type; // тип: 1 - пакет исполнительного модуля, 2 - пакет модуля с датчиками

  byte data[sizeof(ControllerState)]; // N байт данных, для исполнительного модуля в этих данных содержится состояние контроллера
  // для модуля с датчиками: первый байт - тип датчика, 2 байт - его индекс в системе. В обратку модуль с датчиками должен заполнить показания (4 байта следом за индексом 
  // датчика в системе и отправить пакет назад, выставив direction и type.

  byte tail1;
  byte tail2;
  byte crc8;
  
} RS485Packet; // пакет, гоняющийся по RS-485 туда/сюда (21 байт)
//----------------------------------------------------------------------------------------------------------------
typedef struct
{
  int8_t Humidity;
  uint8_t HumidityDecimal;
  
  int8_t Temperature;
  uint8_t TemperatureDecimal;
  
} HumidityAnswer;
//----------------------------------------------------------------------------------------------------------------
#endif
