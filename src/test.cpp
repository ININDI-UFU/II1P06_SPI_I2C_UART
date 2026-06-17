#include <LiquidCrystal_AIP31068_I2C.h>
#include <Adafruit_PCD8544.h>

#define BUTTON 13   // entrada do botão
#define BUTPIN 12   // saída que reflete o estado do botão
#define BLINK  27   // LED que pisca
#define PWMPIN 17   // saída PWM

// Configuração do PWM gerado pelo LEDC
// ledcSetup(canal, freq, resolução) + ledcAttachPin(pino, canal) + ledcWrite(canal, duty)
#define PWM_CHANNEL 0              // canal do LEDC usado para o pino PWMPIN
#define PWM_FREQ_HZ 10             // frequência do PWM: 10 Hz
#define PWM_RES_BITS 12              // resolução: 12 bits -> duty vai de 0 a 4095
#define PWM_DUTY_50PCT (4096 / 2)    // 50% de 4096

LiquidCrystal_AIP31068_I2C lcd(0x3E, 16, 2);
Adafruit_PCD8544 display = Adafruit_PCD8544(18, 23, 16, 5, 19); // Software SPI (CLK, DIN, DC, CS, RST)

void setup()
{
  Serial.begin(115200);

  pinMode(BUTTON, INPUT);
  pinMode(BLINK, OUTPUT);
  pinMode(BUTPIN, OUTPUT);

  // Configura o canal do LEDC (10 Hz, 8 bits) e liga o pino PWMPIN a ele,
  // já fixando o duty em 50%. Como o duty não muda mais, basta fazer isso
  ledcSetup(PWM_CHANNEL, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcAttachPin(PWMPIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, PWM_DUTY_50PCT);

  lcd.init();
  lcd.setCursor(3, 0);
  lcd.print("Hello, world!");
  delay(1500);

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

    display.clearDisplay();
    display.setCursor(10, 10);
    display.println(msg);
    display.display();
  }

  // --- Tarefa 2: piscar o LED, alternando o estado (a cada 500 ms) ---
  static uint32_t lastBlink = 0;
  static bool blinkState = false;
  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    blinkState = !blinkState;
    digitalWrite(BLINK, blinkState ? HIGH : LOW);
  }
}
