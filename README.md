# settings
Serial flasher config

	Flash size				4MB

Partition Table

	Patition Table			Single factory app (large), no OTA

WiFiControlBase

	LED GPIO number			5

LED controle conf

	LED1 GPIO number		26
	LED2 GPIO number		27
	LED3 GPIO number		32
	LED4 GPIO number		33

Servo controle conf

	Servo GPIO number		12

SD card PIN configuration

	MOSI GPIO number		17
	MOSO GPIO number		19
	CLK GPIO number			18
	CS GPIO number			16
	SD card sw GPIO number	4

OLED(SSD1306) PIN configuration

	I2C SDA GPIO number		21
	I2C SCL GPIO number		22

HTTP Server

	WebSocket server support	TRUE

FAT Filesystem support

	Long filename support	Long filename buffer in heap
	OEM Code Page			Japanese (DBCS) (CP932)

Color settings

	Color depth.			1: 1 byte per pixel

Font usage

Enable built-in fonts

	Enable UNSCII 8 (Perfect monospace font)	TRUE

3rd Party Libraries

	QR code library			TRUE


# _Sample project_

(See the README.md file in the upper level 'examples' directory for more information about examples.)

This is the simplest buildable example. The example is used by command `idf.py create-project`
that copies the project to user specified path and set it's name. For more information follow the [docs page](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project)



## How to use example
We encourage the users to use the example as a template for the new projects.
A recommended way is to follow the instructions on a [docs page](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project).

## Example folder contents

The project **sample_project** contains one source file in C language [main.c](main/main.c). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt`
files that provide set of directives and instructions describing the project's source files and targets
(executable, library, or both). 

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md                  This is the file you are currently reading
```
Additionally, the sample project contains Makefile and component.mk files, used for the legacy Make based build system. 
They are not used or needed when building with CMake and idf.py.
