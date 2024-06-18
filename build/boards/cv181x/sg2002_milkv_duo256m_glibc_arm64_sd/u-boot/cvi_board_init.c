static void set_rtc_register_for_power(void)
{
	printf("set_rtc_register_for_power\n");
}

int cvi_board_init(void)
{
	// LED
	PINMUX_CONFIG(PWR_GPIO2, PWR_GPIO_2);

	set_rtc_register_for_power();

	return 0;
}
