/**
 * @file main.c
 * @brief PatroSum: Interactive Addition Game for BitDogLab (RP2040)
 * @author Luis Felipe Patrocinio (https://github.com/luisfpatrocinio/)
 *
 * PatroSum is an interactive math game designed for the BitDogLab (RP2040) platform.
 * The player must solve randomly generated addition problems using a 4x4 matrix keypad.
 * The game provides visual feedback on a display, audio feedback via a buzzer, and uses colored LEDs to indicate correct or incorrect answers.
 *
 * - Random addition questions with numbers up to 999
 * - User input via 4x4 matrix keypad
 * - Visual feedback on display (question, answer, result)
 * - Audio feedback with buzzer (success/error tones)
 * - RGB LEDs for status indication (correct/incorrect)
 * - State machine controls game flow
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
#include "approach.h"

// Display
#include "display.h"
#include "text.h"
#include "draw.h"

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
GameState currentGameState;

// Questão atual
int num1, num2, correctAnswer;
char questionStr[32];  // "num1 + num 2 = "
char answerBuffer[10]; // guarda a resposta do jogador
float questionY = 20;

void generateQuestion()
{
  num1 = rand() % 1000;
  num2 = rand() % 1000;
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
  currentGameState = GENERATE_NEW_QUESTION;

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

  clearDisplay();
  playWelcomeTones();

  drawTextCentered("Bem-vindo ao", 0);
  drawTextCentered("PatroSum", 16);
  showDisplay();

  char str[32] = "";

  while (true)
  {
    // Maquina de estados
    switch (currentGameState)
    {
    case GENERATE_NEW_QUESTION:
      generateQuestion();
      memset(answerBuffer, 0, sizeof(answerBuffer)); // Limpa a resposta anterior
      currentGameState = WAITING_FOR_INPUT;
      questionY = 20; // Reseta a posição Y da pergunta
      break;
    case WAITING_FOR_INPUT:
    {
      pulseLed(LED_RED_PIN, 0.20);
      pulseLed(LED_GREEN_PIN, 0.20);
      pulseLed(LED_BLUE_PIN, 0.20);

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
          currentGameState = CHECK_ANSWER;
        }
        // Se for '*', limpa o buffer
        else if (key == '*')
        {
          memset(answerBuffer, 0, sizeof(answerBuffer));
          playTone(220, 50); // Beep diferente para limpar
        }

        // Aguarda um pouco para evitar leituras múltiplas (debounce)
        sleep_ms(6);
      }
    }
    break;
    case CHECK_ANSWER:
    {
      int playerAnswer = atoi(answerBuffer); // Converte a string da resposta para inteiro
      clearDisplay();

      if (playerAnswer == correctAnswer)
      {
        setLedBrightness(LED_RED_PIN, 0);     // Desliga o LED vermelho
        setLedBrightness(LED_GREEN_PIN, 255); // Liga o LED verde
        setLedBrightness(LED_BLUE_PIN, 0);    // Desliga o LED azul
        drawTextCentered("Correto! :)", 8);
        playTone(523, 150); // C5
        sleep_ms(100);
        playTone(659, 150); // E5
        sleep_ms(100);
        playTone(784, 150); // G5
      }
      else
      {
        setLedBrightness(LED_RED_PIN, 255); // Liga o LED vermelho
        setLedBrightness(LED_GREEN_PIN, 0); // Desliga o LED verde
        setLedBrightness(LED_BLUE_PIN, 0);  // Desliga o LED azul
        drawTextCentered("Errado! :(", 0);
        char correct_str[32];
        sprintf(correct_str, "Resp: %d", correctAnswer);
        drawTextCentered(correct_str, 16);
        blink_led_red(3, 150);
        playTone(261, 500); // C4 (som de erro)
      }
      showDisplay();
      sleep_ms(2000); // Pausa para o jogador ver o resultado
      currentGameState = GENERATE_NEW_QUESTION;
    }
    break;
    }

    // --- Lógica de Desenho na Tela ---
    // Esta parte é executada a cada ciclo, exceto durante a tela de resultado
    if (currentGameState == WAITING_FOR_INPUT)
    {
      clearDisplay();

      // Desenha retangulo no topo
      int _rectHeight = 4;
      drawRectangle(0, 0, SCREEN_WIDTH, _rectHeight);
      drawRectangle(0, SCREEN_HEIGHT - _rectHeight, SCREEN_WIDTH, SCREEN_HEIGHT);

      size_t len = strlen(answerBuffer);
      float _newQuestionY = len > 0 ? 12.0 : 20.0; // Ajusta a posição Y da pergunta se houver resposta
      questionY = approach(questionY, _newQuestionY, 1);
      drawTextCentered("Resolva a conta:", questionY);
      drawTextCentered(questionStr, questionY + 16);
      // Desenha a resposta do usuário ao lado da pergunta
      drawTextCentered(answerBuffer, 48);
      showDisplay();
    }

    sleep_ms(10); // Pequeno delay no loop principal
  }
}
