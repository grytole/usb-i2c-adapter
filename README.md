## usb-i2c-adapter

#### Description

The goal is to create a simple USB to I2C bridge. It must be able to operate as I2C master device in both transmitter and receiver modes. Optionally it must provide interface to additional features or configuration. Adapter must be able to use on PC, router, RPi or any other device with USB host.

---

#### Hardware design

Hardware is based on ready modules: usb-uart bridge and microcontroller development board.

![HW scheme](doc/usb-i2c-adapter-hw-scheme.png)

---

#### Communication protocol

All actions are based on transmitting a command followed by required number of argumens. Every request is followed by success or failure response.

Response is unified. First byte is a prefix with number of following bytes. `00h` means that failure happened during command execution and payload size is zero, what means it is not followed by another data related to this request. Successful command execution has size of response payload followed by argument of request.

##### List of commands

 - **Write byte** - `[00h] [address] [register] [byte]`
 
   TX: `00h 77h F4h 2Eh` - write byte `2Eh` into register `F4h` of device with address `77h`

   RX: `01h 2Eh` - write success, response has one byte with value equal to transmitted

   RX: `00h` - write failure, operation was not acknowledged

 - **Read byte** - `[01h] [address] [register]`

   TX: `01h 1Eh 0Ah` - read byte from register `0Ah` of device with address `1Eh`

   RX: `01h 48h` - read success, response has one byte with value `48h`

   RX: `00h` - read failure, operation was not acknowledged

 - **Detect device** - `[02h] [address]`

   TX: `02h 69h` - detect presence of device with address `69h` on the bus

   RX: `01h 69h` - detect success, response has one byte with value of device address

   RX: `00h` - detect failure, no device with requested address found

---

#### Software design

Adapter code is realized as a state machine. It is a loop of four states: initialization, command receipt, command execution and response with execution result.

---
