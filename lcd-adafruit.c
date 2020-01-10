#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <wiringPi.h>
#include <mcp23017.h>
#include <lcd.h>

#define	AF_BASE		100
#define	AF_RED		(AF_BASE + 6)
#define	AF_GREEN	(AF_BASE + 7)
#define	AF_BLUE		(AF_BASE + 8)

#define	AF_E		  (AF_BASE + 13)
#define	AF_RW		  (AF_BASE + 14)
#define	AF_RS		  (AF_BASE + 15)

#define	AF_DB4		(AF_BASE + 12)
#define	AF_DB5		(AF_BASE + 11)
#define	AF_DB6		(AF_BASE + 10)
#define	AF_DB7		(AF_BASE +  9)

#define	AF_SELECT	(AF_BASE +  0)
#define	AF_RIGHT	(AF_BASE +  1)
#define	AF_DOWN		(AF_BASE +  2)
#define	AF_UP		  (AF_BASE +  3)
#define	AF_LEFT		(AF_BASE +  4)

static unsigned char newChar [8] = {
  0b00100,
  0b00100,
  0b00000,
  0b00100,
  0b01110,
  0b11011,
  0b11011,
  0b10001,
};

static int lcdHandle;

int usage (const char *progName) {
  fprintf (stderr, "Usage: %s colour\n", progName);
  return EXIT_FAILURE;
}

static const char *message =
  "                    "
  "Wiring Pi by Gordon Henderson. HTTP://WIRINGPI.COM/"
  "                    ";

void scrollMessage(int line, int width){
  char buf [32];
  static int position = 0;
  static int timer = 0;

  if (millis() < timer)
    return;

  timer = millis() + 200;

  strncpy (buf, &message [position], width);
  buf[width] = 0;
  lcdPosition(lcdHandle, 0, line);
  lcdPuts(lcdHandle, buf);

  if (++position == (strlen(message) - width))
    position = 0;
}

static void setBacklightColour(int colour) {
  colour &= 7;

  digitalWrite(AF_RED,   !(colour & 1));
  digitalWrite(AF_GREEN, !(colour & 2));
  digitalWrite(AF_BLUE,  !(colour & 4));
}

static void adafruitLCDSetup(int colour) {
  int i;

  pinMode(AF_RED,   OUTPUT);
  pinMode(AF_GREEN, OUTPUT);
  pinMode(AF_BLUE,  OUTPUT);
  setBacklightColour (colour);

  for (i = 0 ; i <= 4 ; ++i) {
    pinMode(AF_BASE + i, INPUT);
    pullUpDnControl(AF_BASE + i, PUD_UP) ;	// Enable pull-ups, switches close to 0v
  }

  pinMode(AF_RW, OUTPUT);
  digitalWrite(AF_RW, LOW);	// Not used with wiringPi - always in write mode

  /* 4-bit mode */
  lcdHandle = lcdInit(
    2,      /* rows          */
    16,     /* cols          */
    4,      /* bits          */
    AF_RS,  /* RS            */
    AF_E,   /* Strobe/Enable */
    AF_DB4, /* D0            */
    AF_DB5, /* D1            */
    AF_DB6, /* D2            */
    AF_DB7, /* D3            */
    0,      /* D4 not used   */
    0,      /* D5 not used   */
    0,      /* D6 not used   */
    0       /* D7 not used   */
  );

  if (lcdHandle < 0) {
    fprintf(stderr, "lcdInit failed\n");
    exit(EXIT_FAILURE);
  }
}

static void waitForEnter(void) {
  printf("Press SELECT to continue: ") ; fflush (stdout);

  while (digitalRead(AF_SELECT) == HIGH)	// Wait for push
    delay(1);

  while (digitalRead(AF_SELECT) == LOW)	// Wait for release
    delay(1) ;

  printf("OK\n") ;
}

static void speedTest(void) {
  unsigned int start, end, taken;
  int times;

  lcdClear(lcdHandle);
  start = millis();
  for (times = 0 ; times < 10 ; ++times) {
    lcdPuts(lcdHandle, "0123456789ABCDEF");
    lcdPuts(lcdHandle, "0123456789ABCDEF");
  }
  end   = millis();
  taken = (end - start) / 10;

  lcdClear(lcdHandle);
  lcdPosition(lcdHandle, 0, 0); 
  lcdPrintf(lcdHandle, "Speed: %dmS", taken);
  lcdPosition(lcdHandle, 0, 1);
  lcdPrintf(lcdHandle, "For full update");

  waitForEnter();

  lcdClear(lcdHandle);
  lcdPosition(lcdHandle, 0, 0);
  lcdPrintf(lcdHandle, "Time: %dmS", taken / 32);
  lcdPosition(lcdHandle, 0, 1);
  lcdPrintf(lcdHandle, "Per character");

  waitForEnter();

  lcdClear(lcdHandle);
  lcdPosition(lcdHandle, 0, 0);
  lcdPrintf(lcdHandle, "%d cps...", 32000 / taken);

  waitForEnter();
}

int main (int argc, char *argv[]) {
  int        colour;
  int        cols = 16;
  int        waitForRelease = FALSE;
  struct tm *t;
  time_t     tim;
  char       buf[32];

  if (argc != 2)
    return usage(argv [0]);

  printf("Raspberry Pi Adafruit LCD test\n");
  printf("==============================\n");

  colour = atoi(argv [1]);

  wiringPiSetupSys();
  mcp23017Setup(AF_BASE, 0x20);

  adafruitLCDSetup(colour);

  lcdPosition(lcdHandle, 0, 0);
  lcdPuts(lcdHandle, "Gordon Henderson");
  lcdPosition(lcdHandle, 0, 1);
  lcdPuts(lcdHandle, "  wiringpi.com  ");

  waitForEnter();

  lcdPosition(lcdHandle, 0, 1);
  lcdPuts(lcdHandle, "Adafruit RGB LCD");

  waitForEnter();

  lcdCharDef(lcdHandle, 2, newChar);

  lcdClear(lcdHandle);
  lcdPosition(lcdHandle, 0, 0);
  lcdPuts(lcdHandle, "User Char: ");
  lcdPutchar(lcdHandle, 2);

  lcdCursor(lcdHandle, TRUE);
  lcdCursorBlink(lcdHandle, TRUE);

  waitForEnter();

  lcdCursor(lcdHandle, FALSE);
  lcdCursorBlink(lcdHandle, FALSE);

  speedTest();

  lcdClear(lcdHandle);

  for (;;) {
    scrollMessage(0, cols);
    
    tim = time(NULL);
    t = localtime(&tim);
    sprintf(buf, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);

    lcdPosition(lcdHandle, (cols - 8) / 2, 1);
    lcdPuts(lcdHandle, buf);

    // Check buttons to cycle colour

    // If Up or Down are still pushed, then skip

    if (waitForRelease) {
      if ((digitalRead(AF_UP) == LOW) || (digitalRead(AF_DOWN) == LOW))
	      continue;
      else
	      waitForRelease = FALSE;
    }

    if (digitalRead(AF_UP) == LOW)	{ // Pushed
      colour = colour + 1;
      if (colour == 8)
	      colour = 0;
      setBacklightColour(colour);
      waitForRelease = TRUE;
    }

    if (digitalRead(AF_DOWN) == LOW)	{ // Pushed {
      colour = colour - 1;
      if (colour == -1)
	      colour = 7;
      setBacklightColour(colour);
      waitForRelease = TRUE;
    }
  }

  return 0;
}

/*
# See http://wiringpi.com/examples/adafruit-rgb-lcd-plate-and-wiringpi/

gpio load i2c
gpio i2cd
# MCP23017 should be add default address 0x20

# Test Red, green and blue LEDs
gpio -x mcp23017:100:0x20 mode 106 out # Red
gpio -x mcp23017:100:0x20 write 106 0
gpio -x mcp23017:100:0x20 mode 107 out # Green
gpio -x mcp23017:100:0x20 write 107 0
gpio -x mcp23017:100:0x20 mode 108 out # Blue
gpio -x mcp23017:100:0x20 write 108

*/
