This is simple PAR-meter based on ambient light sensor TCS34725.

It use blue pill board with STM32F103C8T6 MCU and 1.3" OLED screen.

To compile firmware, use STM32CubeIDE from ST.
To flash firmware, use ST-Link and ST-Link utility from ST.

Default calibration based on sensor sensivity data from TCS34725 datasheet, and works for bare sensor without cover, diffuser, etc...

This firmware provides USB virtual COM port for manual calibration - for Windows, install VCP driver from ST, connect USB port of blue pill
to PC, and open COM port in terminal software like PuTTY. 
Press Enter to show current calibration settings, and short list of supported console commands.

The PAR value calculated by formula:

PAR = K * (Clear * W + Red * R + Green * G + Blue * B), the K W R G - calibration koefficients, Clear, Red, Green, Blue - readed values from sensor,
normalized from 0..max counts [(256 - integration time) * 1024, max 65535] to 0..1 range.

By default, color channels of sensor not used for PAR calclulation, i.e. (W R G B) = (1 0 0 0).

 