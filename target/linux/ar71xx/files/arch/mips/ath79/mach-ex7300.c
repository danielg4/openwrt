/*
 * Netgear EX6400/EX7300 board support, from mach-ap135.c and mach-fritz4020.c
 *
 * Copyright 2018 Daniel Gimpelevich <daniel@gimpelevich.san-francisco.ca.us>
 * Copyright 2018 David Bauer <mail@david-bauer.net>
 * Copyright 2012 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/platform_data/phy-at803x.h>
#include <linux/spi/spi_gpio.h>
#include <linux/spi/74x164.h>

#include <asm/mach-ath79/ar71xx_regs.h>

#include "common.h"
#include "pci.h"
#include "dev-ap9x-pci.h"
#include "dev-gpio-buttons.h"
#include "dev-eth.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define EX7300_KEYS_POLL_INTERVAL	20	/* msecs */
#define EX7300_KEYS_DEBOUNCE_INTERVAL	(3 * EX7300_KEYS_POLL_INTERVAL)

#define EX7300_GPIO_RESET_BTN		11
#define EX7300_GPIO_LED_DATA		15
#define EX7300_GPIO_LED_CLK		18
#define EX7300_GPIO_GREEN_POWER_LED	19
#define EX7300_GPIO_AMBER_POWER_LED	20
#define EX7300_GPIO_WPS_BTN		22
#define EX7300_GPIO_MODE_SWITCH		23
#define EX7300_74AHC164_GPIO_BASE	256
#define EX7300_BLUE_ARROW_LEFT_LED	(EX7300_74AHC164_GPIO_BASE + 7)
#define EX7300_BLUE_ARROW_RIGHT_LED	(EX7300_74AHC164_GPIO_BASE + 6)
#define EX7300_GREEN_WPS_LED		(EX7300_74AHC164_GPIO_BASE + 5)
#define EX7300_RED_CLIENT_LINK_LED	(EX7300_74AHC164_GPIO_BASE + 3)
#define EX7300_GREEN_CLIENT_LINK_LED	(EX7300_74AHC164_GPIO_BASE + 2)
#define EX7300_RED_ROUTER_LINK_LED	(EX7300_74AHC164_GPIO_BASE + 1)
#define EX7300_GREEN_ROUTER_LINK_LED	(EX7300_74AHC164_GPIO_BASE + 0)

#define EX7300_MAC0_OFFSET		0
#define EX7300_MAC1_OFFSET		6
#define EX7300_WMAC_CALDATA_OFFSET	0x1000

static struct spi_gpio_platform_data ex7300_spi_data = {
	.sck		= EX7300_GPIO_LED_CLK,
	.miso		= SPI_GPIO_NO_MISO,
	.mosi		= EX7300_GPIO_LED_DATA,
	.num_chipselect	= 1,
};

static u8 ex7300_ssr_initdata[] = { 0xFF };

static struct gen_74x164_chip_platform_data ex7300_ssr_data = {
	.base = EX7300_74AHC164_GPIO_BASE,
	.num_registers = ARRAY_SIZE(ex7300_ssr_initdata),
	.init_data = ex7300_ssr_initdata,
};

static struct platform_device ex7300_spi_device = {
	.name		= "spi_gpio",
	.id		= 1,
	.dev = {
		.platform_data = &ex7300_spi_data,
	},
};

static struct spi_board_info ex7300_spi_info[] = {
	{
		.bus_num		= 1,
		.chip_select		= 0,
		.max_speed_hz		= 500000,
		.modalias		= "74x164",
		.platform_data		= &ex7300_ssr_data,
		.controller_data	= (void *) 0x0,
	},
};
static struct gpio_led ex7300_leds_gpio[] __initdata = {
	{
		.name		= "netgear:green:power",
		.gpio		= EX7300_GPIO_GREEN_POWER_LED,
		.active_low	= 1,
	},
	{
		.name		= "netgear:amber:power",
		.gpio		= EX7300_GPIO_AMBER_POWER_LED,
		.active_low	= 1,
	},
	{
		.name		= "netgear:blue:left",
		.gpio		= EX7300_BLUE_ARROW_LEFT_LED,
		.active_low	= 1,
	},
	{
		.name		= "netgear:blue:right",
		.gpio		= EX7300_BLUE_ARROW_RIGHT_LED,
		.active_low	= 1,
	},
	{
		.name		= "netgear:green:wps",
		.gpio		= EX7300_GREEN_WPS_LED,
		.active_low	= 1,
	},
	{
		.name		= "netgear:red:client",
		.gpio		= EX7300_RED_CLIENT_LINK_LED,
		.active_low	= 1,
	},
	{
		.name		= "netgear:green:client",
		.gpio		= EX7300_GREEN_CLIENT_LINK_LED,
		.active_low	= 1,
	},
	{
		.name		= "netgear:red:router",
		.gpio		= EX7300_RED_ROUTER_LINK_LED,
		.active_low	= 1,
	},
	{
		.name		= "netgear:green:router",
		.gpio		= EX7300_GREEN_ROUTER_LINK_LED,
		.active_low	= 1,
	}
};

static struct gpio_keys_button ex7300_gpio_keys[] __initdata = {
	{
		.desc		= "reset",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = EX7300_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= EX7300_GPIO_RESET_BTN,
		.active_low	= 1,
	},
	{
		.desc		= "wps",
		.type		= EV_KEY,
		.code		= KEY_WPS_BUTTON,
		.debounce_interval = EX7300_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= EX7300_GPIO_WPS_BTN,
		.active_low	= 1,
	},
	{
		.desc		= "extender_apmode",
		.type		= EV_SW,
		.code		= BTN_0,
		.debounce_interval = EX7300_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= EX7300_GPIO_MODE_SWITCH,
	}
};

static struct at803x_platform_data ex7300_ar803x_data = {
};

static struct mdio_board_info ex7300_mdio0_info[] = {
	{
		.bus_id = "ag71xx-mdio.0",
		.phy_addr = 0,
		.platform_data = &ex7300_ar803x_data,
	},
};

static void __init ex7300_setup(void)
{
	u8 *art = (u8 *) KSEG1ADDR(0x1f050000);
	u8 tmpmac[ETH_ALEN];

	if (!is_valid_ether_addr(ath79_mac_base))
		memcpy(ath79_mac_base, art + EX7300_MAC0_OFFSET, ETH_ALEN);

	ath79_register_m25p80(NULL);

	ath79_init_mac(tmpmac, art + EX7300_MAC1_OFFSET, 0);
	ath79_register_wmac(art + EX7300_WMAC_CALDATA_OFFSET, tmpmac);
	ath79_register_pci();

	ath79_setup_qca955x_eth_cfg(QCA955X_ETH_CFG_RGMII_EN);

	ath79_register_mdio(0, 0x0);

	ath79_init_mac(ath79_eth0_data.mac_addr, ath79_mac_base, 0);

	mdiobus_register_board_info(ex7300_mdio0_info,
				    ARRAY_SIZE(ex7300_mdio0_info));

	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_RGMII;
	ath79_eth0_data.phy_mask = BIT(4);
	ath79_eth0_data.mii_bus_dev = &ath79_mdio0_device.dev;
	/* set default 1000/100/10 pll value */
	ath79_eth0_pll_data.pll_1000	= 0x86000000;
	ath79_eth0_pll_data.pll_100	= 0x80000101;
	ath79_eth0_pll_data.pll_10	= 0x80001313;
	ath79_register_eth(0);

	spi_register_board_info(ex7300_spi_info,
				ARRAY_SIZE(ex7300_spi_info));
	platform_device_register(&ex7300_spi_device);

	ath79_register_gpio_keys_polled(-1, EX7300_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(ex7300_gpio_keys),
					ex7300_gpio_keys);

	ath79_register_leds_gpio(-1, ARRAY_SIZE(ex7300_leds_gpio),
				 ex7300_leds_gpio);
}

MIPS_MACHINE(ATH79_MACH_EX7300, "EX7300", "NETGEAR EX6400/EX7300",
	     ex7300_setup);
