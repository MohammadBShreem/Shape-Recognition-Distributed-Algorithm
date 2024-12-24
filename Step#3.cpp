#include <memory.h>
#include <BB.h>
#include <hwLED.h>
#include <bb_global.h>
#include <serial.h>
#include <layer3_generic.h>
#include <light.h>
#include <stdio.h>  // For printf
#include <stdint.h> // For uint8_t, uint16_t, etc.


#define SETCOOR_MSG 1
#define ACK_MSG 2
#define SHAPE_MSG 3
#define SHAPE_PLUS 4
#define SHAPE_H 5
#define SHAPE_SQUARE 6
#define SHAPE_NONE 7
#define MOVE_MSG 8

uint32_t timeTreatment;
uint8_t currentColor = WHITE;
int16_t x, y;
uint8_t hasSetCoordinates = 0;
uint8_t isPlus = 0;
uint8_t isH = 0;
uint8_t isSecondHNode = 0;
uint8_t isSquare = 0;
uint8_t isSecondSquareNode =0;
uint8_t isThirdSquareNode=0;
uint8_t westResponse = 0;
uint8_t eastResponse = 0;
uint8_t southResponse = 0;
uint8_t northResponse = 0;
uint8_t isShapeDetected = 0;
uint8_t nbrWaitedAnswers =0;

enum direction { NORTH, BOTTOM, WEST, EAST, SOUTH, TOP };

// Message structure for setting coordinates
typedef struct __packed {
    uint8_t type;  // Message type
    int16_t x;     // X coordinate
    int16_t y;     // Y coordinate
} SetCoorMessage;

typedef struct __packed {
    uint8_t type;  // Message type
    uint8_t maxValue;
} AcknowledgmentMessage;

typedef struct __packed {
    uint8_t type;  // Message type
    uint8_t shapeType;
} ShapeMessage;

typedef struct __packed {
    uint8_t type;  // Message type
    uint8_t steps;
    uint8_t checkingType;
} MoveMessage;

// Function prototypes
void updateCoordinatesBasedOnPort(int16_t receivedX, int16_t receivedY, uint8_t port);
void propagateSetCoor(SetCoorMessage *message, uint8_t senderPort);
void startSettingCoordinates();
void processAckMessage(uint8_t maxValue, uint8_t senderPort);
void sendAckMessage(uint8_t maxValue, uint8_t senderPort);
void validatePlusShape();
void validateHShape(uint8_t edge);
void validateSquareShape(uint8_t edge);
void processMoveMessage(uint8_t steps, uint8_t checkingType, uint8_t senderPort);
void sendMoveMessage(uint8_t steps, uint8_t checkingType, uint8_t destPort );

void broadcastShapeMessage(uint8_t shape);

// Initialization
void BBinit() {
    timeTreatment = HAL_GetTick();
    currentColor = WHITE; // Default color

    hasSetCoordinates = 0;    // Tracks if the block has updated coordinates
    isPlus = 0;    // Tracks if the block has updated coordinates
    isH = 0;    // Tracks if the block has updated coordinates
    isSquare = 0;    // Tracks if the block has updated coordinates

    isShapeDetected = 0;
    nbrWaitedAnswers =0;

    isSecondHNode = 0;
    isSquare = 0;
    isSecondSquareNode =0;
    isThirdSquareNode=0;
    westResponse = 0;
    eastResponse = 0;
    southResponse = 0;
    northResponse = 0;

    // Check if the block is connected to TOP and NORTH
    if (is_connected(WEST) && is_connected(NORTH) && is_connected(EAST) && is_connected(SOUTH)) {
    isPlus=1;
        startSettingCoordinates();
    }
    else if (is_connected(WEST) && is_connected(EAST) && is_connected(SOUTH) && !is_connected(NORTH)) {
    isH=1;
        startSettingCoordinates();
    }
    else if (is_connected(NORTH) && is_connected(WEST) && !is_connected(EAST) && !is_connected(SOUTH)){
    isSquare=1;
        startSettingCoordinates();
    }
}

// Main loop
void BBloop() {
    uint32_t time = HAL_GetTick();
    if (time > timeTreatment) {
        timeTreatment = time + 500; // Update every 500ms
        setColor(currentColor);
    }
}

// Starts the coordinate propagation process
void startSettingCoordinates() {


    if (hasSetCoordinates==0){
    hasSetCoordinates=1;
        x=0;
        y=0;
        currentColor=GREEN;
        setColor(currentColor);
    }

    // Create the SETCOOR_MSG packet
    SetCoorMessage message = {SETCOOR_MSG, x, y};

    // Broadcast the message to all connected neighbors
    for (uint8_t p = 0; p < NB_SERIAL_PORT; ++p) {
        if (is_connected(p)) {
            sendMessage(p, (uint8_t*)&message, sizeof(message), 1);
        }
    }
}

// Starts the coordinate propagation process
void startSettingCoordinatesForSecondHNode() {

    // Create the SETCOOR_MSG packet
    SetCoorMessage message = {SETCOOR_MSG, x, y};

    sendMessage(WEST, (uint8_t*)&message, sizeof(message), 1);
    sendMessage(EAST, (uint8_t*)&message, sizeof(message), 1);

}

void startSettingCoordinatesForSecondSquareNode(uint8_t edge) {

    // Create the SETCOOR_MSG packet
    SetCoorMessage message = {SETCOOR_MSG, x, y};
    if(edge==1){
    sendMessage(NORTH, (uint8_t*)&message, sizeof(message), 1);
    }
    else if (edge==2){
        nbrWaitedAnswers=1;

    //sendMessage(EAST, (uint8_t*)&message, sizeof(message), 1);
    uint8_t currentLocationValue;
    if (y<0)currentLocationValue= y*-1;
        else currentLocationValue=y;
sendMoveMessage(currentLocationValue-1,SHAPE_SQUARE,EAST);
    }
}


// Update local coordinates based on the received port direction
void updateCoordinatesBasedOnPort(int16_t receivedX, int16_t receivedY, uint8_t port) {
    switch (port) {
        case NORTH:  // Received from NORTH
            x = receivedX -1;
            y = receivedY;
            break;
        case WEST:  // Received from WEST
            x = receivedX;
            y = receivedY -1;
            break;
        case EAST:  // Received from EAST
            x = receivedX;
            y = receivedY +1;
            break;
        case SOUTH:  // Received from SOUTH
            x = receivedX +1;
            y = receivedY;
            break;
        default:
            break; // Unknown direction
    }
}

// Propagate the SETCOOR_MSG to other neighbors except the sender
// Propagate the SETCOOR_MSG to specific neighbors based on sender port
void propagateSetCoor(SetCoorMessage * message, uint8_t senderPort) {
  uint8_t maxValue = 0;
  switch (senderPort) {
  case NORTH: {
    if (is_connected(SOUTH)) {
      nbrWaitedAnswers++;
      sendMessage(SOUTH, (uint8_t * ) message, sizeof(SetCoorMessage), 1);
    } else {
      if (x < 0) maxValue = x * -1;
      else maxValue = x;
      sendAckMessage(maxValue, NORTH);
    }
    break;
  }
  case WEST: {
    if (is_connected(EAST)) {
      nbrWaitedAnswers++;
      sendMessage(EAST, (uint8_t * ) message, sizeof(SetCoorMessage), 1);
    } else {
      if (y < 0) maxValue = y * -1;
      else maxValue = y;
      sendAckMessage(maxValue, WEST);
    }
    break;
  }
  case EAST: {
    if (is_connected(WEST)) {
      nbrWaitedAnswers++;
      sendMessage(WEST, (uint8_t * ) message, sizeof(SetCoorMessage), 1);
    } else {
      if (y < 0) maxValue = y * -1;
      else maxValue = y;
      sendAckMessage(maxValue, EAST);
    }
    break;
  }
  case SOUTH: {
    if (is_connected(NORTH)) {
      nbrWaitedAnswers++;
      sendMessage(NORTH, (uint8_t * ) message, sizeof(SetCoorMessage), 1);
    } else {
      if (x < 0) {
        maxValue = x * -1;
      } else {
        maxValue = x;
      }
      sendAckMessage(maxValue, SOUTH);
    }
    break;
  }
  default: // Handle other cases if necessary
    break;
  }
}


void sendAckOppositeMessage(uint8_t maxValue, uint8_t senderPort) {
    AcknowledgmentMessage msg = {ACK_MSG, maxValue};
    uint8_t port;
    if (senderPort == NORTH || senderPort == SOUTH){
        port = (senderPort == NORTH) ? SOUTH : NORTH;
        sendMessage(port, (uint8_t*)&msg, sizeof(msg), 1);
    }
    else {
        port = (senderPort == WEST) ? EAST : WEST;
        sendMessage(port, (uint8_t*)&msg, sizeof(msg), 1);
    }
}

void sendAckMessage(uint8_t maxValue, uint8_t destPort) {
    AcknowledgmentMessage msg = {ACK_MSG, maxValue};
    sendMessage(destPort, (uint8_t*)&msg, sizeof(msg), 1);
}

void sendMoveMessage(uint8_t steps, uint8_t checkingType, uint8_t destPort ) {
MoveMessage msg = {MOVE_MSG, steps, checkingType};
    sendMessage(destPort, (uint8_t*)&msg, sizeof(msg), 1);
}

void processMoveMessage(uint8_t steps, uint8_t checkingType, uint8_t senderPort) {

  uint8_t port;
  int16_t currentLocationValue;

  if (checkingType == SHAPE_H) {
    if (senderPort == NORTH || senderPort == SOUTH) {

      if (x < 0) {
        currentLocationValue = x * -1;
      } else {
        currentLocationValue = x;
      }

      if (currentLocationValue == steps) {
        isH = 1;
        isSecondHNode = 1;
        startSettingCoordinatesForSecondHNode();
      } else {
        port = (senderPort == NORTH) ? SOUTH : NORTH;
        sendMoveMessage(steps, checkingType, port);
      }
    }
  } else if (checkingType == SHAPE_SQUARE) {
    if (senderPort == NORTH || senderPort == SOUTH) {

      if (x < 0) {
        currentLocationValue = x * -1;
      } else {
        currentLocationValue = x;
      }

      if (currentLocationValue == steps) {
        currentColor = RED;
        setColor(currentColor);
        southResponse = currentLocationValue;
        isSquare = 1;
        isThirdSquareNode = 1;
        startSettingCoordinatesForSecondSquareNode(2);
      } else {
        port = (senderPort == NORTH) ? SOUTH : NORTH;
        sendMoveMessage(steps, checkingType, port);
      }
    } else if (senderPort == EAST) {
      if (y < 0) currentLocationValue = y * -1;
      else currentLocationValue = y;
      if (currentLocationValue == steps) {
        eastResponse = currentLocationValue;
        isSquare = 1;
        isSecondSquareNode = 1;
        startSettingCoordinatesForSecondSquareNode(1);
      }
      sendMoveMessage(steps, checkingType, WEST);
    } else if (senderPort == WEST) {
      y = steps;
      steps--;
      if (steps == 0) {
        currentColor = RED;
        setColor(currentColor);
        uint8_t maxValue;
        if (y < 0) maxValue = y * -1;
        else maxValue = y;
        sendAckMessage(maxValue, WEST);
      } else {
        nbrWaitedAnswers++;
        sendMoveMessage(steps, checkingType, EAST);
      }
    }

  }

}


// Process the received `ACK_MSG`
void processAckMessage(uint8_t maxValue, uint8_t senderPort) {
    if (nbrWaitedAnswers > 0) {
        nbrWaitedAnswers--; // Decrement acknowledgment counter
        }

    if (nbrWaitedAnswers == 0 && !isPlus && !isH && !isSquare) {
        sendAckOppositeMessage(maxValue, senderPort);
    }
    else if (isPlus){
    if (senderPort==WEST) {westResponse=maxValue;}
    if (senderPort==EAST) {eastResponse=maxValue;}
    if (senderPort==SOUTH) {southResponse=maxValue;}
    if (senderPort==NORTH) {northResponse=maxValue;}

        if (nbrWaitedAnswers == 0){
            validatePlusShape();
        }
    }
    else if (isH){
    if (senderPort==WEST) {westResponse=maxValue;}
    if (senderPort==EAST) {eastResponse=maxValue;}
    if (senderPort==SOUTH) {southResponse=maxValue;}

        if (nbrWaitedAnswers == 0){
        uint8_t edge = (isSecondHNode==1) ? 2 : 1;
        validateHShape(edge);
        }
    }
    else if (isSquare){
    if (senderPort==WEST) {westResponse=maxValue;}
    if (senderPort==EAST) {eastResponse=maxValue;}
    if (senderPort==SOUTH) {southResponse=maxValue;}
    if (senderPort==NORTH) {northResponse=maxValue;}


        if (nbrWaitedAnswers == 0){
        uint8_t edge =1;
        if(isSecondSquareNode)edge=2;
        else if (isThirdSquareNode)edge=3;
            validateSquareShape(edge);
        }
    }
}

// Validate if the block forms a PLUS shape
void validatePlusShape() {
if (westResponse == eastResponse && eastResponse == southResponse && southResponse == northResponse){
broadcastShapeMessage(SHAPE_PLUS); // Broadcast PLUS shape
}
}
void validateHShape(uint8_t edge) {
    // Check if the neighbor counts are equal in all directions
if (edge ==1){
if (westResponse == eastResponse && westResponse*2 == southResponse) {
    //so now we should check the other edge node of the H
sendMoveMessage(southResponse, SHAPE_H, SOUTH);
}
}
else if (edge ==2){
if (westResponse == eastResponse){
       broadcastShapeMessage(SHAPE_H); // Broadcast H shape
}
}
}

void validateSquareShape(uint8_t edge) {
if (edge ==1){
if (westResponse == northResponse){
sendMoveMessage(westResponse, SHAPE_SQUARE, WEST);
}
}
else if (edge ==2){
if (eastResponse==northResponse){
sendMoveMessage(northResponse, SHAPE_SQUARE, NORTH);
}
}
else if (edge ==3){
currentColor=BLUE;
setColor(currentColor);
if (eastResponse==southResponse-1){
broadcastShapeMessage(SHAPE_SQUARE); // Broadcast PLUS shape
}
}
}

// Validate if the block forms a PLUS shape
void broadcastShapeMessage(uint8_t shape) {
    // Check if the neighbor counts are equal in all directions
    if(shape == SHAPE_PLUS && !isShapeDetected){
        isShapeDetected = 1;
        currentColor = BLUE;
        ShapeMessage message = {SHAPE_MSG, SHAPE_PLUS};
        for (uint8_t p = 0; p < NB_SERIAL_PORT; ++p) {
            if (is_connected(p)) {
                sendMessage(p, (uint8_t*)&message, sizeof(message), 1);
            }
        }
    }
    else if(shape == SHAPE_H && !isShapeDetected){
        isShapeDetected = 1;
        currentColor = PURPLE;
        ShapeMessage message = {SHAPE_MSG, SHAPE_H};
        for (uint8_t p = 0; p < NB_SERIAL_PORT; ++p) {
            if (is_connected(p)) {
                sendMessage(p, (uint8_t*)&message, sizeof(message), 1);
            }
        }
    }
    else if(shape == SHAPE_SQUARE && !isShapeDetected){
        isShapeDetected = 1;
        currentColor = RED;
        ShapeMessage message = {SHAPE_MSG, SHAPE_SQUARE};
        for (uint8_t p = 0; p < NB_SERIAL_PORT; ++p) {
            if (is_connected(p)) {
                sendMessage(p, (uint8_t*)&message, sizeof(message), 1);
            }
        }
    }
}


// Process received standard packets
uint8_t process_standard_packet(L3_packet *packet) {
    uint8_t msgType = packet->packet_content[0];
    uint8_t senderPort = packet->io_port;

    switch (msgType) {
        case SETCOOR_MSG: {
            SetCoorMessage *setCoorMsg = (SetCoorMessage*)packet->packet_content;

            if (!hasSetCoordinates) {
                // Update local coordinates based on received data and port
                updateCoordinatesBasedOnPort(setCoorMsg->x, setCoorMsg->y, packet->io_port);

                hasSetCoordinates = 1;

                SetCoorMessage message = {SETCOOR_MSG, x, y};

                // Propagate the SetCoorMessage to neighbors except the sender
                propagateSetCoor(&message, packet->io_port);

                // Notify the sender with ACK
                //uint8_t ackData[1] = {ACK_MSG};
                //sendMessage(packet->io_port, ackData, 1, 1);
            }
            else {
                uint8_t maxValue;
            if (y<0)maxValue= y*-1;
                else maxValue= y;
                sendAckMessage(maxValue, WEST);
            }
            return 1;
        }
        case ACK_MSG: {
            AcknowledgmentMessage *ackMsg = (AcknowledgmentMessage *)packet->packet_content;
            processAckMessage(ackMsg->maxValue, senderPort);
            return 1;
        }
        case SHAPE_MSG: {
            ShapeMessage *shapeMessage = (ShapeMessage *)packet->packet_content;
            broadcastShapeMessage(shapeMessage->shapeType);
            return 1;
        }
        case MOVE_MSG: {
            MoveMessage *moveMessage = (MoveMessage *)packet->packet_content;
            processMoveMessage(moveMessage->steps, moveMessage->checkingType, senderPort);
            return 1;
        }
        default:
            return 0;
    }
}
