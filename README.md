# Shape-Recognition-Distributed-Algorithm

authors:
  * [Mohammad SHREM](https://www.linkedin.com/in/mohammadbshreem/) <mohammad.shrem@edu.univ-fcomte.fr>
  * [Mariam AL KHALAF]() <mariam.al_khalaf@edu.univ-fcomte.fr>
  * [Idrissa MASSALY]() <idrissa.massaly@edu.univ-fcomte.fr>

supervisor:
  * [Prof. Benoit PIRANDA](https://www.femto-st.fr/fr/personnel-femto/bpiranda) <benoit.piranda@univ-fcomte.fr>
  * [Prof. Dominique DHOUTAUT](https://www.femto-st.fr/fr/personnel-femto/ddhoutau) <dominique.dhoutaut@univ-fcomte.fr>

project relaized in [VisibleSim](https://github.com/VisibleSim/VisibleSim)

---

# Shape Recognition Distributed Algorithm

## Overview

This project implements a distributed algorithm for recognizing specific geometric shapes (**PLUS**, **H**, and **SQUARE**) using interconnected nodes in a hardware or virtual system. The algorithm employs message passing, coordinate propagation, and validation stages to identify these shapes, making it suitable for applications such as robotics, sensor networks, and distributed systems.

---

## Features

### Recognized Shapes

1. **PLUS Shape**: A center node connected symmetrically to nodes in all four cardinal directions.
2. **H Shape**: Two horizontal bars connected by a vertical bar in the middle.
3. **SQUARE Shape**: A closed square formed by four nodes at the corners, with connections along the edges.

### Message Types

The algorithm uses specific message types for inter-node communication:
- **SETCOOR_MSG**: Used to propagate coordinates.
- **ACK_MSG**: Acknowledgment message containing the maximum distance value.
- **SHAPE_MSG**: Broadcasted to announce a detected shape.
- **MOVE_MSG**: Used to propagate validation requests for shape checks.

### Communication Protocol

Each node exchanges messages with its neighbors using predefined structures:
- **SetCoorMessage**: For coordinate setting.
- **AcknowledgmentMessage**: For acknowledgment responses.
- **ShapeMessage**: For broadcasting detected shapes.
- **MoveMessage**: For specific movement and shape validation tasks.

---

## Code Details

### Initialization (`BBinit`)

The initialization function sets up the environment, detects potential shape hints, and starts the coordinate propagation process if shape criteria are met. 

Example:
```c
if (is_connected(WEST) && is_connected(NORTH) && is_connected(EAST) && is_connected(SOUTH)) {
    isPlus = 1;
    startSettingCoordinates();
}
```

### Main Loop (`BBloop`)

The main loop periodically updates the node state and ensures consistent execution.

---

## Shape Recognition Process

### PLUS Shape

#### Criteria:
- Connected to **NORTH**, **SOUTH**, **EAST**, and **WEST**.
- Distances to neighbors in all four directions are equal.

#### Detection Steps:
1. **Initialization**:
   - Check if all four directions are connected.
   - Set the `isPlus` flag and start coordinate propagation.

2. **Coordinate Propagation**:
   - Broadcast `SETCOOR_MSG` to all neighbors.
   - Broadcasting `SETCOOR_MSG` will stop when no more neighbor is connected on the path (e.g., move to `NORTH` if the received packet is from `SOUTH`).
   - Maximum distance value is determined based on the direction (`x` or `y`).

3. **Acknowledgment Collection**:
   - Neighbors reply with `ACK_MSG` containing their maximum distance values.
   - When no more neighbors are waiting to reply, all maximum distance values returned should be equal. If they are, broadcast the **PLUS SHAPE**.

4. **Validation**:
   - Compare distances in all directions:
     ```c
     if (westResponse == eastResponse && eastResponse == southResponse && southResponse == northResponse) {
         broadcastShapeMessage(SHAPE_PLUS);
     }
     ```
     

<div align="center">
              <img src="https://github.com/user-attachments/assets/f7fc10bf-dafa-4681-9b32-904c41927285"></br>
              <i>Figure 1: diagram for Plus Shape</i>
              </div>


### H Shape

#### Criteria:
- Two horizontal bars connected by a vertical bar in the middle.

#### Detection Steps:
1. **Initialization**:
   - Detect connections forming the horizontal and vertical components of the **H**.
   - Set the `isH` flag and start coordinate propagation.

2. **Coordinate Propagation**:
   - Broadcast `SETCOOR_MSG` to neighbors.
   - Broadcasting `SETCOOR_MSG` will stop when no more neighbor is connected on the path (e.g., move to `NORTH` if the received packet is from `SOUTH`).
   - Maximum distance value is determined based on the direction (`x` or `y`).

3. **Validation**:
   - Performed in two stages:
     - **Stage 1**: Validate the first horizontal bar:
       ```c
       if (westResponse == eastResponse && westResponse * 2 == southResponse) {
           sendMoveMessage(southResponse, SHAPE_H, SOUTH);
       }
       ```
       Here, the `move` function is used to move to the `SOUTH` with `steps = southResponse`. Once there, the algorithm proceeds to the second horizontal bar.
     - Set the `isH` flag and start coordinate propagation for this bar to neighbors on the `WEST` and `EAST`.
     - Acknowledgment (`ACK`) will return the maximum distance when no more neighbors are connected to the center of the second horizontal bar.

     - **Stage 2**: Confirm the second horizontal bar:
       ```c
       if (westResponse == eastResponse) {
           broadcastShapeMessage(SHAPE_H);
       }
       ```
       If distances are equal, broadcast the **H SHAPE** message.
              <div align="center">
              <img src="https://github.com/user-attachments/assets/6983cc04-ee45-462e-80c6-a3db4f6aa65d"></br>
              <i>Figure 2: diagram for H Shape</i>
              </div>

---

### SQUARE Shape

#### Criteria:
- Equal distances to neighbors along all four edges.
- Symmetry in connectivity to confirm closure of the square.

#### Detection Steps:
1. **Initialization**:
   - Check for connections that could indicate a square (e.g., `NORTH` and `WEST` connected but not `EAST` or `SOUTH`).
   - Set the `isSquare` flag and start coordinate propagation.

2. **Coordinate Propagation**:
   - Broadcast `SETCOOR_MSG` to neighbors, propagating coordinates along potential square edges.
   - Broadcasting `SETCOOR_MSG` will stop when no more neighbor is connected on the path (e.g., move to `NORTH` if the received packet is from `SOUTH`).
   - Maximum distance value is determined based on the direction (`x` or `y`).

3. **Validation**:
   - Validation is carried out in **three stages**:
     - **Stage 1**: Verify symmetry between `WEST` and `NORTH` edges:
       ```c
       if (westResponse == northResponse) {
           sendMoveMessage(westResponse, SHAPE_SQUARE, WEST);
       }
       ```
       Here, the `move` function is used to move to the `WEST` with `steps = westResponse`. Once there, the algorithm proceeds to the second node of the square, storing the `eastResponse = steps`.
     - Set the `isSquare` flag and start coordinate propagation for this bar to neighbors on the `NORTH`.
     - Acknowledgment (`ACK`) will return the maximum distance when no more neighbors are connected to the second node of the square.

     - **Stage 2**: Confirm alignment between `EAST` and `NORTH` edges:
       ```c
       if (eastResponse == northResponse) {
           sendMoveMessage(northResponse, SHAPE_SQUARE, NORTH);
       }
       ```
       Here, the `move` function is used to move to the `NORTH` with `steps = northResponse`. Once there, the algorithm proceeds to the third node of the square, storing the `southResponse = steps` for the third node.

     - Set the `isSquare` flag and start coordinate propagation for this bar to neighbors on the `EAST`.
     - Acknowledgment (`ACK`) will return the maximum distance when no more neighbors are connected to the third node of the square.

     - **Stage 3**: Ensure closure by checking `EAST` and `SOUTH`:
       ```c
       if (eastResponse == southResponse - 1) {
           broadcastShapeMessage(SHAPE_SQUARE);
       }
       ```
   - At each stage, distances are compared to ensure the edges meet the square's geometric properties.

4. **Shape Confirmation**:
   - Once all edges and symmetry checks are validated, the **SQUARE SHAPE** is broadcasted.
   
     <div align="center">
       <img src="https://github.com/user-attachments/assets/3dba5f93-e5a4-499e-9962-0861287cf5bc"></br>
       <i>Figure 3: diagram for square Shape</i>
     </div>
---

## Key Functions

### `startSettingCoordinates`
Broadcasts the initial coordinates to all connected neighbors.

### `updateCoordinatesBasedOnPort`
Updates the node’s coordinates based on the sender’s coordinates and the port direction.

### `propagateSetCoor`
Propagates `SETCOOR_MSG` to neighbors except the sender.

### `CheckState`
Checks the state of neighbors after setting coordinates. Behavior depends on the shape:
- **PLUS**: 
  - For `SOUTH` port: Ensure no neighbors on `EAST` and `WEST`, then forward to `NORTH`.
  - For `NORTH` port: Ensure no neighbors on `EAST` and `WEST`, then forward to `SOUTH`.
  - For `EAST` port: Ensure no neighbors on `NORTH` and `SOUTH`, then forward to `WEST`.
  - For `WEST` port: Ensure no neighbors on `NORTH` and `SOUTH`, then forward to `EAST`.

- **H**:
  - Same as PLUS for `EAST`, `WEST`, and `SOUTH` ports.
  - For `SOUTH`, ensure neighbors are connected on both `EAST` and `WEST`. If true, start sending back `ACK` to confirm the second node of the **H**.

- **SQUARE**:
  - Similar checks as PLUS and H, ensuring symmetry and closure at each node.

### `validatePlusShape`
Validates the geometry of a **PLUS** shape by comparing neighbor distances.

### `validateHShape`
Checks the symmetry of an **H** shape in two stages.

### `validateSquareShape`
Validates the geometry of a **SQUARE** shape in three stages.

### `broadcastShapeMessage`
Broadcasts the detected shape to all neighbors.

---

## Result

<div align="center">
  <img src="https://github.com/user-attachments/assets/f50461e8-23fb-4d6f-ae24-c637395c8c48"></br>
  <i>Figure 4: Testing result - detecing the three edges of square</i>
</div>

<div align="center">
  <img src="https://github.com/user-attachments/assets/92c3ba56-f4b5-44cd-baaa-bb183113a3ce"></br>
  <i>Figure 5: Testing result - Plus Detection</i>
</div>

<div align="center">
  <img src="https://github.com/user-attachments/assets/e623679b-1ab9-4496-a9e5-e2f88a9ac8c5"></br>
  <i>Figure 6: Testing result - coloring the block based on the recicved port for testing</i>
</div>

<div align="center">
  <img src="https://github.com/user-attachments/assets/0bbb1329-85eb-4eb0-87aa-d85ed28920ae"></br>
  <i>Figure 7: Testing result - H Detection</i>
</div>

<div align="center">
  <img src="https://github.com/user-attachments/assets/270d7ab2-3869-4438-84e9-381f4de1f8be"></br>
  <i>Figure 8: Testing result - Square Detection</i>
</div>

---

https://github.com/user-attachments/assets/3ee511be-83f5-4921-8bec-adc089ba6fec
<div align="center">
  <i>Figure 9: Testing result - Full Video</i>
</div>

---






