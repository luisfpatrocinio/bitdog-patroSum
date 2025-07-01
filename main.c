/**
 * @file main.c
 * @brief 4x4 Matrix Keyboard with Buzzer Example for Raspberry Pi Pico
 * @author Luis Felipe Patrocinio (https://github.com/luisfpatrocinio/)
 *
 * This program scans a 4x4 matrix keyboard and plays a tone on a buzzer corresponding to the pressed key.
 *
 * - Keyboard lines and columns are mapped to GPIO pins.
 * - Each key press triggers a specific frequency.
 * - Uses PWM for buzzer control.
 *
 * @version 0.1
 * @date 07-01-2025
 * @copyright Copyright (c) 2025 Luis Felipe Patrocinio
 * @license This project is released under the MIT License.
 * See LICENSE file for full license text.
 * https://github.com/luisfpatrocinio/bitdog-patroLibs/blob/main/LICENSE
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "buzzer.h"
#include "keypad.h"
#include "led.h"

// Display
#include "display.h"
#include "text.h"
#include "draw.h"

#define LED_RED_PIN 13

/**
 * @brief Frequency map for each key in the 4x4 matrix (Hz).
 */
const int keypad_freq_map[4][4] = {
    {262, 294, 330, 349},  // C4, D4, E4, F4
    {392, 440, 494, 523},  // G4, A4, B4, C5
    {587, 659, 698, 784},  // D5, E5, F5, G5
    {880, 988, 1047, 1175} // A5, B5, C6, D6
};

/**
 * @brief Mapa de teclas para o teclado 4x4.
 * Cada posição corresponde ao caractere exibido na tecla.
 */
const char keypad_key_map[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

// -- Game

// Máquina de estados para controlar o fluxo do jogo
typedef enum
{
  GENERATE_NEW_QUESTION,
  WAITING_FOR_INPUT,
  CHECK_ANSWER
} GameState;
GameState current_game_state;

// Questão atual
int num1, num2, correctAnswer;
char questionStr[32];  // "num1 + num 2 = "
char answerBuffer[10]; // guarda a resposta do jogador

void generateQuestion()
{
  num1 = rand() % 10;
  num2 = rand() % 10;
  correctAnswer = num1 + num2;
  sprintf(questionStr, "%d + %d = ?", num1, num2);
}

/**
 * @brief Pisca o LED vermelho conectado ao pino 13.
 * @param times Quantas vezes piscar
 * @param delay_ms Delay em ms entre liga/desliga
 */
void blink_led_red(int times, int delay_ms)
{
  for (int i = 0; i < times; i++)
  {
    gpio_put(LED_RED_PIN, 1);
    sleep_ms(delay_ms);
    gpio_put(LED_RED_PIN, 0);
    sleep_ms(delay_ms);
  }
}

/**
 * @brief Initializes the standard IO, buzzer, and keypad.
 *
 * This function should be called once at the beginning of main().
 */
void setup()
{
  stdio_init_all();
  initBuzzerPWM();
  initKeypad();
  initDisplay();
  initLeds();

  // Inicializa o gerador de números aleatórios com um valor único
  srand(time_us_32());

  // Define o estado inicial do jogo
  current_game_state = GENERATE_NEW_QUESTION;

  // Limpa o buffer de resposta inicial
  memset(answerBuffer, 0, sizeof(answerBuffer));
}

uint8_t row, col;

/**
 * @brief Main program entry point.
 *
 * Initializes peripherals and enters the main loop, scanning the keypad and playing tones.
 * @return int Program exit status (never returns in embedded context).
 */
int main()
{
  setup();

  blink_led_red(1, 100);
  playWelcomeTones();

  char str[32] = "";

  while (true)
  {
    // Maquina de estados
    switch (current_game_state)
    {
    case GENERATE_NEW_QUESTION:
      generateQuestion();
      memset(answerBuffer, 0, sizeof(answerBuffer)); // Limpa a resposta anterior
      current_game_state = WAITING_FOR_INPUT;
      break;
    case WAITING_FOR_INPUT:
    {
      KeyEvent evt = keypadScan();
      if (evt.pressed)
      {
        char key = keypad_key_map[evt.row][evt.col];
        size_t len = strlen(answerBuffer);

        // Se for um dígito, adiciona ao buffer
        if (key >= '0' && key <= '9' && len < sizeof(answerBuffer) - 1)
        {
          answerBuffer[len] = key;
          answerBuffer[len + 1] = '\0';
          playTone(440, 50); // Beep de feedback
        }
        // Se for 'A', vai para a verificação
        else if (key == 'A')
        {
          current_game_state = CHECK_ANSWER;
        }
        // Se for '*', limpa o buffer
        else if (key == '*')
        {
          memset(answerBuffer, 0, sizeof(answerBuffer));
          playTone(220, 50); // Beep diferente para limpar
        }
        // Aguarda um pouco para evitar leituras múltiplas (debounce)
        sleep_ms(30);
      }
    }
    break;
    case CHECK_ANSWER:
    {
      int playerAnswer = atoi(answerBuffer); // Converte a string da resposta para inteiro
      clearDisplay();

      if (playerAnswer == correctAnswer)
      {
        drawTextCentered("Correto! :)", 8);
        playTone(523, 150); // C5
        sleep_ms(100);
        playTone(659, 150); // E5
        sleep_ms(100);
        playTone(784, 150); // G5
      }
      else
      {
        drawTextCentered("Errado! :(", 0);
        char correct_str[32];
        sprintf(correct_str, "Resp: %d", correctAnswer);
        drawTextCentered(correct_str, 16);
        blink_led_red(3, 150);
        playTone(261, 500); // C4 (som de erro)
      }
      showDisplay();
      sleep_ms(2000); // Pausa para o jogador ver o resultado
      current_game_state = GENERATE_NEW_QUESTION;
    }
    break;
    }

    // --- Lógica de Desenho na Tela ---
    // Esta parte é executada a cada ciclo, exceto durante a tela de resultado
    if (current_game_state == WAITING_FOR_INPUT)
    {
      clearDisplay();
      drawTextCentered("Resolva a conta:", 0);
      drawText(0, 16, questionStr);
      // Desenha a resposta do usuário ao lado da pergunta
      drawText(strlen(questionStr) * 8, 16, answerBuffer);
      showDisplay();
    }

    sleep_ms(10); // Pequeno delay no loop principal
  }
}
