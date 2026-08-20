# ATMOS: Post Disaster Patrolling Device

Atmos is a remote controlled rover-based remote controlled device meant to drive to a post disaster terrain to reach humans normally in difficult-to-reach areas. This was made for a competition and due to competition rules, our model is 21x23x14 cm in dimensions (not the most suitable for this purpose). If made a little bigger it has a lot more potential than our current model with no modifications

<img src="./Atmos.jpg">

## Features

- Highly modular (both chassis and electronics)
- HIGH Torque Front Lifters to clear obstacles
- Wireless Range (without external antennas) ~ 250 meter
- Toolless 3D printed parts assembly

## Current Limitations

- No storage unit for medical supplies
- ESP32 Cam did not work due to hardware restrictions
- Heavy (~ 3Kg in mass)
- Cannot drive through rough terrain
- Dimensionally restricted

## Possible Potentials

- Wheels modified to adapt rough terrain
- Storage Unit for medical supplies
- Camera for visual feed

## Bill of Materials (BoM)

| PART NAME | PART NUMBER | QUANTITY USED |
| --------- | ----------- | ------------- |
| Microcontroller | ESP32 | 2 |
| DC Motors | 12v 200RPM | 4 |
| DC Motors | 12v 30RPM | 2 |
| 3s LiPo Battery | Required Capacity | 1 |
| 18650 3.7v Cell | Required Capacity | 1 |
| Motor Driver | L293D IC | 2 |
| DC-DC Buck Converter | LM2596 | 1 |
| I2C ADC | ADS1115 | 1 |
| OLED Display | SH1106 | 1 |
| Joystick Module | --- | 2 |
| Switch | --- | 2 |
| Battery Connectors | XT60, 18650 | 1 |
| Gas Sensor | MQ135 | 1 |
| PCB | --- | 2 |
| Wires | --- | Required Amount |
| Connectors | Required Connectors | --- |

## Working in Brief

The remote and Atmos share information wirelessly via ESP NOW protocol. The remote reads the Joystick input and sends it to Atmos and Atmos receives the following input and executes the action. Similarly, atmos also read the vales of the Gas sensor (MQ 135) and sends it to the remote via ESP NOW and the remote does the required processing.

## Circuit Diagram

<img src="Atmos/Atmos.png">

> Atmos circuit diagram

<img src="Atmos/diagram.png">

> Atmos schematic diagram

<hr>

<img src="Remote/Remote.png">

> Remote circuit Diagram

<img src="Remote/diagram.png">

> Remote schematic Diagram
