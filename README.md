# st7796s
DRM driver for st7796s (SPI interface)


# st7796s
DRM driver for st7796s (SPI interface)


install:  
git clone https://github.com/Sergiy-83/st7796s  
cd st7796s  
make  
make install  


Connection example in the device tree  

``` 
&spi1
	{
	status = "okay";
	pinctrl-0 = <&spi1_3pins_PD>;
	pinctrl-names = "default";

	spi1_display: display@0
		{
		compatible = "sitronix,st7796s";
		
		reg 	   = <0>;
		//backlight  = <&backlight_pwm7>;

		spi-max-frequency = <64000000>;
		
		dc-gpios = <&pio 3 13 0>; //PD13
		reset-gpios = <&pio 3 14 0>; //PD14
		
		bgr;
		fps 	 = <25>;
		buswidth = <8>;
		rotation = <270>;
		status 	 = "okay";
		debug	 = <3>;
		};
	};
```
