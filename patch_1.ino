// Display Codes Draft

#include "Arduino_GigaDisplay_GFX.h"
#include <Fonts/FreeMonoBold24pt7b.h>  // Use a larger monospaced font

// Initialize the display
GigaDisplay_GFX tft;

// Define colors
#define WHITE   0xFFFF
#define BLUE    0x001F

void setup() {
  tft.begin();                  // Initialize the display
  tft.fillScreen(WHITE);        // Set the background to white
  tft.setRotation(1);           // Rotate for wider screen orientation

  // Call the animation effect here
  displayWritingEffect();       // Display the animation once
}

void loop() {
  // Leave the loop empty to avoid repeating the animation
}

// Function to display the writing effect
void displayWritingEffect() {
  // Step 1: Fill the background with white
  tft.fillScreen(WHITE);

  // Step 2: Load a larger monospaced font
  tft.setFont(&FreeMonoBold24pt7b);  // Use larger monospaced font for equal spacing
  tft.setTextColor(BLUE);            // Set text color to blue

  // Define the text lines
  String line1 = "C A R O L I N A";
  String line2 = "S I G N S  &  W O N D E R S";

  // Calculate vertical positions for centering with extra spacing
  int lineHeight = 40;                // Approximate height of the font in pixels
  int centerY = tft.height() / 2;     // Vertical center of the screen
  int y1 = centerY - (lineHeight / 2); // Position for the first line
  int y2 = centerY + (lineHeight / 2) + 20; // Add 20 pixels for spacing between the lines

  // Display "CAROLINA"
  displayTypingEffect(line1, tft.width() / 2, y1, 24);  // Adjust spacing for "CAROLINA"

  // Pause before displaying the next line
  delay(500);

  // Display "SIGNS & WONDERS"
  displayTypingEffect(line2, tft.width() / 2, y2, 24);  // Adjust spacing for "SIGNS & WONDERS"
}

// Function to create typing effect with equal spacing
void displayTypingEffect(String text, int centerX, int y, int charSpacing) {
  // Calculate starting position for center alignment
  int textWidth = text.length() * charSpacing;
  int startX = centerX - textWidth / 2;

  // Print each character with fixed spacing
  for (int i = 0; i < text.length(); i++) {
    tft.setCursor(startX, y);
    tft.print(text[i]);
    startX += charSpacing;  // Move cursor by fixed spacing
    delay(150);             // Typing delay
  }
}
