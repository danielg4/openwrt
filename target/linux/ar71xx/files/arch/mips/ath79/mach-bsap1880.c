/*
 *  Bluesocket BSAP-1800v2 and BSAP-1840 board support, based on mach-eap7660d.c
 *
 *  Copyright 2018 Daniel Gimpelevich <daniel@gimpelevich.san-francisco.ca.us>
 *  Copyright 2010 Daniel Golle <daniel.golle@gmail.com>
 *  Copyright 2008 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright 2008 Imre Kaloz <kaloz@openwrt.org>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/etherdevice.h>
#include <linux/pci.h>
#include <linux/ath9k_platform.h>
#include <linux/delay.h>

#include <asm/mach-ath79/ar71xx_regs.h>
#include <asm/mach-ath79/ath79.h>

#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "machtypes.h"
#include "pci.h"

#define BSAP1880_KEYS_POLL_INTERVAL	20	/* msecs */
#define BSAP1880_KEYS_DEBOUNCE_INTERVAL	(3 * BSAP1880_KEYS_POLL_INTERVAL)

#define BSAP1880_GPIO_WIFI5G_LED	8
#define BSAP1880_GPIO_WIFI2G_LED	7
#define BSAP1880_GPIO_GREEN_LED		5
#define BSAP1880_GPIO_YELLOW_LED	4
#define BSAP1880_GPIO_RESET		6
#define BSAP1880_PHYMASK		BIT(0)
#define BSAP1880_BOARDCONFIG		0x1FFC0000
#define BSAP1880_GBIC_MAC_OFFSET	0x0002

#ifdef CONFIG_PCI
static struct ath9k_platform_data bsap1880_wmac0_data;
static struct ath9k_platform_data bsap1880_wmac1_data;
static char bsap1880_wmac0_mac[6];
static char bsap1880_wmac1_mac[6];

static int bsap1880_pci_plat_dev_init(struct pci_dev *dev)
{
	switch (PCI_SLOT(dev->devfn)) {
	case 17:
		dev->dev.platform_data = &bsap1880_wmac0_data;
		break;

	case 18:
		dev->dev.platform_data = &bsap1880_wmac1_data;
		break;
	}

	return 0;
}

void __init bsap1880_pci_init(void)
{
	bsap1880_wmac0_data.use_eeprom = true;
	bsap1880_wmac1_data.use_eeprom = true;

	bsap1880_wmac0_data.macaddr = bsap1880_wmac0_mac;
	bsap1880_wmac1_data.macaddr = bsap1880_wmac1_mac;

	ath79_pci_set_plat_dev_init(bsap1880_pci_plat_dev_init);
	ath79_register_pci();
}
#else
static inline void bsap1880_pci_init(void)
{
}
#endif /* CONFIG_PCI */

static struct gpio_led bsap1880_leds_gpio[] __initdata = {
	{
		.name		= "bsap1880:green:wifi5g",
		.gpio		= BSAP1880_GPIO_WIFI5G_LED,
		.active_low	= 1,
	},
	{
		.name		= "bsap1880:green:wifi2g",
		.gpio		= BSAP1880_GPIO_WIFI2G_LED,
		.active_low	= 1,
	},
	{
		.name		= "bsap1880:green:status",
		.gpio		= BSAP1880_GPIO_GREEN_LED,
		.active_low	= 1,
	},
	{
		.name		= "bsap1880:yellow:status",
		.gpio		= BSAP1880_GPIO_YELLOW_LED,
		.active_low	= 1,
	}
};

static struct gpio_keys_button bsap1880_gpio_keys[] __initdata = {
	{
		.desc		= "reset",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = BSAP1880_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= BSAP1880_GPIO_RESET,
		.active_low	= 1,
	}
};

static const char *bsap1880_part_probes[] = {
	"RedBoot",
	NULL,
};

static struct flash_platform_data bsap1880_flash_data = {
	.part_probes	= bsap1880_part_probes,
};

static void __init bsap1880_setup(void)
{
	u8 *boardconfig = (u8 *) KSEG1ADDR(BSAP1880_BOARDCONFIG);

	if (!is_valid_ether_addr(ath79_mac_base))
		memcpy(ath79_mac_base, boardconfig
			+ BSAP1880_GBIC_MAC_OFFSET, ETH_ALEN);

	ath79_register_mdio(0, ~BSAP1880_PHYMASK);

	ath79_init_mac(ath79_eth0_data.mac_addr, ath79_mac_base, 0);
	ath79_init_mac(bsap1880_wmac0_mac, ath79_mac_base, 1);
	ath79_init_mac(bsap1880_wmac1_mac, ath79_mac_base, 9);
	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_RGMII_ID;
	ath79_eth0_data.phy_mask = BSAP1880_PHYMASK;
	ath79_register_eth(0);

	ath79_register_m25p80(&bsap1880_flash_data);
	ath79_register_leds_gpio(-1, ARRAY_SIZE(bsap1880_leds_gpio),
					bsap1880_leds_gpio);
	ath79_register_gpio_keys_polled(-1, BSAP1880_KEYS_POLL_INTERVAL,
					 ARRAY_SIZE(bsap1880_gpio_keys),
					 bsap1880_gpio_keys);
	bsap1880_pci_init();
};

MIPS_MACHINE(ATH79_MACH_BSAP1880, "BSAP1880",
		"Adtran/Bluesocket BSAP-1800v2 or BSAP-1840", bsap1880_setup);
