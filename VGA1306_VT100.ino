//============================================
// adapted from the code by smeezekitty at:
//
// https://www.youtube.com/watch?v=VEhVO7zY0RI
//============================================

#define UNDERLINE 4
#define INVERSE 2
#define BLINK 1

#define BUFFERSIZE 4800
#define COLUMNS 80
#define ROWS 60

const uint8_t bitMask[8] = {1, 2, 4, 8, 16, 32, 64, 128};
char text[BUFFERSIZE]; //Text buffer
// char attrib[BUFFERSIZE/2]; //Text attribute buffer, 4 bits per character
#define TEXT(x,y) text[((y) * COLUMNS) + (x)]
int cx,cy;
unsigned char c;
char cur_atr;

char blocking_read(){
  while(!Serial.available());
  return Serial.read();
}

void scrollup(){
  memmove(text, text+COLUMNS, BUFFERSIZE-COLUMNS); //Copy all but last line of text up
// memmove(attrib, attrib+(COLUMNS/2), (BUFFERSIZE-COLUMNS)/2); //Copy all but last line of text atttribute up
  memset(text+(BUFFERSIZE-COLUMNS), 0x20, COLUMNS); //Clear last line
}

void scrolldn(){
  memmove(text+COLUMNS, text, BUFFERSIZE-COLUMNS); //Copy page of text down
// memmove(attrib+(COLUMNS/2), attrib, (BUFFERSIZE-COLUMNS)/2); //Copy page of atrributes down
}

void handle_escape(){
  c = blocking_read();
  byte x,val;
  if(c == 'D'){ //Move down one line
    cy++;
    if(cy > (ROWS-1)){
      scrollup();
      cy = ROWS-1;
    }
  }
  if(c == 'M'){ //Move up one line
    cy--;
    if(cy < 0){cy = 0;}
  }
  if(c == 'E'){ //Move to next line
    cy++;
    cx=0;
    if(cy > (ROWS-1)){
      scrollup();
      cy = ROWS-1;
    }
  }
  if(c == '['){
    c = blocking_read();
    val = 255;
    if(isdigit(c)){
      val = c - '0';
      c = blocking_read();
      if(isdigit(c)){
        val *= 10;
        val += c - '0';
        c = blocking_read();
      }
    }
    switch(c){
      case ';':
        int val2;
        val2 = blocking_read() - '0';
        c = blocking_read();
        if(isdigit(c)){
          val2 *= 10;
          val2 += c - '0';
          c = blocking_read();
        }
        if(c == 'f' || c == 'H'){ //Move cursor to screen location v,h
          cy = val-1; cx = val2-1;
          if(cx > (COLUMNS-1)){cx = COLUMNS-1;}
          if(cx < 0){cx = 0;}
          if(cy > (ROWS-1)){cy = ROWS-1;}
          if(cy < 0){cy = 0;}
        }
        break;
      case 'A':cy-=(val==255)?1:val; if(cy < 0){cy = 0;} break; //Move cursor up n lines
      case 'B':cy+=(val==255)?1:val; if(cy > (ROWS-1)){cy = ROWS-1;} break; //Move cursor down n lines
      case 'C':cx+=(val==255)?1:val; if(cx > (COLUMNS-1)){cx = COLUMNS-1;} break; //Move cursor right n lines
      case 'D':cx-=(val==255)?1:val; if(cx < 0){cx = 0;} break; //Move cursor left n lines
      case 'H':cx = 0; cy = 0; break; //Move cursor to upper left corner
      case 'K':
        if(val == 1){ //Clear line from cursor left
          for(x = cx;x;x--){
            TEXT(x,cy) = 0x20;
          }
        }
        if(val == 0 || val == 255){ //Clear line from cursor right
          for(x = cx;x < COLUMNS;x++){
            TEXT(x,cy) = 0x20;
          }
        }
        if(val == 2){
          for(x = 0;x < COLUMNS;x++){ //Clear entire line
            TEXT(x, cy) = 0x20;
          }
        }
        break;
      case 'J':
        if(val == 0 || val == 255){ //Clear screen from cursor down
          for(int i=((cy*COLUMNS)+cx);i < BUFFERSIZE;i++){
            text[i] = 0x20;
          }
        }
        if(val == 1){ //Clear screen from cursor up
          memset(text, 0x20, (cy*COLUMNS)+cx);
        }
        if(val == 2){ //Clear entire screen
          memset(text, 0x20, BUFFERSIZE);
        }
        break;
      case 'm':
        if(val == 0 || val == 255){
          cur_atr = 0;
        }
        if(val == 4){
          cur_atr |= UNDERLINE;
        }
        if(val == 7){
          cur_atr |= INVERSE;
        }
        if(val == 5){
          cur_atr |= BLINK;
        }
        break;
    }
  }
}

void poll_serial(){ //We need to do this often to avoid dropping bytes in the tiny buffer
  while(Serial.available()){ //Check if there is incoming serial and if there is, handle it
    c = Serial.read();
    if(c == 27){handle_escape(); continue;} //Escape
    if(c == '\n' || cx > (COLUMNS-1)){ //Line Feed
      cx = 0; cy ++;
      if(cy > (ROWS-1)){
        scrollup();
        cy = ROWS-1;
      }
    }
    if(c == '\r'){cx = 0;} //Carriage Return
    if(c == 8 && cx > 0){cx--;} //Back Space
    if(c == '\t' && cx < (COLUMNS-9)){cx = cx + 8;} //Tab
    if(c > 31){
      text[(cy * COLUMNS) + cx] = c;
//    attrib[(cy * (COLUMNS/2)) + cx/2] &= cx&1?0xF0:0x0F;
//    attrib[(cy * (COLUMNS/2)) + cx/2] |= cx&1?cur_atr:cur_atr<<4;
      cx++;
    }
  }
}

void update_display(){

  PORTA |=  B00010000; // cs HIGH

  for (int i = 0; i < BUFFERSIZE; i++){

    PORTA |=  B00100000; // dc HIGH

    for (uint8_t b = 0; b < 8; b++){

      if (text[i] & bitMask[b]) PORTA |=  B01000000; // d0
      else                      PORTA &= ~B01000000;

      PORTA |=  B10000000; // wclk HIGH
      PORTA &= ~B10000000; // wclk LOW
    }

    // Latch data
    PORTA &= ~B00100000; // dc LOW
    PORTA |=  B10000000; // wclk HIGH
    PORTA &= ~B10000000; // wclk LOW
  }

  // 'VSYNC'
  PORTA &= ~B00010000; // cs LOW
  PORTA |=  B10000000; // wclk HIGH
  PORTA &= ~B10000000; // wclk LOW
}

void setup(){
  DDRA  |=  B11110000; // OUTPUT
  PORTA &= ~B11110000; // LOW

  Serial.begin(9600);
  memset(text, 0x20, BUFFERSIZE);
// memset(attrib, 0, BUFFERSIZE/2);
}

void loop(){
  poll_serial();
  update_display();
}
