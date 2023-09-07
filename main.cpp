/*! \mainpage Simon dice  - Game
 * \date 01/01/2023
 * \author Gonzalo Martin Buffa
 * \section Juego Simon dice en microcontrolador STM32, utilizando pultadores y leds
 * [Complete aqui con su descripcion]
 *
 * \section desarrollos Observaciones generales
 * [Complete aqui con sus observaciones]
 *
 * \section changelog Registro de cambios
 *
 * |   Fecha    | Descripcion                                    |
 * |:----------:|:-----------------------------------------------|
 * | 01/09/2023 | Creacion del documento                         |
 * | 02/09/2023 | Trabajo clase presencial                       |
 * 
 * 
 * 
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "mbed.h"
#include "stdlib.h"
/* END Includes --------------------------------------------------------------*/


/* typedef -------------------------------------------------------------------*/

/* typedef -------------------------------------------------------------------*/
/**
 * @brief Tipo de datos de puntero a función, sirve para declarar los distintos callbacks.-
 * 
 */
typedef void(*ptrFunc)(void *param);

/**
 * @brief Enumeracion de los diferentes modos de juego de la MEF(Maquina de Estados Finita)
*/
typedef enum{
    SEQUENCE, 
    LEVEL,
    GAME_LEVEL,
    BEGIN_SQC,
    GAME_SQC,
    GAME,
    WON,
    LOST,
} _eMEFStates;

typedef enum{
    BUTTON_DOWN,
    BUTTON_UP,
    BUTTON_RISING,
    BUTTON_FALLING
}_eButtonState;

/**
 * @brief Enumeracion de los estados de los diferentes estados de los botones, como tengo una configuracion PullDown los coloqué de tal forma que me quede el valor de NOT_PRESSED = 0  y PRESSED = 1
*/
typedef enum{
    PRESSED,
    NOT_PRESSED,
    NO_EVENT
}_eEvent;

typedef struct
{
    _eButtonState   currentState;
    _eEvent         stateInput;
    ptrFunc         callBack;
    uint32_t        timePressed;
    uint32_t        timeDiff;
}_sButton;

typedef union{
    struct{
        uint8_t bit7:1;
        uint8_t bit6:1;
        uint8_t bit5:1;
        uint8_t bit4:1;
        uint8_t bit3:1;
        uint8_t bit2:1;
        uint8_t bit1:1;
        uint8_t bit0:1;
    } bits;
    uint8_t allBits;
} _uByte;

/* END typedef ---------------------------------------------------------------*/

/* define --------------------------------------------------------------------*/
#define INTERVAL            200
#define DEBOUNCETIME        40
#define HALFTIMELED         500
#define LEDTIME             1000
#define DOUBLELEDTIME       2000
#define RESTIME             3000
#define HEARTBEAT           100
#define MASK                0x01
#define LIMIT               0x0F
#define NUMBUTTONS          4
#define MAXLED              4
#define MAXTIME             500
#define BASETIME            100

#define ALLFLAGS            bitMap.allBits
#define RANDINTERVAL        bitMap.bits.bit0
#define INGAME              bitMap.bits.bit1
#define RESLED              bitMap.bits.bit2

/* END define ----------------------------------------------------------------*/

/* hardware configuration ----------------------------------------------------*/
DigitalOut LED(PC_13);
BusIn buttons(PA_7,PA_6,PA_5,PA_4);
BusOut leds(PB_15,PB_14,PB_7,PB_6); //funciona como muchos digitalOut    
/* END hardware configuration ------------------------------------------------*/

/* Function prototypes -------------------------------------------------------*/
void startButton(_sButton *button, ptrFunc buttonFunc);

uint8_t updateMefTask(_sButton *button, int index);

void buttonTask(void *param);

/* END Function prototypes ---------------------------------------------------*/


/* Global variables ----------------------------------------------------------*/
Timer myTimer;
_uByte bitMap;
_eMEFStates states;  
_sButton myButtons[NUMBUTTONS];                 
const uint8_t ledSequence[7] = {0x08,0x0C,0x04,0x06,0x02,0x03,0x01};
const uint8_t gameLevel[9] = {0x0B,0x0A,0x09,0x08,0x07,0x06,0x05,0x04,0x03};
const uint8_t gameSqc[5] = {0x0F, 0x0E, 0x0C, 0x08, 0x00};

/* END Global variables ------------------------------------------------------*/


/* Function prototypes user code ----------------------------------------------*/
void startButton(_sButton *button, ptrFunc buttonFunc){
    button->currentState = BUTTON_UP;
    button->stateInput = NO_EVENT;
    button->callBack = buttonFunc;
    button->timePressed = 0;
    button->timeDiff = 0;
}

uint8_t updateMefTask(_sButton *button, int index){
    uint8_t action=false;
    switch (button->currentState){
        case BUTTON_UP:
            if(button->stateInput==PRESSED)
                button->currentState=BUTTON_FALLING;
        break;
        case BUTTON_FALLING:
            if(button->stateInput==PRESSED){
                    button->currentState=BUTTON_DOWN;
                    button->timePressed=myTimer.read_ms();
                    action=true;
            }else{
                button->currentState=BUTTON_UP;
            }
        break;
        case BUTTON_DOWN:
            if(button->stateInput==NOT_PRESSED)
                button->currentState=BUTTON_RISING;
        break;
        case BUTTON_RISING:
            if(button->stateInput==NOT_PRESSED){
                button->currentState=BUTTON_UP;
                button->timeDiff = myTimer.read_ms() - button->timePressed;
            }else{
                button->currentState=BUTTON_DOWN;
            }
        break;
        default:
            button->currentState=BUTTON_UP;
        break;
    }

    if(!index) //usado en al presionar el boton
        action=true;

    return action;
}

void buttonTask(void *param){

    static uint8_t indice=0;
    uint16_t *maskLed = (uint16_t*)param;
   
    *maskLed &= -(indice!=0);
    *maskLed |= (1<<indice);
    indice +=2;
    indice &=LIMIT;
}

/* END Function prototypes user code ------------------------------------------*/


int main()
{
/* Local variables -----------------------------------------------------------*/
    static int32_t ledTime = 0;
    static int32_t offLedTime = 0; //usado en mostrar la secuencia del juego
    static int8_t indexSequence = 0;
    static int32_t heartBeatTime = 0;
    static int8_t heartBeatStatus = 0;
    static int32_t timeToDebounce = 0;
    static uint8_t totalLevel = 0, currentLevel = 0;
    static uint16_t randomLed = 0, randomInterval = 0;
    static uint8_t gameSequence[12] = {};
    static uint8_t gameIndex = 0;
    static int8_t gameData = 0; //usada para guardar el valor ingresado por el pulsador
    uint16_t cont = 0;
    uint8_t i = 0;
/* END Local variables -------------------------------------------------------*/

/* User code -----------------------------------------------------------------*/
    myTimer.start();

    ALLFLAGS = false;

    //inicializamo los botones
    for(i=0; i<NUMBUTTONS; i++) {
        startButton(&myButtons[i], buttonTask);
    }

states = SEQUENCE;
    while(1){
        
        if((myTimer.read_ms()-timeToDebounce)>DEBOUNCETIME)
        {
            timeToDebounce=myTimer.read_ms();

            for(i=0; i<NUMBUTTONS; i++){
                if(buttons[i].read())
                    myButtons[i].stateInput = PRESSED;
                else
                    myButtons[i].stateInput = NOT_PRESSED;
                if(updateMefTask(&myButtons[i], 1))
                    gameData = MASK<<i;
            }

            switch (states)
            {
            case SEQUENCE:

                if((myTimer.read_ms()-ledTime)>INTERVAL) //interval -> tendria que cambair dependiendo de la variable a que esto usando
                {
                    ledTime=myTimer.read_ms();
                    leds.write(~ledSequence[indexSequence]);
                    (indexSequence<6)?indexSequence++ : indexSequence = 0;
                }

                //boton SW1
                if(buttons[1].read())
                    myButtons[1].stateInput = PRESSED;
                else
                    myButtons[1].stateInput = NOT_PRESSED;
                if(updateMefTask(&myButtons[1], 1))
                    states=LEVEL;

                //boton SW0
                if(buttons[0].read()){ 
                    myButtons[0].stateInput = PRESSED;
                }else{
                    myButtons[0].stateInput = NOT_PRESSED;
                }

                if(updateMefTask(&myButtons[0],0) && myButtons->timeDiff > LEDTIME && myButtons->timeDiff < DOUBLELEDTIME){
                    states=GAME_LEVEL;
                    ledTime=myTimer.read_ms();
                    currentLevel=indexSequence=0;
                }

                break;
            case LEVEL:

                if(buttons[1].read())
                    myButtons[1].stateInput = PRESSED;
                else
                    myButtons[1].stateInput = NOT_PRESSED;

                if(updateMefTask(&myButtons[1],1)){
                    totalLevel<8? totalLevel++ : totalLevel=0;
                    ledTime=myTimer.read_ms();
                }

                leds.write(gameLevel[totalLevel]);

                if((myTimer.read_ms()-ledTime)>DOUBLELEDTIME) //200*10 -> 2segundos
                {
                    ledTime=myTimer.read_ms();
                    states=SEQUENCE;
                }


                break;
            case GAME_LEVEL:

                if((myTimer.read_ms()-ledTime)>DOUBLELEDTIME){
                    states=BEGIN_SQC;
                    ledTime=myTimer.read_ms();
                    indexSequence=0;
                } else{
                    leds.write(gameLevel[currentLevel]);
                }           

                break;
            case BEGIN_SQC:

                if((myTimer.read_ms()-ledTime)>LEDTIME){
                    ledTime=myTimer.read_ms();
                    leds.write(~gameSqc[indexSequence]);

                    if(indexSequence<4){
                        indexSequence++;
                    } else{
                        indexSequence=gameIndex=0;
                        ledTime=myTimer.read_ms();
                        RANDINTERVAL=true; //lo activamos cunaod cambiamos de nivel

                        if(INGAME){
                            ledTime = myTimer.read_ms();
                            states=GAME;
                            INGAME=false;
                        } else{
                            states=GAME_SQC;
                        }
                    }
                }

            break;
            case GAME_SQC:

                srand(myTimer.read_us());

                if(RANDINTERVAL){
                    randomInterval = (rand()% (MAXTIME+1)+BASETIME);// ((rand()%(500+1)+100);
                    RANDINTERVAL = false;
                }

                if((myTimer.read_ms()-offLedTime)>(randomInterval/2)){ //limpiamos la pantalla antes de imprimir
                    offLedTime = myTimer.read_ms();
                    leds.write(0x0F); //apagamos todos los leds
                }                
                
                if(myTimer.read_ms()-ledTime > randomInterval){ //entramos cuando se cumple el tiempo generado
                    ledTime=myTimer.read_ms();

                    randomLed = ((rand()%MAXLED+1)-1); // ((rand()%4+1)-1);

                    if(gameIndex<currentLevel+4){
                        gameSequence[gameIndex] = MASK<<randomLed;
                        leds.write(~gameSequence[gameIndex]);
                        gameIndex++;
                    } else{ //volvemos para mostrar la secuencia
                        leds.write(0x0F);
                        gameIndex = 0;
                        ledTime = offLedTime = myTimer.read_ms();
                        states=BEGIN_SQC; 
                        INGAME=true;
                    }                    
                }

                break;
            case GAME:

                leds=~buttons;

                //if((buttons.read()==(gameSequence[gameIndex])) && (buttons.read()!=0x00)){
                if((gameData == gameSequence[gameIndex]) && (gameData!=0)){
                    ledTime=myTimer.read_ms(); //actualizamos para no perder por falta de tiempo
                    gameIndex++;
                    gameData = 0;

                    //puede no entrar en ningun if
                    if((currentLevel==totalLevel) && (gameIndex==currentLevel+4)){ //llega al maximo, entonces ganas
                        states=WON;
                        gameIndex = 0;
                        ledTime=myTimer.read_ms(); 
                    }
                    
                    if((gameIndex==currentLevel+4) && (currentLevel<totalLevel)){ //primero controlamos el numero que ingresamos y luego el nivel en el que estamos
                        currentLevel++;
                        states=GAME_LEVEL;
                    }
                } else if(((gameData != gameSequence[gameIndex]) && (gameData!=0))  || ((myTimer.read_ms()-ledTime)>RESTIME)){ 
                //else if(((buttons.read()!=(gameSequence[gameIndex])) && (buttons.read()!=0x00)) || ((myTimer.read_ms()-ledTime)>RESTIME)){
                        states=LOST;
                        gameIndex=0;
                        gameData = 0;
                        ledTime=myTimer.read_ms(); 
                }

            break;
            case LOST:
                if(cont==20){
                    leds.write(0x0F);
                    if((myTimer.read_ms()-ledTime)>RESTIME) //interval -> tendria que cambair dependiendo de la variable a que esto usando
                    {   
                        ledTime=myTimer.read_ms();
                        states=SEQUENCE;
                        cont=0;
                        for(i=0; i<NUMBUTTONS; i++) {
                            startButton(&myButtons[i], buttonTask);
                        } 
                    }
                } else{
                    if((myTimer.read_ms()-ledTime)>HEARTBEAT) //interval -> tendria que cambair dependiendo de la variable a que esto usando
                    {
                        ledTime=myTimer.read_ms();
                        cont++;

                        if(!RESLED){
                            leds.write(0x00);
                            RESLED = true;
                        } else{
                            leds.write(0x0F);
                            RESLED = false;
                        }
                    }
                }
                break;
            case WON:
                if(cont==4){
                    leds.write(0x0F);
                    if((myTimer.read_ms()-ledTime)>RESTIME) //interval -> tendria que cambair dependiendo de la variable a que esto usando
                    {   
                        ledTime=myTimer.read_ms();
                        states=SEQUENCE;
                        cont=0;
                        for(i=0; i<NUMBUTTONS; i++) {
                            startButton(&myButtons[i], buttonTask);
                        }                         
                    }
                } else{
                    if((myTimer.read_ms()-ledTime)>HALFTIMELED) //interval -> tendria que cambair dependiendo de la variable a que esto usando
                    {
                        ledTime=myTimer.read_ms();
                        cont++;

                        if(!RESLED){
                            leds.write(0x00);
                            RESLED = true;
                        } else{
                            leds.write(0x0F);
                            RESLED = false;
                        }
                    }
                }
                break;
            }
        }

        if((myTimer.read_ms()-heartBeatTime)>HEARTBEAT) //interval -> tendria que cambair dependiendo de la variable a que esto usando
        {
            heartBeatTime=myTimer.read_ms();
            if(heartBeatStatus){
                LED.write(1);
                heartBeatStatus=0;
            } else {
                LED.write(0);
                heartBeatStatus=1;
            }
        }
    
        //leds=buttons;
    }

/* END User code -------------------------------------------------------------*/
}