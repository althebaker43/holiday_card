
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

#define CLK_IO 1000000
#define PRESCALE 1
#define FREQ_TO_COUNT(freq) ((CLK_IO / (2 * freq * PRESCALE)) - 1)

#define BEAT_DIV 3
#define SEQ_PRESCALE 1024
#define BPM_TO_COUNT(bpm) ((CLK_IO * (uint32_t)60) / (2 * BEAT_DIV * (uint32_t)(SEQ_PRESCALE + 1) * bpm))

#define MAX_NUM_NOTES 16
struct Note {
  uint8_t code;
  uint8_t count;
};
struct Note Notes [MAX_NUM_NOTES];
static unsigned int CurrentNoteIdx = 0;
#define END_SEQ {0, 0}

enum Codes {
  CODE_NONE = 0,
  CODE_A3,
  CODE_Bflat,
  CODE_B,
  CODE_C,
  CODE_Csharp,
  CODE_D,
  CODE_Dsharp,
  CODE_E,
  CODE_F,
  CODE_Fsharp,
  CODE_G,
  CODE_Gsharp,
  CODE_A4
};


void toneTimerSetup()
{
  // Connect pin PB1 to timer 1
  set(DDRB, DDB1);

  // Clear timer 1 on compare match
  set(TCCR1B, WGM12);

  // Set pin PC0 as output
  set(DDRC, DDC0);

  // Toggle timer 1 output on compare match
  set(TCCR1A, COM1A0);
}

void silence()
{
  // Disable speaker
  clear(PORTC, PORTC0);

  // Disconnect timer output
  clear(TCCR1A, COM1A0);

  // Deactivate timer 1
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
      return;
    }

  uint16_t count = keyToneMap[keyCode];

  // Set compare value for timer 1
  OCR1AH = (count >> 8) & 0xFF;
  OCR1AL = count & 0xFF;

  // Clear timer 1 on compare match
  set(TCCR1B, WGM12);

  // Toggle timer 1 output on compare match
  set(TCCR1A, COM1A0);

  // Activate timer 1 (no clock scaling)
  set(TCCR1B, CS10);

  // Enable speaker
  set(PORTC, PORTC0);
}

void ledSetup()
{
  // Configure PB0, PD5-7 as outputs
  set(DDRB, DDB0);
  set(DDRD, DDD5);
  set(DDRD, DDD6);
  set(DDRD, DDD7);
}

void toggleLEDs(unsigned int keyCode)
{
  const int NUM_LEDS = 4;
  int ledIdx = 0;
  for (ledIdx = 0; ledIdx < NUM_LEDS; ++ledIdx)
    {
      if (keyCode & (1 << ledIdx))
	{
	  switch (ledIdx)
	    {
	    case 0: set(PORTD, PORTD5); break;
	    case 1: set(PORTD, PORTD6); break;
	    case 2: set(PORTD, PORTD7); break;
	    case 3: set(PORTB, PORTB0); break;
	    default: break;
	    };
	}
      else
	{
	  switch (ledIdx)
	    {
	    case 0: clear(PORTD, PORTD5); break;
	    case 1: clear(PORTD, PORTD6); break;
	    case 2: clear(PORTD, PORTD7); break;
	    case 3: clear(PORTB, PORTB0); break;
	    default: break;
	    };
	}
    }
}

void enableKeyInterrupts()
{
  // Enable interrupts on PCINT16-19
  set(PCMSK2, PCINT16);
  set(PCMSK2, PCINT17);
  set(PCMSK2, PCINT18);
  set(PCMSK2, PCINT19);
}

void disableKeyInterrupts()
{
  clear(PCMSK2, PCINT16);
  clear(PCMSK2, PCINT17);
  clear(PCMSK2, PCINT18);
  clear(PCMSK2, PCINT19);
}

void keySetup()
{
  // Enable pull-ups on PD0-3
  set(PORTD, PORTD0);
  set(PORTD, PORTD1);
  set(PORTD, PORTD2);
  set(PORTD, PORTD3);

  enableKeyInterrupts();

  // Enable interrupts on any pin change
  set(PCICR, PCIE2);
}

unsigned int scanKeys()
{
#ifndef HANSON_CARD
  return (((test(PIND, PIND3) ? 0 : 1) << 3) |
  	  ((test(PIND, PIND2) ? 0 : 1) << 2) |
  	  ((test(PIND, PIND1) ? 0 : 1) << 1) |
  	  ((test(PIND, PIND0) ? 0 : 1) << 0));
#else
  return (((test(PIND, PIND2) ? 0 : 1) << 3) |
  	  ((test(PIND, PIND1) ? 0 : 1) << 2) |
  	  ((test(PIND, PIND0) ? 0 : 1) << 1) |
  	  ((test(PIND, PIND3) ? 0 : 1) << 0));
#endif
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

void sequenceTimerSetup()
{
  // Clear timer 0 on compare match
  set(TCCR0A, WGM01);

  // Set timer 0 output on compare match
  /* set(TCCR0A, COM0A0); */
  /* set(TCCR0A, COM0A1); */

  // Enable interrupts on compare match
  set(TIMSK0, OCIE0A);

  // Set timer 0 compare value
  OCR0A = (uint8_t)BPM_TO_COUNT(100);
}

void initSequence(struct Note sequence[])
{
  disableKeyInterrupts();

  unsigned int initNoteIdx = 0;
  for (; sequence[initNoteIdx].count == 0; ++initNoteIdx)
    {
      Notes[initNoteIdx] = sequence[initNoteIdx];
    }

  CurrentNoteIdx = 0;

  tone(Notes[CurrentNoteIdx].code);
  toggleLEDs(Notes[CurrentNoteIdx].code);

  // Clear timer 0 count
  TCNT0 = 0;

  // Activate timer 0 (with prescaler set to 1024)
  TCCR0B |= ((1 << CS00 ) | (1 << CS02));
}

void onBeatDiv()
{
  if (Notes[CurrentNoteIdx].count > 0)
    {
      --(Notes[CurrentNoteIdx].count);
      return;
    }

  ++CurrentNoteIdx;

  if (Notes[CurrentNoteIdx].count == 0)
    {
      // Deactivate timer 0
      TCCR0B &= ~((1 << CS00) | (1 << CS01) | (1 << CS02));

      tone(0);
      toggleLEDs(0);

      enableKeyInterrupts();

      return;
    }

  tone(Notes[CurrentNoteIdx].code);
  toggleLEDs(Notes[CurrentNoteIdx].code);
}

ISR(TIMER0_COMPA_vect)
{
  onBeatDiv();
  reti();
}

int main()
{
  cli();

  toneTimerSetup();

  keySetup();

  ledSetup();

  sequenceTimerSetup();

  sei();

  //initSequence((struct Note[]){{1 << 0, 1}, {1 << 1, 1}, {1 << 2, 1}, {1 << 3, 1}, END_SEQ});

  sleep_mode();

  return 0;
}
