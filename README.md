# Simon-Dice
### UNER - Ingeniería Mecatrónica - Computación III - Trabajo Práctico de Embebidos
El juego utiliza una placa de desarrollo Stm32f103 "Blue Pill" con 4 pulsadores conectados en PullUp con 8 resistores en total (4 de 220 ohms y 4 de 4,7K ohms) en los pines A4, A5, A6, A7 (izq a der), los pulsadores activan 4 leds conectados con 4 resistores de 220 ohms a los pines B6, B7, B14, B15 (izq a der)

### Gameplay
El Codigo funciona con una MEF (Maquina de estados finita) por la cual se desplaza durante la ejecución del programa, las distintas etapas del juego son:

- Secuencia inicial (presionando A6 se elije el nivel, manteniendo A7 entre 1 a 2 segundos se ingresa al juego)
- Secuencia previa al juego
  - Se muestra el nivel actual de juego
  - Se muestra una secuencia previa a mostrar la secuencia aleatoria del juego
  - Se muestra la secuencia aleatoria del juego
  - Se vuelve a mostrar la secuencia previa para informar al usuario de que empieze a ingresar datos
- En el juego se debe de presionar el patron dependiendo de la secuencia aleatoria mostrada, en caso de ingresar correctamente la secuencia se muestra la secuencia del siguiente nivel o se muestra la pantalla de victoria, en caso contrario se muestra la de derrota
- Las secuencias de resultado varian dependiedo de si se gana o pierde, luego de esta secuencia y esperar 3 segundo se dirige al usuario a la secuencia inicial.


