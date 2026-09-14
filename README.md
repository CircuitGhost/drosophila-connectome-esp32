# Drosophila Connectome Central Complex (CX) Flight Simulator

A neuromorphic flight simulator based on research into the *Drosophila melanogaster* Central Complex (CX), including connectome reconstructions from Google Research, Janelia Research Campus, and FlyWire.

This project implements the core neural circuits involved in heading estimation, visual landmark learning, vector arithmetic, and reinforcement learning in real-time on an ESP32-C6 microcontroller.

### Hardware

* **MCU**: ESP32-C6 (160 MHz 32-bit RISC-V)
* **Display**: Waveshare 1.47" ST7789 LCD (172×320, 80 MHz SPI)

---

## Neural Circuit Architecture

The model is organized around the primary functional pathways of the fruit fly Central Complex:

```
 [Visual Patterns on Corridor Walls]
                  │
                  ▼
         Ring Neurons (ER)
                  │
                  │  Plastic Visual Synapses
                  ▼
        E-PG Compass Neurons
   (Ellipsoid Body Ring Attractor)
            ▲             │
            │             ▼
        P-EN / P-EG Neurons
   (Angular Velocity / Yaw Shift)
                          │
                          ▼
            Fan-shaped Body (FB) ◄─── Dopamine (DANs)
          (Columnar Vector Math)
                          │
                          ▼
                Motor Steering Output
               (Veer Left / Veer Right)
```

### 1. Compass Ring Attractor ($E\text{-}PG$ Wedges)

The Ellipsoid Body is modeled as 16 discrete wedges. Neural activity forms a localized "bump" around this ring, serving as the fly's internal heading estimate. Recurrent cosine excitation maintains the bump, while global inhibition prevents broad activation.

Angular velocity ($\Delta\theta$) from the Protocerebral Bridge ($P\text{-}EN / P\text{-}EG$) shifts the bump left or right as the fly turns:

$$V_i(t+1) = \text{ReLU}\left( V_i(t) + \sum_j W_{ij}^{\text{recurrent}} V_j(t) + \beta \cdot \text{Shift}(\Delta\theta) + I_i^{\text{visual}} \right)$$

### 2. Visual Landmark Learning ($ER \to E\text{-}PG$)

Visual input is partitioned into 16 azimuthal sectors represented by Ring Neurons ($ER$). These form plastic inhibitory synapses onto the compass neurons ($E\text{-}PG$).

Using an anti-Hebbian plasticity rule, the fly continuously binds corridor landmark patterns to its heading:

$$W_{k,i}(t+1) = W_{k,i}(t) - \eta \cdot ER_k \cdot EPG_i$$

### 3. Goal-Directed Steering & Reinforcement Learning

The Fan-shaped Body ($FB$) stores a goal vector. Columnar neurons ($P\text{-}FN$ and $h\Delta$) perform vector subtraction between current heading and goal direction, producing asymmetric left/right motor bias:

$$\text{Steer} = \sum_{i=1}^{8} PFN_i^{\text{Left}} - \sum_{i=9}^{16} PFN_i^{\text{Right}}$$

Dopaminergic neurons ($DANs$) modulate steering preferences based on experience:
* **Wall collisions**: Trigger a $PPL1$-like penalty signal, deflecting the travel vector away from obstacles.
* **Centered flight**: Triggers a $PAM$-like reward signal, reinforcing stable forward navigation.

### 4. Giant Fiber Threat Evasion Reflex

A looming visual shadow activates the Giant Fiber escape circuit. The fly's escape agility is directly scaled by its learned intelligence score ($IQ$):

* **Naive Fly ($IQ < 30\%$)**: Sluggish reaction time and weak evasion; fails to clear the shadow in time and gets **EATEN** (resetting intelligence to $0\%$).
* **Trained Fly ($IQ > 60\%$)**: Executes an instant, high-speed evasive bank into open corridor space.

---

## Display Layout (172×320)

| Viewport | Description |
| :--- | :--- |
| **Top (172×160)** | **2D Optic Flow Arena**: Top-down view of the scrolling corridor, animated fly avatar, physical wall boundaries, and looming predator shadow. |
| **Bottom (172×160)** | **Central Complex Monitor**: Outer ring of 16 $ER$ visual nodes, inner ring of 16 $E\text{-}PG$ compass nodes, green $FB$ goal vector, orange compass pointer, and live `IQ: XX%` telemetry. |

---

## Controls & Indicators

* **`BOOT` Button (GPIO 9)**:
  * *Tap*: Administer electric shock penalty ($PPL1$ dopamine burst + red flash).
  * *Hold (>350ms)*: Manually summon a looming predator attack to test evasion capability.
* **`RST` Button**: Hardware chip reset (reboots MCU and wipes neural weights to clean slate).
* **Onboard RGB LED (GPIO 8)**:
  * 🔴 **Red Flash**: Wall collision, shock penalty, or predator attack.
  * 🟢 **Green Pulse**: Centered cruising reward ($PAM$ cluster).
  * 🔵 **Dim Blue**: Steady cruising.

---

## Building & Flashing

```bash
# Build firmware
pio run

# Flash to device
pio run -t upload
```

---

## License

[MIT](LICENSE)
