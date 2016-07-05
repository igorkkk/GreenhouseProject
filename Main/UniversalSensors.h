#ifndef _UNUVERSAL_SENSORS_H
#define _UNUVERSAL_SENSORS_H
#include <Arduino.h>
#include "ModuleController.h"
//-------------------------------------------------------------------------------------------------------------------------------------------------------
// команды
//-------------------------------------------------------------------------------------------------------------------------------------------------------
#define UNI_START_MEASURE 0x44 // запустить конвертацию
#define UNI_READ_SCRATCHPAD 0xBE // прочитать скратчпад
#define UNI_WRITE_SCRATCHPAD  0x4E // записать скратчпад
#define UNI_SAVE_EEPROM 0x25 // сохранить настройки в EEPROM

//-------------------------------------------------------------------------------------------------------------------------------------------------------
// максимальное кол-во датчиков в универсальном модуле
//-------------------------------------------------------------------------------------------------------------------------------------------------------
#define MAX_UNI_SENSORS 3
//-------------------------------------------------------------------------------------------------------------------------------------------------------
// значение, говорящее, что датчика нет
//-------------------------------------------------------------------------------------------------------------------------------------------------------
#define NO_SENSOR_REGISTERED 0xFF
//-------------------------------------------------------------------------------------------------------------------------------------------------------
// описание разных частей скратчпада
//-------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct
{
  byte packet_type; // тип пакета
  byte packet_subtype; // подтип пакета
  byte config; // конфигурация
  byte controller_id; // ID контроллера, к которому привязан модуль
  byte rf_id; // уникальный идентификатор модуля
  
} UniScratchpadHead; // голова скратчпада, общая для всех типов модулей
//-------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct
{
  UniScratchpadHead head; // голова
  byte data[24]; // сырые данные
  byte crc8; // контрольная сумма
  
} UniRawScratchpad; // "сырой" скратчпад, байты данных могут меняться в зависимости от типа модуля
//-------------------------------------------------------------------------------------------------------------------------------------------------------
typedef enum
{
  slotEmpty, // пустой слот, без настроек
  slotWindowLeftChannel, // настройки привязки к левому каналу одного окна
  slotWindowRightChannel, // настройки привязки к правому каналу одного окна
  slotWateringChannel, // настройки привязки к статусу канала полива 
  slotLightChannel, // настройки привязки к статусу канала досветки
  slotPin // настройки привязки к статусу пина
  
} UniSlotType; // тип слота, для которого указаны настройки
//-------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct
{
  byte slotType; // тип слота, одно из значений UniSlotType 
  byte slotLinkedData; // данные, привязанные к слоту мастером, должны хранится слейвом без изменений
  byte slotStatus; // статус слота (HIGH или LOW)
  
} UniSlotData; // данные одного слота настроек универсального модуля 
//-------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct
{
  UniSlotData slots[8]; // слоты настроек
  
} UniExecutionModuleScratchpad; // скратчпад исполнительного модуля
//-------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct
{
  byte index; // индекс датчика
  byte type; // тип датчика
  byte data[4]; // данные датчика
  
} UniSensorData; // данные с датчика универсального модуля
//-------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct
{
  byte battery_status;
  byte calibration_factor1;
  byte calibration_factor2;
  byte query_interval;
  byte reserved[2];
  UniSensorData sensors[MAX_UNI_SENSORS];
   
} UniSensorsScratchpad; // скратчпад модуля с датчиками
//-------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct
{
  byte sensorType; // тип датчика
  byte sensorData[2]; // данные датчика
   
} UniNextionData; // данные для отображения на Nextion
//-------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct
{
  uint8_t reserved[3]; // резерв, добитие до 24 байт
  uint8_t controllerStatus;
  uint8_t nextionStatus1;
  uint8_t nextionStatus2;  
  uint8_t openTemperature; // температура открытия окон
  uint8_t closeTemperature; // температура закрытия окон

  uint8_t dataCount; // кол-во записанных показаний с датчиков
  UniNextionData data[5]; // показания с датчиков
  
} UniNextionScratchpad; // скратчпад выносного модуля Nextion
//-------------------------------------------------------------------------------------------------------------------------------------------------------
// класс для работы со скратчпадом, представляет основные функции, никак не изменяет переданный скратчпад до вызова функции read и функции write.
//-------------------------------------------------------------------------------------------------------------------------------------------------------
class UniScratchpadClass
{
  public:
    UniScratchpadClass();

    void begin(byte pin,UniRawScratchpad* scratch); // привязываемся к пину и куску памяти, куда будем писать данные
    bool read(); // читаем скратчпад
    bool write(); // пишем скратчпад
    bool save(); // сохраняем скратчпад в EEPROM модуля
    bool startMeasure(); // запускаем конвертацию

  private:

    byte pin;
    UniRawScratchpad* scratchpad;

    bool canWork(); // проверяем, можем ли работать
  
};
//-------------------------------------------------------------------------------------------------------------------------------------------------------
/*

Все неиспользуемые поля инициализируются 0xFF

структура скратчпада модуля с датчиками:

packet_type - 1 байт, тип пакета (прописано значение 1)
packet_subtype - 1 байт, подтип пакета (прописано значение 0)
config - 1 байт, конфигурация (бит 0 - вкл/выкл передатчик, бит 1 - поддерживается ли фактор калибровки)
controller_id - 1 байт, идентификатор контроллера, к которому привязан модуль
rf_id - 1 байт, уникальный идентификатор модуля
battery_status - 1 байт, статус заряда батареи
calibration_factor1 - 1 байт, фактор калибровки
calibration_factor2 - 1 байт, фактор калибровки
query_interval - 1 байт, интервал обновления показаний (старшие 4 бита - минуты, младшие 4 бита - секунды)
reserved - 2 байт, резерв
index1 - 1 байт, индекс первого датчика в системе
type1 - 1 байт, тип первого датчика
data1 - 4 байта, данные первого датчика
index2 - 1 байт
type2 - 1 байт
data2 - 4 байта
index3 - 1 байт
type3 - 1 байт
data3 - 4 байта
crc8 - 1 байт, контрольная сумма скратчпада

*/
//-------------------------------------------------------------------------------------------------------------------------------------------------------
typedef enum
{
  uniNone = 0, // ничего нет
  uniTemp = 1, // только температура, значащие - два байта
  uniHumidity = 2, // влажность (первые два байта), температура (вторые два байта) 
  uniLuminosity = 3, // освещённость, 4 байта
  uniSoilMoisture = 4, // влажность почвы (два байта)
  uniPH = 5 // pH (два байта)
  
} UniSensorType; // тип датчика
//-------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct
{
 OneState* State1; // первое внутреннее состояние в контроллере
 OneState* State2; // второе внутреннее состояние в контроллере  
 
} UniSensorState; // состояние для датчика, максимум два (например, для влажности надо ещё и температуру тянуть)
//-------------------------------------------------------------------------------------------------------------------------------------------------------
typedef enum
{
  uniSensorsClient = 1, // packet_type == 1
  uniNextionClient = 2, // packet_type == 2
  uniExecutionClient = 3 // packet_type == 3
  
} UniClientType; // тип клиента
//-------------------------------------------------------------------------------------------------------------------------------------------------------
// класс поддержки регистрации универсальных датчиков в системе
//-------------------------------------------------------------------------------------------------------------------------------------------------------
class UniRegDispatcher 
 {
  public:
    UniRegDispatcher();

    void Setup(); // настраивает диспетчер регистрации перед работой

    // добавляет тип универсального датчика в систему, возвращает true, если добавилось (т.е. индекс был больше, чем есть в системе)
    bool AddUniSensor(UniSensorType type, uint8_t sensorIndex);

    // возвращает состояния для ранее зарегистрированного датчика
    bool GetRegisteredStates(UniSensorType type, uint8_t sensorIndex, UniSensorState& resultStates);

    // возвращает кол-во жёстко прописанных в прошивке датчиков того или иного типа
    uint8_t GetHardCodedSensorsCount(UniSensorType type); 
    // возвращает кол-во зарегистрированных универсальных модулей нужного типа
    uint8_t GetUniSensorsCount(UniSensorType type);

    uint8_t GetControllerID(); // возвращает уникальный ID контроллера

    void SaveState(); // сохраняет текущее состояние


 private:

    void ReadState(); // читает последнее запомненное состояние
    void RestoreState(); // восстанавливает последнее запомненное состояние


    // модули разного типа, для быстрого доступа к ним
    AbstractModule* temperatureModule; // модуль температуры
    AbstractModule* humidityModule; // модуль влажности
    AbstractModule* luminosityModule; // модуль освещенности
    AbstractModule* soilMoistureModule; // модуль влажности почвы
    AbstractModule* phModule; // модуль контроля pH

    // жёстко указанные в прошивке датчики
    uint8_t hardCodedTemperatureCount;
    uint8_t hardCodedHumidityCount;
    uint8_t hardCodedLuminosityCount;
    uint8_t hardCodedSoilMoistureCount;
    uint8_t hardCodedPHCount;


    // последние выданные индексы для универсальных датчиков
    uint8_t currentTemperatureCount;
    uint8_t currentHumidityCount;
    uint8_t currentLuminosityCount;
    uint8_t currentSoilMoistureCount;
  
 };
//-------------------------------------------------------------------------------------------------------------------------------------------------------
extern UniRegDispatcher UniDispatcher; // экземпляр класса диспетчера, доступный отовсюду
//-------------------------------------------------------------------------------------------------------------------------------------------------------
// абстрактный класс клиента, работающего с универсальным модулем по шине 1-Wire
//-------------------------------------------------------------------------------------------------------------------------------------------------------
class AbstractUniClient
{
    public:
      AbstractUniClient() {};

      // регистрирует модуль в системе
      virtual void Register(UniRawScratchpad* scratchpad) = 0; 

      // обновляет данные с модуля
      virtual void Update(UniRawScratchpad* scratchpad, bool isModuleOnline) = 0;

      void SetPin(byte p) { pin = p; }

   protected:
      byte pin;
  
};
//-------------------------------------------------------------------------------------------------------------------------------------------------------
// класс ничего не делающего клиента
//-------------------------------------------------------------------------------------------------------------------------------------------------------
class DummyUniClient : public AbstractUniClient
{
  public:
    DummyUniClient() : AbstractUniClient() {}
    virtual void Register(UniRawScratchpad* scratchpad) { UNUSED(scratchpad); }
    virtual void Update(UniRawScratchpad* scratchpad, bool isModuleOnline) { UNUSED(scratchpad); UNUSED(isModuleOnline); }
  
};
//-------------------------------------------------------------------------------------------------------------------------------------------------------
// класс клиента для работы с модулями датчиков
//-------------------------------------------------------------------------------------------------------------------------------------------------------
class SensorsUniClient : public AbstractUniClient
{
  public:
    SensorsUniClient();
    virtual void Register(UniRawScratchpad* scratchpad);
    virtual void Update(UniRawScratchpad* scratchpad, bool isModuleOnline);

  private:

    unsigned long measureTimer;
    void UpdateStateData(const UniSensorState& states,const UniSensorData* data,bool IsModuleOnline);
    void UpdateOneState(OneState* os, const UniSensorData* data, bool IsModuleOnline);
  
};
//-------------------------------------------------------------------------------------------------------------------------------------------------------
// класс клиента исполнительного модуля
//-------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef USE_UNI_EXECUTION_MODULE
class UniExecutionModuleClient  : public AbstractUniClient
{
  public:

    UniExecutionModuleClient();
    virtual void Register(UniRawScratchpad* scratchpad);
    virtual void Update(UniRawScratchpad* scratchpad, bool isModuleOnline);
  
};
#endif
//-------------------------------------------------------------------------------------------------------------------------------------------------------
struct UniNextionWaitScreenData
{
  byte sensorType;
  byte sensorIndex;
  const char* moduleName;
  UniNextionWaitScreenData(byte a, byte b, const char* c) 
  {
    sensorType = a;
    sensorIndex = b;
    moduleName = c;
  }
};
//-------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef USE_UNI_NEXTION_MODULE
//-------------------------------------------------------------------------------------------------------------------------------------------------------
class NextionUniClient : public AbstractUniClient
{
  public:
    NextionUniClient();
    virtual void Register(UniRawScratchpad* scratchpad);
    virtual void Update(UniRawScratchpad* scratchpad, bool isModuleOnline);

  private:

    unsigned long updateTimer;
    bool tempChanged;
  
};
//-------------------------------------------------------------------------------------------------------------------------------------------------------
#endif // USE_UNI_NEXTION_MODULE
//-------------------------------------------------------------------------------------------------------------------------------------------------------
// Фабрика клиентов
//-------------------------------------------------------------------------------------------------------------------------------------------------------
class UniClientsFactory
{
  private:

    DummyUniClient dummyClient;
    SensorsUniClient sensorsClient;
    #ifdef USE_UNI_NEXTION_MODULE
    NextionUniClient nextionClient;
    #endif

    #ifdef USE_UNI_EXECUTION_MODULE
    UniExecutionModuleClient executionClient;
    #endif
  
  public:
    UniClientsFactory();
    // возвращает клиента по типу пакета скратчпада
    AbstractUniClient* GetClient(UniRawScratchpad* scratchpad);
};
//-------------------------------------------------------------------------------------------------------------------------------------------------------
#if UNI_WIRED_MODULES_COUNT > 0
//-------------------------------------------------------------------------------------------------------------------------------------------------------
// класс опроса универсальных модулей, постоянно висящих на линии
//-------------------------------------------------------------------------------------------------------------------------------------------------------
 class UniPermanentLine
 {
  public:
    UniPermanentLine(uint8_t pinNumber);

    void Update(uint16_t dt);

  private:


    bool IsRegistered();

    AbstractUniClient* lastClient; // последний известный клиент
    byte pin;
    unsigned long timer; // таймер обновления
  
 };
//-------------------------------------------------------------------------------------------------------------------------------------------------------
#endif
//-------------------------------------------------------------------------------------------------------------------------------------------------------
// класс регистрации универсальных модулей в системе 
//-------------------------------------------------------------------------------------------------------------------------------------------------------
class UniRegistrationLine
{
  public:
    UniRegistrationLine(byte pin);

    bool IsModulePresent(); // проверяет, есть ли модуль на линии
    void CopyScratchpad(UniRawScratchpad* dest); // копирует скратчпад в другой

    bool SetScratchpadData(UniRawScratchpad* src); // копирует данные из переданного скратчпада во внутренний

    void Register(); // регистрирует универсальный модуль в системе

  private:

    bool IsSameScratchpadType(UniRawScratchpad* src); // тестирует, такой же тип скратчпада или нет

    // скратчпад модуля на линии, отдельный, т.к. регистрация у нас разнесена по времени с чтением скратчпада,
    // и поэтому мы не можем здесь использовать общий скратчпад/
    UniRawScratchpad scratchpad; 

    // пин, на котором мы висим
    byte pin;
  
};
//-------------------------------------------------------------------------------------------------------------------------------------------------------
#endif
