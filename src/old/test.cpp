#include <LiquidCrystal_AIP31068_I2C.h>
// CONTORNO TEMPORÁRIO DO LASECSIMUL (ver comentário do "display" abaixo): expõe o membro privado
// spi_dev da biblioteca só para trocar a velocidade do SPI por software. Remover quando o
// simulador entregar as bordas rápidas corretamente.
#define private public
#include <Adafruit_PCD8544.h>
#undef private

#define BUTTON 13   // entrada do botão
#define BUTPIN 12   // saída que reflete o estado do botão
#define BLINK  27   // LED que pisca
#define PWMPIN 17   // saída PWM

// Configuração do PWM gerado pelo LEDC, via analogWrite() (em vez de
// ledcSetup/ledcAttachPin/ledcWrite manuais com canal fixo): no SimulIDE,
// ledcWrite() sozinho (sem repetir o ledcAttachPin) não atualiza o sinal
// visto no analisador - o exemplo "Devkitc_test" do próprio SimulIDE usa
// analogWrite() e funciona corretamente, então seguimos o mesmo caminho.
#define PWM_FREQ_HZ 1000           // frequência do PWM: 1 kHz (10 Hz dava só 1 pulso a cada 100 ms,
                                   // período longo demais para aparecer bem no analisador de sinal)
// Resolução de 8 bits (e não 12): o analogWrite() usa os canais LEDC de baixa velocidade (15, 14...)
// e, no LasecSimul, esses canais só aceitam 1 kHz até 8 bits - com 12 bits o ledcSetup() falha e
// cada analogWrite() imprime "analogWrite setup failed" na serial. Na placa real 12 bits funcionaria.
#define PWM_RES_BITS 8               // resolução: 8 bits -> duty vai de 0 a 255
#define PWM_DUTY_MAX ((1 << PWM_RES_BITS) - 1)  // duty máximo (100%) para a resolução acima

LiquidCrystal_AIP31068_I2C lcd(0x3E, 16, 2);
// Na placa real, o normal é SPI por hardware (VSPI: CLK = GPIO18, DIN = GPIO23 fixos):
//   Adafruit_PCD8544 display = Adafruit_PCD8544(16, 5, 19); // (DC, CS, RST)
// No LasecSimul atual isso não funciona: o DC/CS (GPIO) e os bits do SPI por hardware chegam ao
// circuito fora de ordem, e o SPI por software a 4 MHz perde bordas. Por isso, para a aula, usamos
// SPI por software (mesmos pinos) em 10 kHz - devagar o bastante para o simulador acompanhar.
#define PCD8544_SOFT_SPI_HZ 10000
Adafruit_PCD8544 display = Adafruit_PCD8544(18, 23, 16, 5, 19); // Software SPI (CLK, DIN, DC, CS, RST)

void setup()
{
  Serial.begin(115200);

  pinMode(BUTTON, INPUT);
  pinMode(BLINK, OUTPUT);
  pinMode(BUTPIN, OUTPUT);

  // Configura o PWM (1 kHz, 8 bits) no pino PWMPIN. O duty começa em 0
  // e é atualizado em rampa no loop().
  pinMode(PWMPIN, OUTPUT);
  analogWriteFrequency(PWM_FREQ_HZ);
  analogWriteResolution(PWM_RES_BITS);
  analogWrite(PWMPIN, 0);

  lcd.init();
  lcd.setCursor(3, 0);
  lcd.print("Hello, world!");
  delay(1500);

  // Troca o dispositivo SPI criado pela biblioteca (fixo em 4 MHz) por um igual, só que lento.
  delete display.spi_dev;
  display.spi_dev = new Adafruit_SPIDevice(5, 18, -1, 23, PCD8544_SOFT_SPI_HZ); // (CS, CLK, MISO, DIN, Hz)
  display.begin();   // begin() não limpa a memória de vídeo do controlador, então sem isso
  display.clearDisplay();   // a tela fica mostrando o "lixo" de inicialização (ruído na imagem).
  display.display();
  delay(1500);

  lcd.clear();
}

void loop()
{
  // --- Tarefa 1: ler o botão e atualizar os displays (a cada 50 ms) ---
  static uint32_t lastButton = 0;
  if (millis() - lastButton > 50) {
    lastButton = millis();

    bool pressed = (digitalRead(BUTTON) == LOW);
    digitalWrite(BUTPIN, pressed ? LOW : HIGH);

    const char *msg = pressed ? "BUTTON Low " : "BUTTON High";
    Serial.println(msg);

    lcd.setCursor(3, 0);
    lcd.print(msg);

    // Com o SPI em 10 kHz, redesenhar o Nokia leva ~0,5 s; então só redesenha quando o
    // estado do botão muda (senão o pisca e o PWM ficariam travando a cada 50 ms).
    static int lastPressed = -1;
    if (pressed != lastPressed) {
      lastPressed = pressed;
      display.clearDisplay();
      display.setCursor(10, 10);
      display.println(msg);
      display.display();
    }
  }

  // --- Tarefa 2: piscar o LED, alternando o estado (a cada 500 ms) ---
  static uint32_t lastBlink = 0;
  static bool blinkState = false;
  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    blinkState = !blinkState;
    digitalWrite(BLINK, blinkState ? HIGH : LOW);
  }

  // --- Tarefa 3: rampa de PWM, subindo de 0% a 100% e depois descendo de
  // volta a 0%, repetindo (passo de 1% do duty máximo a cada 20 ms) ---
  static uint32_t lastPwm = 0;
  static int pwmDuty = 0;
  static int pwmStep = PWM_DUTY_MAX / 100;
  if (millis() - lastPwm > 20) {
    lastPwm = millis();

    pwmDuty += pwmStep;
    if (pwmDuty >= PWM_DUTY_MAX) {
      pwmDuty = PWM_DUTY_MAX;
      pwmStep = -pwmStep;
    } else if (pwmDuty <= 0) {
      pwmDuty = 0;
      pwmStep = -pwmStep;
    }

    analogWrite(PWMPIN, pwmDuty);
  }
}
