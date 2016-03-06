#include "LuminosityModule.h"
#include "ModuleController.h"

static uint8_t LAMP_RELAYS[] = { LAMP_RELAYS_PINS }; // объявляем массив пинов реле

BH1750Support::BH1750Support()
{

}
void BH1750Support::begin(BH1750Address addr, BH1750Mode mode)
{
  deviceAddress = addr;
  Wire.begin();
  writeByte(BH1750PowerOn); // включаем датчик
  ChangeMode(mode); 
}
void BH1750Support::ChangeMode(BH1750Mode mode) // смена режима работы
{
   currentMode = mode; // сохраняем текущий режим опроса
   writeByte((uint8_t)currentMode);
  _delay_ms(10);
}
void BH1750Support::ChangeAddress(BH1750Address newAddr)
{
  if(newAddr != deviceAddress) // только при смене адреса включаем датчик
  { 
    deviceAddress = newAddr;
    
    writeByte(BH1750PowerOn); // включаем датчик  
    ChangeMode(currentMode); // меняем режим опроса на текущий
  } // if
}
void BH1750Support::writeByte(uint8_t toWrite) 
{
  Wire.beginTransmission(deviceAddress);
  BH1750_WIRE_WRITE(toWrite);
  Wire.endTransmission();
}
long BH1750Support::GetCurrentLuminosity() 
{

  long curLuminosity = NO_LUMINOSITY_DATA;

  Wire.beginTransmission(deviceAddress); // начинаем опрос датчика освещенности
 if(Wire.requestFrom(deviceAddress, 2) == 2)// ждём два байта
 {
  // читаем два байта
  curLuminosity = BH1750_WIRE_READ();
  curLuminosity <<= 8;
  curLuminosity |= BH1750_WIRE_READ();
  curLuminosity = curLuminosity/1.2; // конвертируем в люксы
 }

  Wire.endTransmission();


  return curLuminosity;
}


void LuminosityModule::Setup()
{
  #if LIGHT_SENSORS_COUNT > 0
  lightMeter.begin(); // запускаем первый датчик освещенности
  State.AddState(StateLuminosity,0); // добавляем в состояние модуля флаг, что мы поддерживаем освещенность, и у нас есть датчик с индексов 0
  #endif

  #if LIGHT_SENSORS_COUNT > 1
  lightMeter2.begin(BH1750Address2);
  State.AddState(StateLuminosity,1); // добавляем в состояние модуля флаг, что мы поддерживаем освещенность, и у нас есть датчик с индексов 1
  #endif

  
  // настройка модуля тут
  settings = mainController->GetSettings();

  workMode = lightAutomatic; // автоматический режим работы
  bRelaysIsOn = false; // все реле выключены

  blinker.begin(DIODE_LIGHT_MANUAL_MODE_PIN,F("LX")); // настраиваем блинкер на нужный пин

  #ifdef SAVE_RELAY_STATES
   uint8_t relayCnt = LAMP_RELAYS_COUNT/8; // устанавливаем кол-во каналов реле
   if(LAMP_RELAYS_COUNT > 8 && LAMP_RELAYS_COUNT % 8)
    relayCnt++;

  if(LAMP_RELAYS_COUNT < 9)
    relayCnt = 1;
    
   for(uint8_t i=0;i<relayCnt;i++) // добавляем состояния реле (каждый канал - 8 реле)
    State.AddState(StateRelay,i);
  #endif  


 // выключаем все реле
  for(uint8_t i=0;i<LAMP_RELAYS_COUNT;i++)
  {
    pinMode(LAMP_RELAYS[i],OUTPUT);
    digitalWrite(LAMP_RELAYS[i],RELAY_OFF);

    #ifdef SAVE_RELAY_STATES
    uint8_t idx = i/8;
    uint8_t bitNum1 = i % 8;
    OneState* os = State.GetState(StateRelay,idx);
    if(os)
    {
      uint8_t curRelayStates = *((uint8_t*) os->Data);
      bitWrite(curRelayStates,bitNum1, bRelaysIsOn);
      State.UpdateState(StateRelay,idx,(void*)&curRelayStates);
    }
    #endif
  } // for
    
       
 }
void LuminosityModule::Update(uint16_t dt)
{ 
  // обновление модуля тут

    blinker.update();

 // обновляем состояние всех реле управления досветкой
  for(uint8_t i=0;i<LAMP_RELAYS_COUNT;i++)
  {
    digitalWrite(LAMP_RELAYS[i],bRelaysIsOn ? RELAY_ON : RELAY_OFF);

    #ifdef SAVE_RELAY_STATES
    uint8_t idx = i/8;
    uint8_t bitNum1 = i % 8;
    OneState* os = State.GetState(StateRelay,idx);
    if(os)
    {
      uint8_t curRelayStates = *((uint8_t*) os->Data);
      bitWrite(curRelayStates,bitNum1, bRelaysIsOn);
      State.UpdateState(StateRelay,idx,(void*)&curRelayStates);
    }
    #endif
    
  } // for 


  lastUpdateCall += dt;
  if(lastUpdateCall < LUMINOSITY_UPDATE_INTERVAL) // обновляем согласно настроенному интервалу
    return;
  else
    lastUpdateCall = 0;

  long lum = NO_LUMINOSITY_DATA;

  #if LIGHT_SENSORS_COUNT > 0
    lum = lightMeter.GetCurrentLuminosity();
    State.UpdateState(StateLuminosity,0,(void*)&lum);
  #endif
    
  #if LIGHT_SENSORS_COUNT > 1
    lum = lightMeter2.GetCurrentLuminosity();
    State.UpdateState(StateLuminosity,1,(void*)&lum);
  #endif   

}

bool  LuminosityModule::ExecCommand(const Command& command, bool wantAnswer)
{
  if(wantAnswer) 
    PublishSingleton = UNKNOWN_COMMAND;
  
  uint8_t argsCnt = command.GetArgsCount();
  
  if(command.GetType() == ctSET) 
  {
      if(wantAnswer) 
        PublishSingleton = PARAMS_MISSED;

      if(argsCnt > 0)
      {
         String s = command.GetArg(0);
         if(s == STATE_ON) // CTSET=LIGHT|ON
         {
          
          // попросили включить досветку
          if(command.IsInternal() // если команда пришла от другого модуля
          && workMode == lightManual)  // и мы в ручном режиме, то
          {
            // просто игнорируем команду, потому что нами управляют в ручном режиме
          } // if
           else
           {
              if(!command.IsInternal()) // пришла команда от пользователя,
              {
                workMode = lightManual; // переходим на ручной режим работы
                // мигаем светодиодом на 8 пине
                blinker.blink(WORK_MODE_BLINK_INTERVAL);
              }

            if(!bRelaysIsOn)
            {
              // значит - досветка была выключена и будет включена, надо записать в лог событие
              mainController->Log(this,s); 
            }

            bRelaysIsOn = true; // включаем реле досветки
            
            PublishSingleton.Status = true;
            if(wantAnswer) 
              PublishSingleton = STATE_ON;
            
           } // else
 
          
         } // STATE_ON
         else
         if(s == STATE_OFF) // CTSET=LIGHT|OFF
         {
          // попросили выключить досветку
          if(command.IsInternal() // если команда пришла от другого модуля
          && workMode == lightManual)  // и мы в ручном режиме, то
          {
            // просто игнорируем команду, потому что нами управляют в ручном режиме
           } // if
           else
           {
              if(!command.IsInternal()) // пришла команда от пользователя,
              {
                workMode = lightManual; // переходим на ручной режим работы
                // мигаем светодиодом на 8 пине
                blinker.blink(WORK_MODE_BLINK_INTERVAL);
              }

            if(bRelaysIsOn)
            {
              // значит - досветка была включена и будет выключена, надо записать в лог событие
              mainController->Log(this,s); 
            }

            bRelaysIsOn = false; // выключаем реле досветки
            
            PublishSingleton.Status = true;
            if(wantAnswer) 
              PublishSingleton = STATE_OFF;
            
           } // else
         } // STATE_OFF
         else
         if(s == WORK_MODE) // CTSET=LIGHT|MODE|AUTO, CTSET=LIGHT|MODE|MANUAL
         {
           // попросили установить режим работы
           if(argsCnt > 1)
           {
              s = command.GetArg(1);
              if(s == WM_MANUAL)
              {
                // попросили перейти в ручной режим работы
                workMode = lightManual; // переходим на ручной режим работы
                 // мигаем светодиодом на 8 пине
               blinker.blink(WORK_MODE_BLINK_INTERVAL);
              }
              else
              if(s == WM_AUTOMATIC)
              {
                // попросили перейти в автоматический режим работы
                workMode = lightAutomatic; // переходим на автоматический режим работы
                 // гасим диод на 8 пине
                blinker.blink();
              }

              PublishSingleton.Status = true;
              if(wantAnswer)
              {
                PublishSingleton = WORK_MODE; 
                PublishSingleton << PARAM_DELIMITER << (workMode == lightAutomatic ? WM_AUTOMATIC : WM_MANUAL);
              }
              
           } // if (argsCnt > 1)
         } // WORK_MODE
         
      } // if(argsCnt > 0)
  }
  else
  if(command.GetType() == ctGET) //получить данные
  {

    String t = command.GetRawArguments();
    t.toUpperCase();
    if(t == GetID()) // нет аргументов, попросили дать показания с датчика
    {
      
      PublishSingleton.Status = true;
      if(wantAnswer) 
      {
        PublishSingleton = 
     #if LIGHT_SENSORS_COUNT > 0
      (lightMeter.GetCurrentLuminosity());
      #else
        NO_LUMINOSITY_DATA;
      #endif
      
     PublishSingleton << PARAM_DELIMITER;
     PublishSingleton << 
      #if LIGHT_SENSORS_COUNT > 1
        (lightMeter2.GetCurrentLuminosity());
      #else
        NO_LUMINOSITY_DATA;
      #endif
      }
    }
    else
    if(argsCnt > 0)
    {
       String s = command.GetArg(0);
       if(s == WORK_MODE) // запросили режим работы
       {
          PublishSingleton.Status = true;
          if(wantAnswer) 
          {
            PublishSingleton = WORK_MODE;
            PublishSingleton << PARAM_DELIMITER << (workMode == lightAutomatic ? WM_AUTOMATIC : WM_MANUAL);
          }
          
       } // if(s == WORK_MODE)
       else
       if(s == LIGHT_STATE_COMMAND) // CTGET=LIGHT|STATE
       {
          if(wantAnswer)
          {
            PublishSingleton = LIGHT_STATE_COMMAND;
            PublishSingleton << PARAM_DELIMITER << (bRelaysIsOn ? STATE_ON : STATE_OFF) << PARAM_DELIMITER << (workMode == lightAutomatic ? WM_AUTOMATIC : WM_MANUAL);
          }
          
          PublishSingleton.Status = true;
             
       } // LIGHT_STATE_COMMAND
        
      // разбор других команд

      
    } // if(argsCnt > 0)
    
  } // if
 
 // отвечаем на команду
    mainController->Publish(this,command);
    
  return true;
}

