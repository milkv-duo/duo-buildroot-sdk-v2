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

	set_rtc_register_for_power();

	return 0;
}
