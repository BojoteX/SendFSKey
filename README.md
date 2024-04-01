# SendFSKey - Sends MSFS Keyboard commands over the network

## Overview
SendFSKey is designed for seamless remote keyboard input in Microsoft Flight Simulator (MSFS) from a local network. Its meant to be used by cockpit builders or developers to facilitate sending keyboard commands to the sim over a local network. It was specifically designed to work with Spad.NeXt Keyboard commands sending facility but it can work with pretty much any program that sends Keypresses to the SendFSKey window.

## Sample use cases
Certain functions in MSFS do NOT expose a SimConnect event or control binding, making it impossible to interact with them programmatically. SendFSKey can be used to send keyboard commands to the sim over the network, enabling you to control these functions remotely. For example, you can use SendFSKey to send a KeyPress to toggle the Flashlight (which does not have a Simconnect event to enable it) or for example VR activation, view changes, view resets. Another example is the use of the "KEY_UP" and "KEY_DOWN" events to simulate a button press and hold, which can be used to control the throttle, flaps, or other functions that require a continuous input. There are much more examples of this, the limit is your imagination.

## Features
- **Dual-Mode Functionality**: Operate in either remote or client mode for flexible usage scenarios with a single executable.
- **Efficient and Lightweight**: Crafted with C++ for maximum efficiency and minimal resource usage. No CPU usage and very fast persistent network connections
- **Correctly interprets** KEY_UP and KEY_DOWN events and allows clients to determine time between presses.
- **Fast and Resilient**: Engineered to be incredibly fast, ensuring real-time performance with built-in resilience to network instabilities.
- **User-Friendly**: Easy to set up and use, regardless of your technical background.
- **Open Source**: Freely available for personal and commercial use under the MIT No Attribution License.

> [!WARNING]
> Its possible you get a virus warning when downloading the .exe file, in that case I suggest you download the sources and compile it yourself.

## Prerequisites
- Microsoft Flight Simulator 2020 installed on the server computer.
- A network connection between the client and server machines.
- SendFSKey in client mode requires keypresses sent to its own window -or- a program that can send keypresses to it. (I suggest using Spad.NeXt for this)

## Installation
Clone the repository and compile the project or download the .exe file provided

> [!WARNING]
> Keep in mind that in order for the program to be able to send keypresses to MSFS, the SERVER component needs to run as administrator. If you DON'T want to do this, you'll need to add SendFSKey.exe to your Flight Simulator exe.xml file so that it runs at the same privilege level the sim runs.

## Contributing
We welcome contributions! If you'd like to improve SendFSKey or suggest new features, please feel free to fork the repository, make your changes, and submit a pull request. All contributions will be reviewed for potential inclusion in the project.

## License
SendFSKey is distributed under the MIT No Attribution License. See the [LICENSE](LICENSE.md) file for more details.

## Acknowledgements
- A big thank you to the MSFS community for the inspiration and support.
- Contributors who have dedicated their time and effort to enhance this project.
- Spad.NeXt developer for making me do this :) Ulrich, I say this in a good way

## Issues
- For remote Spad.NeXt instalations make sure you DISABLE the "Network Instalation" option, otherwise keypresses will be blocked. It sounds counterintuitive but its by design according to the Spad.NeXt developer.

## Pending items
vJoy support for sending joystick and axis commands over the network. Suggestions and contributions are welcome.