//ПРОГРАММА УПРАВЛЕНИЯ ЩИТОВ ЩКУ СИСТЕМ АГС
//АО "НИУИФ", ОКиС, ГЭМАА 2021
//-------------------------------
//подключение основных библиотек
#include <SPI.h>
#include <Controllino.h>
//задание направления хода!
//0-нормальное,1-реверсивное
#define MOVEDIRECTION 1
//включение режима автоперезапуска отбора на время проведения ОПИ (0 - выкл)
#define TEST_MODE 0
//определение всех портов(не изменять)
#if MOVEDIRECTION == 0
#define S1_STARTPOSITION_ENDSTOP CONTROLLINO_A2
#define S2_ENDPOSITION_ENDSTOP CONTROLLINO_A3
#define RELAY_K1_FORWARD CONTROLLINO_D0
#define RELAY_K2_REVERSE CONTROLLINO_D1
#else
#define S1_STARTPOSITION_ENDSTOP CONTROLLINO_A3
#define S2_ENDPOSITION_ENDSTOP CONTROLLINO_A2
#define RELAY_K1_FORWARD CONTROLLINO_D1
#define RELAY_K2_REVERSE CONTROLLINO_D0
#endif
#define STOP_BUTTON CONTROLLINO_IN0
#define START_BUTTON CONTROLLINO_A0
#define MANUAL_OPERATION_BUTTON CONTROLLINO_A1
#define RELAY_K3_STOP_LAMP CONTROLLINO_D2
#define RELAY_K4_NORMAL_WORK_LAMP CONTROLLINO_D3
#define ORGS_OUTPUT CONTROLLINO_D5
//настройка таймера рестарта пробоотбора (для тестового режима)!
int32_t TEST_RESTART_TIME = 180;
//настройка таймингов пробоотбора!
//время ожидания в конечном положении
int32_t PROBE_GRAB_TIME = 5;
//время между циклами отбора
int32_t CYCLE_TIME = 420;
//задание триггеров и переменных (не изменять)
int MOB_STATE = 0;
int LAST_MOB_STATE = 0;
uint32_t setTimestamp = 0;
uint32_t checkTimestamp = 0;
volatile boolean START_TRIGGER = false;
volatile int BOTTOM_REACHED = LOW;
volatile int TOP_REACHED = LOW;
boolean REVERSE_TRIGGER = true;
boolean FORWARD_TRIGGER = false;
boolean PROGRAM_FIRST_STARTED = true;
boolean CYCLE_FINISHED = false;
boolean PREVENT_RESTART = true;
boolean START_ONCE = true;
boolean SET_PREV_GRAB_TIME = true;
boolean START_WAIT_SEQUENCE = false;
boolean START_GRAB_SEQUENCE = false;
boolean WAIT_DONE = false;
boolean PRINT_ONCE = true;
//задание промежуточных переменных времени (не изменять)
int32_t start_time = 0;
int32_t end_time = 0;
int32_t total_cycle_time = 0;
int32_t time_to_wait = 0;
int32_t wait_cur = 0;
int32_t grab_time_cur = 0;
int32_t grab_time_prev = 0;
int32_t check_time = 0;
//конфигурация портов и интерфейсов (не изменять)
void setup()
{
  Serial.begin(9600);
  pinMode(MANUAL_OPERATION_BUTTON, INPUT);
  pinMode(S1_STARTPOSITION_ENDSTOP, INPUT);
  pinMode(S2_ENDPOSITION_ENDSTOP, INPUT);
  pinMode(STOP_BUTTON, INPUT);
  pinMode(RELAY_K1_FORWARD, OUTPUT);
  pinMode(RELAY_K2_REVERSE, OUTPUT);
  pinMode(RELAY_K3_STOP_LAMP, OUTPUT);
  pinMode(RELAY_K4_NORMAL_WORK_LAMP, OUTPUT);
  pinMode(ORGS_OUTPUT, OUTPUT); 
  attachInterrupt(digitalPinToInterrupt(STOP_BUTTON), STOP, RISING);
  Serial.println("AGS Sampling Device Started.");
  Serial.println();
}
//функция переключения триггеров работы устройства
void yield()  //авторестарт в тестовом режиме
{
   if (TEST_MODE==1&&PREVENT_RESTART==true)
   {checkTimestamp = millis()/1000;
    if(checkTimestamp-setTimestamp>=TEST_RESTART_TIME)
   {START_TRIGGER = true;
    PROGRAM_FIRST_STARTED = false;
    PREVENT_RESTART = false;
    if(PRINT_ONCE)
    {Serial.println("Device restarted!");}}}
}

void STOP()
{
  if (!PREVENT_RESTART) 
  {START_TRIGGER = !START_TRIGGER;
   PREVENT_RESTART = !PREVENT_RESTART;
   setTimestamp=millis()/1000;
   Serial.println("Device stopped!");
   yield();
   PRINT_ONCE = true;}
}
//функция вращения вперед с индикацией
void FORWARD()
{
  digitalWrite(RELAY_K1_FORWARD, HIGH);
  digitalWrite(RELAY_K2_REVERSE, LOW);
  FORWARD_TRIGGER = true;
  REVERSE_TRIGGER = false;
}
//функция вращения назад с индикацией
void REVERSE()
{
  digitalWrite(RELAY_K1_FORWARD, LOW);
  digitalWrite(RELAY_K2_REVERSE, HIGH);
  FORWARD_TRIGGER = false;
  REVERSE_TRIGGER = true;
}
//функция отключения реле с индикацией
void STOP_MOVEMENT()
{
   digitalWrite(RELAY_K1_FORWARD, LOW);
   digitalWrite(RELAY_K2_REVERSE, LOW);
   digitalWrite(RELAY_K4_NORMAL_WORK_LAMP, LOW);
   digitalWrite(RELAY_K3_STOP_LAMP, HIGH);
}
//функция отключения реле без индикации
void PAUSE_MOVEMENT()
{
   digitalWrite(RELAY_K1_FORWARD, LOW);
   digitalWrite(RELAY_K2_REVERSE, LOW);
}
//основной цикл
void loop()
{
  TOP_REACHED = digitalRead(S1_STARTPOSITION_ENDSTOP); //определения считываний состояния концевиков и кнопки ручного отбора
  BOTTOM_REACHED = digitalRead(S2_ENDPOSITION_ENDSTOP);
  MOB_STATE = digitalRead(MANUAL_OPERATION_BUTTON);
  //возврат в начальное положение после перезапуска
  if(PROGRAM_FIRST_STARTED&&!TOP_REACHED)
  {
    REVERSE();
    if(TOP_REACHED)
    {
      STOP_MOVEMENT();
    }
  }
  //подача сигнала на АГС в верхнем положении отборника
  if(TOP_REACHED)
  {
    digitalWrite(ORGS_OUTPUT, HIGH);
  }
  else
  {
    digitalWrite(ORGS_OUTPUT, LOW);
  }
  //микрозадержка для обработки режимов
  delay(1);
  //смена триггеров начала при нажатии на кнопку старта
  if(digitalRead(START_BUTTON) == 1)
  {
    START_TRIGGER = true;
    PROGRAM_FIRST_STARTED = false;
    PREVENT_RESTART = false;
    if(PRINT_ONCE)
    {Serial.println("Device started!");}
    PRINT_ONCE = false;
  }
  //условия для корректной работы кнопки ручного отбора
  if(MOB_STATE != LAST_MOB_STATE)
  {
    if(MOB_STATE == 1)
    {
      WAIT_DONE = true;
    }
  }
  LAST_MOB_STATE = MOB_STATE;
  //точка начала программы при нажатии кнопки старт
  if(START_TRIGGER)
  {
    digitalWrite(RELAY_K3_STOP_LAMP, LOW); //индикация работы
    digitalWrite(RELAY_K4_NORMAL_WORK_LAMP, HIGH);
    //действия при достижении верхней точки
    if(TOP_REACHED)
    {
      if(START_ONCE)//старт цикла, движение вниз
      {
        start_time = millis()/1000;
        Serial.print("Cycle started. start_time = ");
        Serial.println(start_time);
        FORWARD();
        START_ONCE = false;
        WAIT_DONE = false;
      }
      
      if(CYCLE_FINISHED) //движение вверх, конец цикла
      {
        SET_PREV_GRAB_TIME = true;
        end_time = millis()/1000;
        PAUSE_MOVEMENT();
        Serial.print("Cycle finished. end_time = ");
        Serial.println(end_time);
        total_cycle_time = end_time - start_time;
        Serial.print("total_cycle_time = ");
        Serial.println(total_cycle_time);
        Serial.print("CYCLE_TIME = ");
        Serial.println(CYCLE_TIME);
        Serial.print("delta = ");
        Serial.println(CYCLE_TIME - total_cycle_time);
        if(CYCLE_TIME - total_cycle_time >= 0)
        {time_to_wait = CYCLE_TIME - total_cycle_time;}
        else {time_to_wait = 0;}
        Serial.print("time_to_wait = ");
        Serial.println(time_to_wait);
        Serial.print("ORGS_OUTPUT = ");
        Serial.println(digitalRead(ORGS_OUTPUT));
        Serial.println();
        CYCLE_FINISHED = false;
        START_WAIT_SEQUENCE = true;
      }

      if(START_WAIT_SEQUENCE) //выдержка времени для достижения требуемого интервала отборов пробы
      {
        wait_cur = millis()/1000;
        if(wait_cur - end_time >= time_to_wait)
         {
           WAIT_DONE = true;
         }
      }
      if(WAIT_DONE) //смена триггеров по окончанию выдержки
      {
        START_ONCE = true;
        START_WAIT_SEQUENCE = false;
      }
    }
    
    if(BOTTOM_REACHED) //движение вниз до нижней точки
    {
      if(SET_PREV_GRAB_TIME)
      {
        check_time = millis()/1000;
        PAUSE_MOVEMENT();
        Serial.print("Endpoint reached. Grabbing probe... check_time = ");
        Serial.println(check_time);
        START_GRAB_SEQUENCE = true;
        delay(10);
        grab_time_prev = millis()/1000;
        SET_PREV_GRAB_TIME = !SET_PREV_GRAB_TIME;
      }
      
      if(START_GRAB_SEQUENCE) //отбор пробы
      {
       grab_time_cur = millis()/1000;
       if(grab_time_cur - grab_time_prev >= PROBE_GRAB_TIME)
       {
        if(START_GRAB_SEQUENCE)
        {
          check_time = millis()/1000;
          Serial.print("Grab done. Returning... check_time = ");
          Serial.println(check_time);
          CYCLE_FINISHED = true;
          START_GRAB_SEQUENCE = !START_GRAB_SEQUENCE;
          delay(10);
          REVERSE();
        }
       }
      } 
    }
    else //движение вне концевиков
    {
      if(REVERSE_TRIGGER)
      {
        REVERSE();
      }
      if(FORWARD_TRIGGER)
      {
        FORWARD();
      }
    }
   }
   else //отключение и индикация при нажатии кнопки стоп
   {
    STOP_MOVEMENT();
   }
}
