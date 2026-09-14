# Drosophila Connectome Central Complex (CX) Flight Simulator

A small neuromorphic flight simulator based on research into the *Drosophila melanogaster* Central Complex (CX), including connectome work from Google Research, Janelia Research Campus, and FlyWire.

The goal is to take some of the neural circuits involved in navigation, heading estimation, visual landmark learning, and reinforcement and turn them into something that can run in real time on an ESP32-C6.

Current hardware:

* ESP32-C6, 160 MHz RISC-V
* Waveshare 1.47" ST7789 display
* 172×320 resolution

---

## Neuroscience / circuit model

The model is loosely organized around the same major pathways seen in the fly Central Complex:

```text
[Visual Patterns on Corridor Walls]
                │
                ▼
       Ring Neurons (ER)
                │
                │  plastic visual connections
                ▼
      E-PG Compass Neurons
   (Ellipsoid Body ring attractor)
          ▲             │
          │             ▼
      P-EN / P-EG Neurons
  (angular velocity / bump shift)
                        │
                        ▼
          Fan-shaped Body
       (FB columnar neurons)
                ▲
                │
             Dopamine
                │
                ▼
       Motor Steering Output
      (veer left / veer right)
```

This isn't intended to simulate every neuron in a fly brain. The idea is to reproduce a few of the interesting computational structures in the CX with a model small enough to run interactively on a microcontroller.

### 1. Compass ring attractor — E-PG wedges

The Ellipsoid Body is represented as 16 discrete wedges.

E-PG activity forms a moving "bump" around this ring, which acts as the fly's internal estimate of heading. Local excitation keeps the bump together while broader inhibition prevents the entire ring from becoming active at once.

P-EN / P-EG activity shifts that bump left or right as the simulated fly turns.

$$
V_i(t+1) =
\text{ReLU}\left(
V_i(t)
+ \sum_j W_{ij}^{recurrent}V_j(t)
+ \beta \cdot \text{Shift}(\Delta\theta)
+ I_i^{visual}
\right)
$$

In practice, this gives the fly an internal compass that continues to update even while visual input is changing.

### 2. Visual landmark learning — ER → E-PG

Visual input is divided into 16 azimuthal sectors represented by Ring Neurons (ER).

These feed into the E-PG compass through plastic inhibitory connections. Rather than giving the fly a pre-programmed map of the corridor, the weights can adapt as it experiences repeated visual patterns.

The current implementation uses a simple anti-Hebbian update:

$$
W_{k,i}(t+1)
=
W_{k,i}(t)
-
\eta \cdot ER_k \cdot EPG_i
$$

The result is a crude form of visual calibration: repeated landmarks start influencing the compass state based on what the fly has previously experienced.

### 3. Goal-directed steering and reinforcement

The Fan-shaped Body provides the goal/navigation side of the model.

A desired travel direction is compared against the current compass heading, with P-FN and hΔ-inspired pathways producing a steering bias.

A simplified version of the steering output is:

$$
\text{Steer}
=
\sum_{i=1}^{8} PFN_i^{Left}
-
\sum_{i=9}^{16} PFN_i^{Right}
$$

That value becomes the motor command that pushes the fly left or right.

There is also a small reinforcement layer using dopamine-inspired reward and penalty signals.

* Wall collisions produce a PPL1-like penalty signal.
* Stable, centered flight produces a PAM-like reward signal.
* Those signals gradually alter the fly's preferred steering behavior.

So the fly isn't following a fixed path. Its behavior changes as it flies, hits things, and successfully moves through the corridor.

### 4. Giant Fiber predator reflex

The simulator also includes a deliberately simpler circuit: a looming-shadow escape response inspired by the Giant Fiber System.

When a large visual threat appears, the fly gets a short window to evade it.

Its chance of escaping is influenced by its current learned behavior / "IQ" score.

Fail to move far enough before the strike and the fly gets **EATEN**.

Its learned state then resets to 0%.

This part is less about faithfully reproducing the full Giant Fiber circuit and more about giving the learned navigation system a visible consequence.

---

## Display layout

The UI is designed around the 172×320 Waveshare LCD.

### Top viewport — 172×160

The upper half of the screen shows the actual simulation:

* 2D corridor viewed from above
* Scrolling wall textures used as visual landmarks
* Animated fly
* Physical wall collisions
* Predator / looming-shadow events

The visual environment isn't just decoration. The wall patterns are also the input used by the ER visual-learning portion of the model.

### Bottom viewport — 172×160

The lower half displays real-time telemetry from the Central Complex:

* **Outer Ring**: 16 ER visual units indicating active optical flow inputs around the fly.
* **Inner Ring**: 16 E-PG wedges displaying the rotating activity bump (biological compass).
* **Green Vector**: Desired travel heading stored in the Fan-shaped Body.
* **Orange Pointer**: Current decoded compass heading.
* **Metrics**: Real-time `IQ: XX%` score, synaptic plasticity delta (`DW`), and steering bias.

---

## Hardware Controls

* **`BOOT` Button (GPIO 9)**:
  * **Short Tap**: Negative reinforcement / electric shock penalty ($PPL1$ Dopamine burst).
  * **Long Hold (>350ms)**: Manually summons a looming predator attack to test the fly's learned evasion reflex.
* **`RST` Button**: Hardware chip reset (reboots ESP32 and resets intelligence to 0%).
* **Onboard RGB LED (GPIO 8)**:
  * 🔴 **Red Flash**: Wall collision, shock penalty, or predator threat.
  * 🟢 **Green Pulse**: Centered cruising reward (PAM Dopamine cluster).
  * 🔵 **Dim Blue**: Steady cruising.

---

## Building & Flashing

Built with [PlatformIO](https://platformio.org/):

```bash
# Build firmware
pio run

# Flash to board
pio run -t upload
```

---

## License

MIT
