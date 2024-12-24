#include <user_code.h>
#include <layer3_generic.h>
#include <abstraction.h>
#include <BB.h>
#include <hwLED.h>

#define NUM_PORTS 6 // Define the number of ports for clarity
#define BLINK_COUNT 3
#define BLINK_DELAY 200 // Delay in milliseconds between blinks
#define BLINK_MSG 1 // Define message type for blink
#define STABILIZATION_DELAY 500 // Delay in milliseconds for state stabilization

uint8_t neighborStates[NUM_PORTS] = {0}; // Track the state of all 6 ports (0 = disconnected, 1 = connected)
uint32_t time = 0, treatmentTime = 0;
uint32_t blinkEndTime = 0; // Time until blinking stops
uint8_t blinkCounter = 0; // Count of blinks
uint8_t blinkColor = 0; // Current blinking color
uint8_t blinkInProgress = 0; // Indicates if the block is currently blinking
uint32_t lastEventTime[NUM_PORTS] = {0}; // Track the last event time for each port

enum LED_COLOR {
    WHITE = 0,
    RED = 1,
    GREEN = 2
};

// Function to set the LED to a specific color
void setColorRGB(uint8_t r, uint8_t g, uint8_t b) {
    set_RGB(r, g, b); // Set LED RGB color
}

// Function to set the LED based on predefined colors
void setColor(enum LED_COLOR color) {
    switch (color) {
        case WHITE:
            setColorRGB(255, 255, 255);
            break;
        case RED:
            setColorRGB(255, 0, 0);
            break;
        case GREEN:
            setColorRGB(0, 255, 0);
            break;
        default:
            setColorRGB(255, 255, 255); // Default to WHITE
            break;
    }
}

// Function to broadcast a blink message to all neighbors
void broadcastBlink(uint8_t color) {
    uint8_t data[2] = {BLINK_MSG, color};
    for (uint8_t port = 0; port < NUM_PORTS; port++) {
        if (is_connected(port)) {
            sendMessage(port, data, 2, 1);
        }
    }
}

// Start blinking with a specific color
void startBlinking(uint8_t color) {
    blinkCounter = BLINK_COUNT * 2; // Total on/off cycles
    blinkEndTime = HAL_GetTick() + BLINK_DELAY; // Start blinking immediately
    blinkColor = color; // Set the color for blinking
    blinkInProgress = 1; // Indicate that blinking is in progress
    setColor(color); // Set the initial color
}

void BBinit() {
    setColor(WHITE); // Default color is WHITE
    treatmentTime = HAL_GetTick() + 1000;
}

void BBloop() {
    time = HAL_GetTick();
    if (time > treatmentTime) {
        treatmentTime = time + 400;

        // Handle blinking logic
        if (blinkCounter > 0 && time > blinkEndTime) {
            // Alternate the LED state
            if (blinkCounter % 2 == 1) {
                setColor(blinkColor); // Blink with the current color
            } else {
                setColor(WHITE); // Turn off LED during off phase
            }

            blinkCounter--;
            blinkEndTime = time + BLINK_DELAY;

            // Stop blinking after finishing all blinks
            if (blinkCounter == 0) {
                setColor(WHITE); // Reset to default color
                blinkInProgress = 0; // Reset blinking state
            }
            return; // Skip other processing while blinking
        }

        // Check each port for changes in connection status
        for (uint8_t port = 0; port < NUM_PORTS; port++) {
            uint8_t isConnected = is_connected(port); // Get the current connection status

            // If the connection status has changed and stabilized
            if (neighborStates[port] != isConnected &&
                time > lastEventTime[port] + STABILIZATION_DELAY) {
                neighborStates[port] = isConnected;
                lastEventTime[port] = time; // Update event time
                broadcastBlink(isConnected ? GREEN : RED); // Broadcast green for added, red for removed
                startBlinking(isConnected ? GREEN : RED); // Blink locally
                break; // Only handle one port change at a time
            }
        }
    }
}

// Handle incoming messages
uint8_t process_standard_packet(L3_packet *packet) {
    if (packet == NULL) return 1; // Handle null packets

    // Parse the received message
    uint8_t messageType = packet->packet_content[0];
    if (messageType == BLINK_MSG) {
        uint8_t color = packet->packet_content[1];
        if (!blinkInProgress) { // Only start blinking if not already in progress
            startBlinking(color); // Start blinking with the received color
            broadcastBlink(color); // Forward the message to neighbors
        }
    }

    return 0;
}
