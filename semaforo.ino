#include <Servo.h>

//Declaração e atribuição aos pinos:
const int PIN_VERM_CARRO   = 13;
const int PIN_AMARELO      = 12;
const int PIN_VERDE_CARRO  = 11;
const int PIN_VERM_PED     = 10;
const int PIN_VERDE_PED    = 9;
const int PIN_BUZZER       = 7;
const int PIN_BOTAO        = 2;
const int PIN_FOTOSENSOR   = A0;
const int PIN_TRIG         = 5;
const int PIN_ECHO         = 6;
const int PIN_SERVO        = 4;

Servo cancela;

//Fotossensor(Calibrado com valores medidos: claro ~974, escuro ~54)
const int LIMIAR_LUZ   = 500;
const int MARGEM_LUZ   = 200;
bool diaAtual = true;

//Estados semáforo durante o dia:
enum EstadoSemaforo {
  VERDE_CARRO,
  AMARELO_CARRO,
  VERMELHO_CARRO,
  PEDESTRE_AVISO,
  PEDESTRE_VERDE
};
EstadoSemaforo estado = VERDE_CARRO;

unsigned long ultimoTick = 0;
unsigned long tempoNoEstado = 0;

const unsigned long DURACAO_VERDE_CARRO    = 25000UL;
const unsigned long DURACAO_AMARELO_CARRO  = 5000UL;
const unsigned long DURACAO_VERMELHO_CARRO = 15000UL;
const unsigned long DURACAO_PEDESTRE_AVISO = 5000UL;
const unsigned long DURACAO_PEDESTRE_VERDE = 15000UL;

//Buzzer:
unsigned long ultimoBuzzer = 0;
bool buzzerFaseAguda = true;
const unsigned long INTERVALO_BUZZER = 500UL;

//Semáforo modo noturno:
unsigned long ultimoPisca = 0;
bool amareloAceso = false;
const unsigned long INTERVALO_PISCA = 500UL;

//Botão:
unsigned long ultimoBotao = 0;
const unsigned long DEBOUNCE_BOTAO = 200UL;
bool botaoAnterior = HIGH;

//Servo e Cancela:
bool cancelaAberta = false;
unsigned long cancelaAbertaDesde = 0;
const unsigned long TEMPO_CANCELA_ABERTA = 3000UL;
const long DISTANCIA_LIMITE_CM = 30;

void setup() {
  pinMode(PIN_VERM_CARRO, OUTPUT);
  pinMode(PIN_AMARELO, OUTPUT);
  pinMode(PIN_VERDE_CARRO, OUTPUT);
  pinMode(PIN_VERM_PED, OUTPUT);
  pinMode(PIN_VERDE_PED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  pinMode(PIN_BOTAO, INPUT_PULLUP);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  cancela.attach(PIN_SERVO);
  cancela.write(0);

  ultimoTick = millis();
}

void loop() {
  unsigned long agora = millis();
  unsigned long delta = agora - ultimoTick;
  ultimoTick = agora;

  bool dia = lerDiaNoite();

  if (dia) {
    tempoNoEstado += delta;
    verificarBotaoPedestre();
    atualizarSemaforoDiurno();
    atualizarBuzzer();
  } else {
    atualizarPiscaNoturno();
  }

  atualizarSensorEcancela();
}

bool lerDiaNoite() {
  int luz = analogRead(PIN_FOTOSENSOR);

  if (diaAtual) {
    if (luz < LIMIAR_LUZ - MARGEM_LUZ) {
      diaAtual = false;
    }
  } else {
    if (luz > LIMIAR_LUZ + MARGEM_LUZ) {
      diaAtual = true;
    }
  }
  return diaAtual;
}

void verificarBotaoPedestre() {
  bool botaoAtual = digitalRead(PIN_BOTAO);
  if (estado == VERDE_CARRO &&
      botaoAnterior == HIGH &&
      botaoAtual == LOW &&
      (millis() - ultimoBotao >= DEBOUNCE_BOTAO)) {
    ultimoBotao = millis();
    estado = PEDESTRE_AVISO;
    tempoNoEstado = 0;
  }

  botaoAnterior = botaoAtual;
}

void atualizarSemaforoDiurno() {
  switch (estado) {

    case VERDE_CARRO:
      digitalWrite(PIN_VERDE_CARRO, HIGH);
      digitalWrite(PIN_AMARELO, LOW);
      digitalWrite(PIN_VERM_CARRO, LOW);
      digitalWrite(PIN_VERM_PED, HIGH);
      digitalWrite(PIN_VERDE_PED, LOW);
      if (tempoNoEstado >= DURACAO_VERDE_CARRO) {
        estado = AMARELO_CARRO;
        tempoNoEstado = 0;
      }
      break;

    case AMARELO_CARRO:
      digitalWrite(PIN_VERDE_CARRO, LOW);
      digitalWrite(PIN_AMARELO, HIGH);
      if (tempoNoEstado >= DURACAO_AMARELO_CARRO) {
        estado = VERMELHO_CARRO;
        tempoNoEstado = 0;
      }
      break;

    case VERMELHO_CARRO:
      digitalWrite(PIN_AMARELO, LOW);
      digitalWrite(PIN_VERM_CARRO, HIGH);
      digitalWrite(PIN_VERM_PED, LOW);
      digitalWrite(PIN_VERDE_PED, HIGH);
      if (tempoNoEstado >= DURACAO_VERMELHO_CARRO) {
        estado = VERDE_CARRO;
        tempoNoEstado = 0;
      }
      break;

    case PEDESTRE_AVISO:
      digitalWrite(PIN_VERDE_CARRO, LOW);
      digitalWrite(PIN_AMARELO, HIGH);
      if (tempoNoEstado >= DURACAO_PEDESTRE_AVISO) {
        estado = PEDESTRE_VERDE;
        tempoNoEstado = 0;
      }
      break;

    case PEDESTRE_VERDE:
      digitalWrite(PIN_AMARELO, LOW);
      digitalWrite(PIN_VERM_CARRO, HIGH);
      digitalWrite(PIN_VERM_PED, LOW);
      digitalWrite(PIN_VERDE_PED, HIGH);
      if (tempoNoEstado >= DURACAO_PEDESTRE_VERDE) {
        estado = VERDE_CARRO;
        tempoNoEstado = 0;
      }
      break;
  }
}

void atualizarBuzzer() {
  bool devTocar = (estado == VERMELHO_CARRO || estado == PEDESTRE_VERDE);

  if (devTocar) {
    if (millis() - ultimoBuzzer >= INTERVALO_BUZZER) {
      ultimoBuzzer = millis();
      buzzerFaseAguda = !buzzerFaseAguda;
      tone(PIN_BUZZER, buzzerFaseAguda ? 1000 : 2000);
    }
  } else {
    noTone(PIN_BUZZER);
  }
}

void atualizarPiscaNoturno() {
  digitalWrite(PIN_VERDE_CARRO, LOW);
  digitalWrite(PIN_VERM_CARRO, LOW);
  digitalWrite(PIN_VERDE_PED, LOW);
  digitalWrite(PIN_VERM_PED, LOW);
  noTone(PIN_BUZZER);

  if (millis() - ultimoPisca >= INTERVALO_PISCA) {
    ultimoPisca = millis();
    amareloAceso = !amareloAceso;
    digitalWrite(PIN_AMARELO, amareloAceso ? HIGH : LOW);
  }
}

void atualizarSensorEcancela() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duracao = pulseIn(PIN_ECHO, HIGH, 20000);
  long distancia = duracao * 0.034 / 2;

  if (distancia > 0 && distancia <= DISTANCIA_LIMITE_CM) {
    cancela.write(90);
    cancelaAberta = true;
    cancelaAbertaDesde = millis();
  } else if (cancelaAberta && (millis() - cancelaAbertaDesde >= TEMPO_CANCELA_ABERTA)) {
    cancela.write(0);
    cancelaAberta = false;
  }
}
