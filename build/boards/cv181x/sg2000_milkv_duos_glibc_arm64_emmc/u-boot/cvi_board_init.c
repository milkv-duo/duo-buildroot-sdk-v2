static void set_rtc_register_for_power(void)
{
	printf("set_rtc_register_for_power\n");

	// Reset Key
	mmio_write_32(0x050260D0, 0x7);
}

int cvi_board_init(void)
{
	// LED
	PINMUX_CONFIG(IIC0_SDA, XGPIOA_29);

	// USB
	PINMUX_CONFIG(USB_VBUS_EN, XGPIOB_5);

	// I2C4 for TP
	PINMUX_CONFIG(VIVO_D1, IIC4_SCL);
	PINMUX_CONFIG(VIVO_D0, IIC4_SDA);

	// TP INT
	PINMUX_CONFIG(JTAG_CPU_TCK, XGPIOA_18);
	// TP Reset
	PINMUX_CONFIG(JTAG_CPU_TMS, XGPIOA_19);

	// Camera0
	PINMUX_CONFIG(IIC3_SCL, IIC3_SCL);
	PINMUX_CONFIG(IIC3_SDA, IIC3_SDA);
	PINMUX_CONFIG(CAM_MCLK0, CAM_MCLK0); // Sensor0 MCLK
	PINMUX_CONFIG(CAM_RST0, XGPIOA_2);   // Sensor0 RESET

	// Camera1
	PINMUX_CONFIG(IIC2_SDA, IIC2_SDA);
	PINMUX_CONFIG(IIC2_SCL, IIC2_SCL);
	PINMUX_CONFIG(CAM_MCLK1, CAM_MCLK1); // Sensor1 MCLK
	PINMUX_CONFIG(CAM_PD1, XGPIOA_4);    // Sensor1 RESET

	set_rtc_register_for_power();

	return 0;
}
