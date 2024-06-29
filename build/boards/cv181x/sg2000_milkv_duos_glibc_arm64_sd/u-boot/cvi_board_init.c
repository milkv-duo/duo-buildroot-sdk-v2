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

	set_rtc_register_for_power();

	return 0;
}
