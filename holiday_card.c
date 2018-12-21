
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

void blinkSetup()
{
  // Connect pin PD6 to timer 0
  set(DDRD, DDD6);

  // Set a 0.5-second period on timer 0
  OCR0A = 0xFF;

  // Toggle timer 0 output on compare match
  clear(TCCR0A, COM0A1);
  set(TCCR0A, COM0A0);

  // Activate timer 0 and scale clkIO by 1024
  set(TCCR0B, CS02);
  clear(TCCR0B, CS01);
  set(TCCR0B, CS00);
}

void flatToneSetup()
{
  // Connect pin PB3 to timer 2
  set(DDRB, DDD3);

  // Set a ~2kHz frequency
  OCR2A = 8;

  // Toggle timer 2 output on compare match
  clear(TCCR2A, COM2A1);
  set(TCCR2A, COM2A0);

  // Clear timer 2 on compare match
  clear(TCCR2B, WGM22);
  set(TCCR2A, WGM21);
  clear(TCCR2A, WGM20);

  // Activate timer 2 and scale clkIO by 32
  clear(TCCR2B, CS22);
  set(TCCR2B, CS21);
  set(TCCR2B, CS20);
}

void toneTimerSetup()
{
  // Connect pin PB1 to timer 1
  set(DDRB, DDB1);

  // Clear timer 1 on compare match
  clear(TCCR1B, WGM13);
  set(TCCR1B, WGM12);
  clear(TCCR1A, WGM11);
  clear(TCCR1A, WGM10);

  // Set pin PD4 as output
  set(DDRD, DDD4);

  // Set default tone
  OCR1AH = 2271 >> 8;
  OCR1AL = 2271 & 0xFF;
}

void tone(count)
{
  // Set compare value for timer 1
  OCR1AH = count >> 8;
  OCR1AL = count & 0xFF;

  // Activate timer 1 (no clock scaling)
  clear(TCCR1B, CS12);
  clear(TCCR1B, CS11);
  set(TCCR1B, CS10);

  // Enable speaker
  set(PORTD, PORTD4);

  // Toggle timer 1 output on compare match
  set(TCCR1A, COM1A0);
}

int isTuned()
{
  return test(PORTD, PORTD4);
}

void silence()
{
  // Disable speaker
  clear(PORTD, PORTD4);

  // Disconnect timer output
  clear(TCCR1A, COM1A0);
  clear(TCCR1A, COM1A1);

  // Deactivate timer 1
  clear(TCCR1B, CS12);
  clear(TCCR1B, CS11);
  clear(TCCR1B, CS10);
}

void toggleTone()
{
  if (isTuned())
    {
      silence();
    }
  else
    {
      tone(2271);
    }
}

void ledSetup()
{
  // Configure PD0-3 as outputs
  set(DDRD, DDD0);
  set(DDRD, DDD1);
  set(DDRD, DDD2);
  set(DDRD, DDD3);
}

void toggleLEDs()
{
  set(PIND, PIND0);
  set(PIND, PIND1);
  set(PIND, PIND2);
  set(PIND, PIND3);
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

void onKeyChange()
{
  toggleTone();
  toggleLEDs();
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
