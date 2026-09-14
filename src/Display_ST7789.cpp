#include "Display_ST7789.h"
   
#define SPI_WRITE(_dat)         SPI.transfer(_dat)
#define SPI_WRITE_Word(_dat)    SPI.transfer16(_dat)

void SPI_Init()
{
  SPI.begin(EXAMPLE_PIN_NUM_SCLK, EXAMPLE_PIN_NUM_MISO, EXAMPLE_PIN_NUM_MOSI); 
}

void LCD_WriteCommand(uint8_t Cmd)  
{ 
  SPI.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, LOW); 
  SPI_WRITE(Cmd);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);  
  SPI.endTransaction();
}

void LCD_WriteData(uint8_t Data) 
{ 
  SPI.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, HIGH);  
  SPI_WRITE(Data);  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);  
  SPI.endTransaction();
}    

void LCD_Reset(void)
{
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);       
  delay(50);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_RST, LOW); 
  delay(50);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_RST, HIGH); 
  delay(50);
}

void Backlight_Init(void)
{
  pinMode(EXAMPLE_PIN_NUM_BK_LIGHT, OUTPUT);
  digitalWrite(EXAMPLE_PIN_NUM_BK_LIGHT, HIGH);
}

void Set_Backlight(uint8_t Light)
{
  pinMode(EXAMPLE_PIN_NUM_BK_LIGHT, OUTPUT);
  digitalWrite(EXAMPLE_PIN_NUM_BK_LIGHT, Light > 0 ? HIGH : LOW);
}

void LCD_Init(void)
{
  pinMode(EXAMPLE_PIN_NUM_LCD_CS, OUTPUT);
  pinMode(EXAMPLE_PIN_NUM_LCD_DC, OUTPUT);
  pinMode(EXAMPLE_PIN_NUM_LCD_RST, OUTPUT); 
  Backlight_Init();
  SPI_Init();

  LCD_Reset();
  
  // Official initialization sequence for Waveshare 1.47 ST7789
  LCD_WriteCommand(0x11);
  delay(120);
  
  LCD_WriteCommand(0x36);
  LCD_WriteData(0x00); // Portrait mode

  LCD_WriteCommand(0x3A);
  LCD_WriteData(0x05); // 16-bit 565 format

  LCD_WriteCommand(0xB0);
  LCD_WriteData(0x00);
  LCD_WriteData(0xE8);
  
  LCD_WriteCommand(0xB2);
  LCD_WriteData(0x0C);
  LCD_WriteData(0x0C);
  LCD_WriteData(0x00);
  LCD_WriteData(0x33);
  LCD_WriteData(0x33);

  LCD_WriteCommand(0xB7);
  LCD_WriteData(0x35);

  LCD_WriteCommand(0xBB);
  LCD_WriteData(0x35);

  LCD_WriteCommand(0xC0);
  LCD_WriteData(0x2C);

  LCD_WriteCommand(0xC2);
  LCD_WriteData(0x01);

  LCD_WriteCommand(0xC3);
  LCD_WriteData(0x13);

  LCD_WriteCommand(0xC4);
  LCD_WriteData(0x20);

  LCD_WriteCommand(0xC6);
  LCD_WriteData(0x0F);

  LCD_WriteCommand(0xD0);
  LCD_WriteData(0xA4);
  LCD_WriteData(0xA1);

  LCD_WriteCommand(0xD6);
  LCD_WriteData(0xA1);

  LCD_WriteCommand(0xE0);
  LCD_WriteData(0xF0);
  LCD_WriteData(0x00);
  LCD_WriteData(0x04);
  LCD_WriteData(0x04);
  LCD_WriteData(0x04);
  LCD_WriteData(0x05);
  LCD_WriteData(0x29);
  LCD_WriteData(0x33);
  LCD_WriteData(0x3E);
  LCD_WriteData(0x38);
  LCD_WriteData(0x12);
  LCD_WriteData(0x12);
  LCD_WriteData(0x28);
  LCD_WriteData(0x30);

  LCD_WriteCommand(0xE1);
  LCD_WriteData(0xF0);
  LCD_WriteData(0x07);
  LCD_WriteData(0x0A);
  LCD_WriteData(0x0D);
  LCD_WriteData(0x0B);
  LCD_WriteData(0x07);
  LCD_WriteData(0x28);
  LCD_WriteData(0x33);
  LCD_WriteData(0x3E);
  LCD_WriteData(0x36);
  LCD_WriteData(0x14);
  LCD_WriteData(0x14);
  LCD_WriteData(0x29);
  LCD_WriteData(0x32);

  LCD_WriteCommand(0x21); // Invert display

  LCD_WriteCommand(0x11);
  delay(120);
  LCD_WriteCommand(0x29); // Display ON
}

void LCD_SetCursor(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
{ 
  LCD_WriteCommand(0x2A);
  LCD_WriteData((Xstart + Offset_X) >> 8);
  LCD_WriteData((Xstart + Offset_X) & 0xFF);
  LCD_WriteData((Xend + Offset_X) >> 8);
  LCD_WriteData((Xend + Offset_X) & 0xFF);
  
  LCD_WriteCommand(0x2B);
  LCD_WriteData((Ystart + Offset_Y) >> 8);
  LCD_WriteData((Ystart + Offset_Y) & 0xFF);
  LCD_WriteData((Yend + Offset_Y) >> 8);
  LCD_WriteData((Yend + Offset_Y) & 0xFF);

  LCD_WriteCommand(0x2C);
}

void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, const uint16_t* color)
{          
  uint16_t Show_Width = Xend - Xstart + 1;
  uint16_t Show_Height = Yend - Ystart + 1;
  uint32_t numBytes = Show_Width * Show_Height * sizeof(uint16_t);
  
  LCD_SetCursor(Xstart, Ystart, Xend, Yend);
  
  SPI.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, HIGH);  
  SPI.writeBytes((const uint8_t*)color, numBytes);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);  
  SPI.endTransaction();
}

void LCD_Clear(uint16_t Color)
{
  uint16_t line[LCD_WIDTH];
  // Swap bytes for big-endian ST7789 transmission
  uint16_t be_color = (Color >> 8) | (Color << 8);
  for (int i = 0; i < LCD_WIDTH; i++) line[i] = be_color;
  for (int y = 0; y < LCD_HEIGHT; y++) {
    LCD_addWindow(0, y, LCD_WIDTH - 1, y, line);
  }
}
