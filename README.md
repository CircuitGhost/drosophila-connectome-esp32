# Drosophila Connectome Central Complex (CX) Flight Simulator

A neuromorphic implementation of the fruit fly (*Drosophila melanogaster*) Central Complex (CX) navigation and reinforcement learning connectome, mapped directly from research published by **Google Research**, **Janelia Research Campus**, and **FlyWire**.

Engineered to run in real-time on the **ESP32-C6 160MHz RISC-V microcontroller** with a **Waveshare 1.47" ST7789 Display (172×320)**.

---

## 🧠 Neuroscience & Connectome Circuit Architecture

Google's connectomics team mapped out the exact synaptic topology and functional cell types responsible for visual landmark calibration, angular velocity tracking, and goal-directed vector steering:

```
[Visual Patterns on Corridor Walls]
               │
               ▼
      Ring Neurons (ER)  ◄─── Plastic Synapses (Visual Landmark Learning)
               │
               ▼
     E-PG Compass Neurons (Ellipsoid Body Ring Attractor)
          ▲         │
          │         ▼
      P-EN / P-EG (Protocerebral Bridge: Angular Velocity Shifter)
                    │
                    ▼
     Fan-shaped Body (FB Columnar & Tangent Neurons) ◄─── Dopamine (DANs)
                    │
                    ▼
     Motor Steering (Veer Left / Veer Right)
```

### 1. Compass Ring Attractor ($E-PG$ Wedges)
* **Ellipsoid Body** represented as an array of 16 discrete anatomical wedges ($N = 16$).
* Recurrent weights implement local cosine excitation and global ring surround inhibition.
* **Protocerebral Bridge ($P-EN / P-EG$)**: Real-time angular velocity ($\Delta\theta$) shifts the activity bump left and right as the fly veers.

$$V_i(t+1) = \text{ReLU}\left( V_i(t) + \sum_{j} W_{ij}^{recurrent} V_j(t) + \beta \cdot \text{Shift}(\Delta\theta) + I_i^{visual} \right)$$

### 2. Unsupervised Visual Plasticity ($ER \to E-PG$)
* Visual inputs enter through 16 azimuthal Ring Neurons ($ER$), forming inhibitory plastic synapses onto the $E-PG$ compass neurons.
* An **anti-Hebbian plasticity rule** allows the fly to learn and calibrate against wall optical flow patterns from scratch:

$$W_{k,i}(t+1) = W_{k,i}(t) - \eta \cdot ER_k \cdot EPG_i$$

### 3. Goal-Directed Vector Steering & Reinforcement Learning (Fan-shaped Body)
* The **Fan-shaped Body ($FB$)** stores the desired travel vector.
* Columnar bridge neurons ($P-FN$ and $h\Delta$) perform vector subtraction between the decoded compass heading and the target vector:

$$\text{Steer} = \sum_{i=1}^{8} PFN_i^{Left} - \sum_{i=9}^{16} PFN_i^{Right}$$

* **Dopaminergic Neurons ($DANs$)**: Wall collisions trigger the $PPL1$ penalty cluster, deflecting the goal vector away from obstacles, while centered corridor cruising triggers the $PAM$ reward cluster.

### 4. Giant Fiber Neuron ($GFN$) Predator Threat Reflex
* A looming visual shadow activates the hardwired insect escape reflex.
* Evasion agility is directly tied to the fly's **Learned Intelligence Score ($IQ$)**.
* If a naive fly fails to clear the shadow before the strike, the fly is **EATEN** and its neural intelligence resets to $0\%$.

---

## 🖥️ Display Layout (172×320 Waveshare LCD)

* **Top Viewport (0, 0 to 172, 160)**:
  * 2D Optic Flow Corridor Arena with scrolling textured walls.
  * Flapping fly avatar with physical collision detection.
  * Looming predator shadow on manual summons.
* **Bottom Viewport (0, 160 to 172, 320)**:
  * **Outer Ring**: 16 Visual $ER$ Units (brightness represents visual feature activation).
  * **Inner Ring**: 16 $E-PG$ Compass Bump Nodes (spinning biological gyroscope).
  * **Green Arrow**: Fan-shaped Body ($FB$) Goal Vector.
  * **Orange Pointer**: Current decoded compass heading.
  * **Live Metrics**: `IQ: XX%`, Synaptic Delta $\Delta W$, Steering Bias, and Active Learning Phase (`P1: NAIVE` $\to$ `P2: LOCKED` $\to$ `P3: CENTERING` $\to$ `P4: GFN ESCAPE`).

---

## 🕹️ Hardware Controls & Pinout

### Waveshare ESP32-C6-Touch-LCD-1.47
* **Screen Driver**: ST7789 / JD9853 (80 MHz Hardware SPI)
* **SCLK**: `GPIO 7`
* **MOSI**: `GPIO 6`
* **MISO**: `GPIO 5`
* **LCD CS**: `GPIO 14`
* **LCD DC**: `GPIO 15`
* **LCD RST**: `GPIO 21`
* **Backlight**: `GPIO 22` (Active HIGH)
* **Interactive Button (`BOOT`)**: `GPIO 9`
  * **Quick Tap**: Delivers an electric shock penalty ($PPL1$ Dopamine spike + Red LED flash).
  * **Hold (>350ms)**: Summons a looming predator attack to test the fly's learned evasion reflex.
* **Reset Button (`RST`)**: Physical hardware reboot.
* **Onboard WS2812 RGB LED (`GPIO 8`)**:
  * 🔴 **Red Flash**: Collision penalty, electric shock, or predator strike.
  * 🟢 **Green Pulse**: Centered flight reward (PAM Dopamine cluster).
  * 🔵 **Dim Blue**: Steady cruising.

---

## 🚀 Building & Flashing

This project uses [PlatformIO](https://platformio.org/):

```bash
# Build firmware
pio run

# Flash to device
pio run -t upload
```
