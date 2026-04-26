# Unified Switch Firmware

This project combines the firmware for several of my single pair ethernet switches into one codebase. It supports the following hardware:
- [Switch v4](https://github.com/ben5049/switch-v4-hardware): 4 Ports (3x 100/1000BASE-T1 + 1x 10BASE-T1S)
- [Switch v5](https://github.com/ben5049/switch-v5-hardware): 7 Ports (5x 100/1000BASE-T1 + 1x 10BASE-T1S + 1x 10/100/1000BASE-T)

The goal was to move development from STM32CubeIDE to CMake (+ VS Code) and reduce the amount of duplicated code.

The firmware is written exclusively in C.

## Overview

This repository is split into firmware for the primary and secondary MCUs on each switch board:
- The **primary** MCU does all of the configuration and management of the switch.
- The **secondary** MCU manages power to connected devices using Power over Datalines (PoDL).

Currently only the primary MCU firmware has been written.

## Building

To build the firmware:
1. Clone this repo and open the `primary` folder in VS Code.
2. Install the [STM32CubeIDE for Visual Studio Code](https://marketplace.visualstudio.com/items?itemName=stmicroelectronics.stm32-vscode-extension) extension.
3. You should be prompted to configure the project (may need to reload window). Follow the configuration steps.
4. Select a preset in the command palette (`ctrl` + `shift` + `p`) > `CMake: Select Configure Preset` > select an option. These options are from [CMakePresets.json](CMakePresets.json).
5. To build the firmware go to the command palette and go to `Tasks: Run Task` > `build` (note you can select `upload` here to upload the firmware via an STLink).

## Debugging

To debug the firmware:
- Follow the [building](#building) guide above up to step 4.
- Go to the `Run and Debug` menu in the side bar
- Make sure you have an STLink plugged in
- Click the green play button at the top to start a debugging session. (this is configured in [.vscode/launch.json](.vscode/launch.json))

## Primary MCU Firmware

Most of the non-generated code can be found in the [application](primary/NonSecure/application) directory. This project uses the ARM Trustzone architecture to have a secure bootloader and a non-secure application.

The non-secure firmware is responsible for the following:
- Initialising non-secure peripherals
- Initialising the switch and PHY chips
- Running the networking stack
- Publishing diagnostic information with Zenoh Pico
- Switch and PHY maintenance
- Firmware updates (WIP)
- Running PTP (WIP)
- Running STP (WIP)

## Dependencies

- [ThreadX](https://github.com/eclipse-threadx/threadx)
- [NetX Duo](https://github.com/eclipse-threadx/netxduo)
- [SJA1105 Driver](https://github.com/ben5049/stm32-sja1105) (by me)
- [PHY Drivers](https://github.com/ben5049/phy-drivers) (by me)
- [Zenoh Pico](https://github.com/eclipse-zenoh/zenoh-pico)
- [Nanopb](https://github.com/nanopb/nanopb)
- ~~[mstp-lib](https://github.com/adigostin/mstp-lib)~~
- [Logging](https://github.com/ben5049/logging) (by me)
- [Bootloader](https://github.com/ben5049/bootloader) (by me)


## Functional Description

### PHY State Machines

Each PHY has a state machine that controls what it is currently doing starting from the UNINITIALISED state:

![phy-state-transitions](/docs/images/phy-state-transitions.drawio.png)

No-delay loops shouldn't be possible, but there is a limit to the number of transitions that can take place back-to-back to prevent deadlocks.

The purpose of the state machine is to allow the PHY to sleep when there is no link, as well as controlling the polling rate. Interrupts are also used to asynchronously update the state. The figure below shows the timings when 5 ports are unconnected:

![phy-state-timing](/docs/images/phy-state-timing.drawio.png)
