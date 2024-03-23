# SendFSKey - Send KeyPress actions over a network to MSFS

## Overview
SendFSKey is a lightweight application designed for seamless remote keyboard input in Microsoft Flight Simulator (MSFS) from a local network. Its meant to be used by cockpit builders or developers to facilitate sending keyboard commands to the sim over a local network. Existing programs like FSUIPC and Key2Lan have this functionality but its so terrible and unintuitive that I decided to write my own program and make it free.

## Features
- **Dual-Mode Functionality**: Operate in either remote or client mode for flexible usage scenarios with a single executable.
- **Efficient and Lightweight**: Crafted with C++ for maximum efficiency and minimal resource usage. No CPU usage and very fast persistent network connections
- **Correctly interprets KEY_UP and KEY_DOWN events and allows clients to determine time between presses.
- **Fast and Resilient**: Engineered to be incredibly fast, ensuring real-time performance with built-in resilience to network instabilities.
- **User-Friendly**: Easy to set up and use, regardless of your technical background.
- **Open Source**: Freely available for personal and commercial use under the MIT No Attribution License.

## Getting Started

### Prerequisites
- Microsoft Flight Simulator 2020 installed on the server computer.
- A network connection between the client and server machines.

### Installation
Clone the repository and compile the project.

## Contributing
We welcome contributions! If you'd like to improve SendFSKey or suggest new features, please feel free to fork the repository, make your changes, and submit a pull request. All contributions will be reviewed for potential inclusion in the project.

## License
SendFSKey is distributed under the MIT No Attribution License. See the [LICENSE](LICENSE.md) file for more details.

## Acknowledgements
- A big thank you to the MSFS community for the inspiration and support.
- Contributors who have dedicated their time and effort to enhance this project.
- Spad NeXt developer for making me do this :) Ulrich, I say this in a good way