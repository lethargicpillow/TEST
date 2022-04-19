#include <SPI.h>
#include <Controllino.h>

#define MAO CONTROLLINO_A0 //вывод, к которому подключен МЭО
#define VIBRO CONTROLLINO_D3 //вывод, к которому подключен вибростол
#define LIGHT CONTROLLINO_D2 //вывод, к которому подключено освещение
#define VIBRO_DURATION 5500 //длительность работы вибростола, мс
#define LIGHT_DURATION 30000 //длительность работы освещения, мс
#define PROBE_REMOVE_DURATION 40000 //длительность цикла удаления пробы, мс
#define STARTUP_DELAY 500 //единичная задержка для полной загрузки прибора после его перезапуска
#define LIGHT_STARTUP_DELAY 1000 //задержка на время разогрева светодиодов
#define CONTACT_BOUNCE_DELAY 1500 //задержка для избежания дребезга контактов
#define DOUBLECHECK_DELAY 2000

boolean MAO_STATE = 1; //переменная текущего состояния МЭО
boolean MAO_PREV_STATE = 0; //переменная предыдущего состояния МЭО
boolean TRIGGER = 1; //триггер концевика
boolean DOUBLECHECK = 0;

void setup() 

{
 pinMode(MAO, INPUT); //определение вывода МЭО как входного
 pinMode(LIGHT, OUTPUT); //определение вывода вибростола как выходного
 pinMode(VIBRO, OUTPUT); //определение вывода освещения как выходного
 Serial.begin(9600); //инициализация COM-порта на скорости 9600 бод
 Serial.print(STARTUP_DELAY/1000);
 delay(STARTUP_DELAY);
}

void loop()
{
   MAO_STATE = digitalRead(MAO); //считывание сигнала текущего состояния МЭО;
   if (MAO_STATE != MAO_PREV_STATE) //если изменился сигнал с МЭО
   {
   delay(DOUBLECHECK_DELAY);
   if (MAO_STATE == digitalRead(MAO))
   {DOUBLECHECK=1;}
   else {DOUBLECHECK=0;}
   if (MAO_STATE == LOW && TRIGGER == 0 && DOUBLECHECK == 1) //если сработал концевик МЭО и он был отжат ранее
   {
   TRIGGER = 1; //установка триггера концевика
   DOUBLECHECK = 0;
   digitalWrite(VIBRO, HIGH); //включение вибростола
   delay(VIBRO_DURATION); //работа вибростола в течение указанного времени (подготовка пробы)
   digitalWrite(VIBRO, LOW); //отключение вибростола
   digitalWrite(LIGHT, HIGH); //включение освещения
   delay(LIGHT_STARTUP_DELAY); //задержка на разогрев светодиодов
   Serial.println(TRIGGER); //вывод логической 1 в COM-порт
   delay(LIGHT_DURATION); //работа освещения в течение указанного времени
   digitalWrite(LIGHT, LOW); //отключение освещения
   digitalWrite(VIBRO, HIGH); //включение вибростола
   delay(PROBE_REMOVE_DURATION); //работа вибростола в течение указанного времени (удаление пробы)
   digitalWrite(VIBRO, LOW); //отключение вибростола
   } 
   else //если сигнал МЭО не изменился (и концевик не сработал)
   {
   TRIGGER = 0; //сброс триггера концевика
   digitalWrite(VIBRO, LOW); //отключение вибростола
   digitalWrite(LIGHT, LOW); //отключение освещения
   }
    delay(CONTACT_BOUNCE_DELAY); //микрозадержка от дребезга контактов
   }
   MAO_PREV_STATE = MAO_STATE; //запись переменной предыдущего состояния МЭО
}
