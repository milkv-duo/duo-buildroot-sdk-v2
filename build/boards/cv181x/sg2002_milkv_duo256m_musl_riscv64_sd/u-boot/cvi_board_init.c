static void set_rtc_register_for_power(void)
{
	printf("set_rtc_register_for_power\n");
}

int cvi_board_init(void)
{
	// LED
	PINMUX_CONFIG(PWR_GPIO2, PWR_GPIO_2);

	// Camera
	PINMUX_CONFIG(PAD_MIPI_TXM1, IIC2_SDA);    // GP10
	PINMUX_CONFIG(PAD_MIPI_TXP1, IIC2_SCL);    // GP11
	PINMUX_CONFIG(PAD_MIPI_TXP0, CAM_MCLK0);   // Sensor MCLK
	PINMUX_CONFIG(PAD_MIPI_TXP2, XGPIOC_17);   // Sensor RESET

	set_rtc_register_for_power();

	return 0;
}
