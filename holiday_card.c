
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#ifdef set
#error "\"set\" already defined"
#else
#define set(reg, bit) (reg |= 1 << bit)
#endif

#ifdef clear
#error "\"clear\" already defined"
#else
#define clear(reg, bit) (reg &= ~(1 << bit))
#endif

#ifdef test
#error "\"test\" already defined"
#else
#define test(reg, bit) (reg & (1 << bit))
#endif

//const static uint16_t CLK_IO = 1000000;
//const static uint8_t PRESCALE = 1;
#define CLK_IO 1000000
#define PRESCALE 1
#define FREQ_TO_COUNT(freq) ((CLK_IO / (2 * freq * PRESCALE)) - 1)


void toneTimerSetup()
{
  // Connect pin PB1 to timer 1
  set(DDRB, DDB1);

  // Clear timer 1 on compare match
  clear(TCCR1B, WGM13);
  set(TCCR1B, WGM12);
  clear(TCCR1A, WGM11);
  clear(TCCR1A, WGM10);

  // Set pin PC0 as output
  set(DDRC, DDC0);

  // Set default tone
  OCR1AH = 2271 >> 8;
  OCR1AL = 2271 & 0xFF;
}

void silence()
{
  // Disable speaker
  clear(PORTC, PORTC0);

  // Disconnect timer output
  clear(TCCR1A, COM1A0);
  clear(TCCR1A, COM1A1);

  // Deactivate timer 1
  clear(TCCR1B, CS12);
  clear(TCCR1B, CS11);
  clear(TCCR1B, CS10);
}

void tone(unsigned int keyCode)
{
  const static int MAX_KEY = 13;
  const uint16_t keyToneMap [] = {
    0,
    FREQ_TO_COUNT(220), // A3
    FREQ_TO_COUNT(233.0),
    FREQ_TO_COUNT(246.94),
    FREQ_TO_COUNT(261.63),
    FREQ_TO_COUNT(277.18),
    FREQ_TO_COUNT(293.66),
    FREQ_TO_COUNT(311.13),
    FREQ_TO_COUNT(329.63),
    FREQ_TO_COUNT(349.23),
    FREQ_TO_COUNT(369.99),
    FREQ_TO_COUNT(392),
    FREQ_TO_COUNT(415.3),
    FREQ_TO_COUNT(440) // A4
  };

  if (keyCode > MAX_KEY)
    {
      return;
    }

  if (keyCode == 0)
    {
      silence();
    }

  uint16_t count = keyToneMap[keyCode];

  // Set compare value for timer 1
  OCR1AH = count >> 8;
  OCR1AL = count & 0xFF;

  // Activate timer 1 (no clock scaling)
  clear(TCCR1B, CS12);
  clear(TCCR1B, CS11);
  set(TCCR1B, CS10);

  // Enable speaker
  set(PORTC, PORTC0);

  // Toggle timer 1 output on compare match
  set(TCCR1A, COM1A0);
}

void ledSetup()
{
  // Configure PD0-3 as outputs
  set(DDRD, DDD0);
  set(DDRD, DDD1);
  set(DDRD, DDD2);
  set(DDRD, DDD3);
}

void toggleLEDs(unsigned int keyCode)
{
  const int NUM_LEDS = 4;
  const uint8_t keyLEDMap [] = {
    PIND3,
    PIND2,
    PIND1,
    PIND0
  };

  int ledIdx = 0;
  for (ledIdx = 0; ledIdx < NUM_LEDS; ++ledIdx)
    {
      uint8_t ledPin = keyLEDMap[ledIdx];
      if (keyCode & (1 << ledIdx))
	{
	  set(PIND, ledPin);
	}
      else
	{
	  clear(PIND, ledPin);
	}
    }
}

void keySetup()
{
  // Enable pull-ups on PB0, PD5-7
  set(PORTB, PORTB0);
  set(PORTD, PORTD5);
  set(PORTD, PORTD6);
  set(PORTD, PORTD7);

  // Enable interrupts on PCINT0,21-23
  set(PCMSK0, PCINT0);
  set(PCMSK2, PCINT21);
  set(PCMSK2, PCINT22);
  set(PCMSK2, PCINT23);

  // Enable interrupts on any pin change
  set(PCICR, PCIE0);
  set(PCICR, PCIE2);
}

unsigned int scanKeys()
{
  return (((test(PORTD, PORTD5) ? 1 : 0) << 3) |
	  ((test(PORTD, PORTD6) ? 1 : 0) << 2) |
	  ((test(PORTD, PORTD7) ? 1 : 0) << 1) |
	  ((test(PORTB, PORTB0) ? 1 : 0) << 0));
}

void onKeyChange()
{
  unsigned int keyCode = scanKeys();

  tone(keyCode);

  toggleLEDs(keyCode);
}

ISR(PCINT0_vect)
{
  onKeyChange();
  reti();
}

ISR(PCINT1_vect)
{
  onKeyChange();
  reti();
}

ISR(PCINT2_vect)
{
  onKeyChange();
  reti();
}

int main()
{
  cli();

  toneTimerSetup();

  keySetup();

  ledSetup();

  sei();

  sleep_mode();

  return 0;
}
