// SPDX-License-Identifier: GPL-2.0
/* dwmac-cvitek.c - Bitmain DWMAC specific glue layer
 *
 * Copyright (c) 2019 Cvitek Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/stmmac.h>
#include <linux/module.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/of_net.h>
#include <linux/of_gpio.h>
#include <linux/io.h>

#include "stmmac_platform.h"

#ifdef CONFIG_PM_SLEEP
#if defined(CONFIG_ARCH_CV180X) || defined(CONFIG_ARCH_CV181X)
#include "dwmac1000.h"
#include "dwmac_dma.h"
#include "hwif.h"
#endif
#endif

struct cvitek_mac {
	struct device *dev;
	struct reset_control *rst;
	struct clk *clk_tx;
	struct clk *gate_clk_500m;
	struct clk *gate_clk_axi4;
	struct gpio_desc *reset;
};

static u64 bm_dma_mask = DMA_BIT_MASK(40);

static int bm_eth_reset_phy(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int phy_reset_gpio;
	u32	ephy_base_addr = 0x0;
	void __iomem *ephy_base_reg;
	u32	ephy_top_addr = 0x0;
	void __iomem *ephy_top_reg;

	if (!np)
		return 0;

	of_property_read_u32(np, "ephy_base_reg", &ephy_base_addr);
	of_property_read_u32(np, "ephy_top_reg", &ephy_top_addr);

	if (ephy_base_addr && ephy_top_addr) {
		ephy_base_reg = ioremap(ephy_base_addr, 0x80);
		ephy_top_reg = ioremap(ephy_top_addr, 0x10);
#ifdef CONFIG_PM_SLEEP
#if defined(CONFIG_ARCH_CV180X) || defined(CONFIG_ARCH_CV181X)
		// set rg_ephy_apb_rw_sel 0x0804@[0]=1/APB by using APB interface
		writel(0x0001, ephy_top_reg + 0x4);

		// Release 0x0800[0]=0/shutdown
		writel(0x0900, ephy_top_reg);

		// Release 0x0800[2]=1/dig_rst_n, Let mii_reg can be accessabile
		writel(0x0904, ephy_top_reg);
		// ANA INIT (PD/EN), switch to MII-page5
		writel(0x0500, ephy_base_reg + 0x7c);
		// Release ANA_PD p5.0x10@[13:8] = 6'b001100
		writel(0x0c00, ephy_base_reg + 0x40);
		// Release ANA_EN p5.0x10@[7:0] = 8'b01111110
		writel(0x0c7e, ephy_base_reg + 0x40);
		// Wait PLL_Lock, Lock_Status p5.0x12@[15] = 1
		//mdelay(1);

		// Release 0x0800[1] = 1/ana_rst_n
		writel(0x0906, ephy_top_reg);

		// ANA INIT
		// @Switch to MII-page5
		writel(0x0500, ephy_base_reg + 0x7c);
		// PHY_ID
		writel(0x0043, ephy_base_reg + 0x8);
		writel(0x5649, ephy_base_reg + 0xc);
		// switch to MDIO control by ETH_MAC
		writel(0x0, ephy_top_reg + 0x4);
#endif
#endif
		iounmap(ephy_base_reg);
		iounmap(ephy_top_reg);
	}

	phy_reset_gpio = of_get_named_gpio(np, "phy-reset-gpios", 0);

	if (phy_reset_gpio < 0)
		return 0;

	if (gpio_request(phy_reset_gpio, "eth-phy-reset"))
		return 0;

	/* RESET_PU */
	gpio_direction_output(phy_reset_gpio, 0);
	mdelay(20);

	gpio_direction_output(phy_reset_gpio, 1);
	/* RC charging time */
	mdelay(60);

	return 0;
}

void bm_dwmac_exit(struct platform_device *pdev, void *priv)
{
	struct cvitek_mac *bsp_priv = priv;

	clk_disable_unprepare(bsp_priv->gate_clk_500m);
	clk_disable_unprepare(bsp_priv->gate_clk_axi4);
}

static int bm_dwmac_probe(struct platform_device *pdev)
{
	struct plat_stmmacenet_data *plat_dat;
	struct stmmac_resources stmmac_res;
	struct cvitek_mac *bsp_priv = NULL;
	int ret;

	pdev->dev.dma_mask = &bm_dma_mask;
	pdev->dev.coherent_dma_mask = bm_dma_mask;

	bm_eth_reset_phy(pdev);

	ret = stmmac_get_platform_resources(pdev, &stmmac_res);
	if (ret)
		return ret;

	plat_dat = stmmac_probe_config_dt(pdev, &stmmac_res.mac);
	if (IS_ERR(plat_dat))
		return PTR_ERR(plat_dat);

	ret = stmmac_dvr_probe(&pdev->dev, plat_dat, &stmmac_res);
	if (ret)
		goto err_remove_config_dt;

	bsp_priv = devm_kzalloc(&pdev->dev, sizeof(*bsp_priv), GFP_KERNEL);
	if (!bsp_priv)
		return PTR_ERR(bsp_priv);

	bsp_priv->dev = &pdev->dev;

	/* clock setup */
	bsp_priv->gate_clk_500m = devm_clk_get(&pdev->dev, "clk_500m_eth");

	if (IS_ERR(bsp_priv->gate_clk_500m))
		dev_warn(&pdev->dev, "Cannot get clk_500m_eth!\n");
	else
		clk_prepare_enable(bsp_priv->gate_clk_500m);

	bsp_priv->gate_clk_axi4 = devm_clk_get(&pdev->dev, "clk_axi4_eth");

	if (IS_ERR(bsp_priv->gate_clk_axi4))
		dev_warn(&pdev->dev, "Cannot get gate_clk_axi4!\n");
	else
		clk_prepare_enable(bsp_priv->gate_clk_axi4);

	plat_dat->bsp_priv = bsp_priv;
	plat_dat->exit = bm_dwmac_exit;

	return 0;

err_remove_config_dt:
	stmmac_remove_config_dt(pdev, plat_dat);

	return ret;
}

static const struct of_device_id bm_dwmac_match[] = {
	{ .compatible = "cvitek,ethernet" },
	{ }
};
MODULE_DEVICE_TABLE(of, bm_dwmac_match);

#ifdef CONFIG_PM_SLEEP
#if defined(CONFIG_ARCH_CV180X) || defined(CONFIG_ARCH_CV181X)

static int cvi_eth_pm_suspend(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);

	if (!priv->reg_ctx) {
		priv->reg_ctx = devm_kzalloc(priv->device, sizeof(struct stmmac_reg_context), GFP_KERNEL);
		if (!priv->reg_ctx)
			return -ENOMEM;
	}
	priv->reg_ctx->ctrl = readl(priv->ioaddr + GMAC_CONTROL);
	priv->reg_ctx->frame_filter = readl(priv->ioaddr + GMAC_FRAME_FILTER);
	priv->reg_ctx->hash_high = readl(priv->ioaddr + GMAC_HASH_HIGH);
	priv->reg_ctx->hash_low = readl(priv->ioaddr + GMAC_HASH_LOW);
	priv->reg_ctx->mii_addr = readl(priv->ioaddr + GMAC_MII_ADDR);
	priv->reg_ctx->mii_data = readl(priv->ioaddr + GMAC_MII_DATA);
	priv->reg_ctx->flow_ctrl = readl(priv->ioaddr + GMAC_FLOW_CTRL);
	priv->reg_ctx->vlan_tag = readl(priv->ioaddr + GMAC_VLAN_TAG);
	priv->reg_ctx->debug = readl(priv->ioaddr + GMAC_DEBUG);
	priv->reg_ctx->wakeup_fileter = readl(priv->ioaddr + GMAC_WAKEUP_FILTER);
	priv->reg_ctx->lpi_ctrl_status = readl(priv->ioaddr + LPI_CTRL_STATUS);
	priv->reg_ctx->lpi_timer_ctrl = readl(priv->ioaddr + LPI_TIMER_CTRL);
	priv->reg_ctx->int_mask = readl(priv->ioaddr + GMAC_INT_MASK);
	priv->reg_ctx->mac_addr0_high = readl(priv->ioaddr + GMAC_MAC_ADDR0_HIGH);
	priv->reg_ctx->mac_addr0_low = readl(priv->ioaddr + GMAC_MAC_ADDR0_LOW);
	priv->reg_ctx->pcs_base = readl(priv->ioaddr + GMAC_PCS_BASE);
	priv->reg_ctx->mmc_ctrl = readl(priv->ioaddr + GMAC_MMC_CTRL);
	priv->reg_ctx->mmc_rx_intr_mask = readl(priv->ioaddr + GMAC_MMC_RX_INTR_MASK);
	priv->reg_ctx->mmc_tx_intr_mask = readl(priv->ioaddr + GMAC_MMC_TX_INTR_MASK);
	priv->reg_ctx->mmc_ipc_rx_intr_mask = readl(priv->ioaddr + GMAC_MMC_IPC_RX_INTR_MASK);
	priv->reg_ctx->mmc_rx_csum_offload = readl(priv->ioaddr + GMAC_MMC_RX_CSUM_OFFLOAD);
	priv->reg_ctx->dma_bus_mode = readl(priv->ioaddr + DMA_BUS_MODE);
	priv->reg_ctx->dma_rx_base_addr = readl(priv->ioaddr + DMA_RCV_BASE_ADDR);
	priv->reg_ctx->dma_tx_base_addr = readl(priv->ioaddr + DMA_TX_BASE_ADDR);
	priv->reg_ctx->dma_ctrl = readl(priv->ioaddr + DMA_CONTROL);
	priv->reg_ctx->dma_intr_ena = readl(priv->ioaddr + DMA_INTR_ENA);
	priv->reg_ctx->dma_rx_watchdog = readl(priv->ioaddr + DMA_RX_WATCHDOG);
	priv->reg_ctx->dma_axi_bus_mode = readl(priv->ioaddr + DMA_AXI_BUS_MODE);

	return 0;
}

extern void stmmac_reset_subtask2(struct stmmac_priv *priv);

static int cvi_eth_pm_resume(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct platform_device *pdev = to_platform_device(dev);

	bm_eth_reset_phy(pdev);
	writel(priv->reg_ctx->ctrl, priv->ioaddr + GMAC_CONTROL);
	writel(priv->reg_ctx->frame_filter, priv->ioaddr + GMAC_FRAME_FILTER);
	writel(priv->reg_ctx->hash_high, priv->ioaddr + GMAC_HASH_HIGH);
	writel(priv->reg_ctx->hash_low, priv->ioaddr + GMAC_HASH_LOW);
	writel(priv->reg_ctx->mii_addr, priv->ioaddr + GMAC_MII_ADDR);
	writel(priv->reg_ctx->mii_data, priv->ioaddr + GMAC_MII_DATA);
	writel(priv->reg_ctx->flow_ctrl, priv->ioaddr + GMAC_FLOW_CTRL);
	writel(priv->reg_ctx->vlan_tag, priv->ioaddr + GMAC_VLAN_TAG);
	writel(priv->reg_ctx->debug, priv->ioaddr + GMAC_DEBUG);
	writel(priv->reg_ctx->wakeup_fileter, priv->ioaddr + GMAC_WAKEUP_FILTER);
	writel(priv->reg_ctx->lpi_ctrl_status, priv->ioaddr + LPI_CTRL_STATUS);
	writel(priv->reg_ctx->lpi_timer_ctrl, priv->ioaddr + LPI_TIMER_CTRL);
	writel(priv->reg_ctx->int_mask, priv->ioaddr + GMAC_INT_MASK);
	writel(priv->reg_ctx->mac_addr0_high, priv->ioaddr + GMAC_MAC_ADDR0_HIGH);
	writel(priv->reg_ctx->mac_addr0_low, priv->ioaddr + GMAC_MAC_ADDR0_LOW);
	writel(priv->reg_ctx->pcs_base, priv->ioaddr + GMAC_PCS_BASE);
	writel(priv->reg_ctx->mmc_ctrl, priv->ioaddr + GMAC_MMC_CTRL);
	writel(priv->reg_ctx->mmc_rx_intr_mask, priv->ioaddr + GMAC_MMC_RX_INTR_MASK);
	writel(priv->reg_ctx->mmc_tx_intr_mask, priv->ioaddr + GMAC_MMC_TX_INTR_MASK);
	writel(priv->reg_ctx->mmc_ipc_rx_intr_mask, priv->ioaddr + GMAC_MMC_IPC_RX_INTR_MASK);
	writel(priv->reg_ctx->mmc_rx_csum_offload, priv->ioaddr + GMAC_MMC_RX_CSUM_OFFLOAD);
	writel(priv->reg_ctx->dma_bus_mode | 0x1, priv->ioaddr + DMA_BUS_MODE);

	stmmac_reset_subtask2(priv);

	return 0;
}
#else
#define cvi_eth_pm_suspend	NULL
#define cvi_eth_pm_resume	NULL
#endif
static const struct dev_pm_ops cvi_eth_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(cvi_eth_pm_suspend, cvi_eth_pm_resume)
};
#endif

static struct platform_driver bm_dwmac_driver = {
	.probe  = bm_dwmac_probe,
	.remove = stmmac_pltfr_remove,
	.driver = {
		.name           = "bm-dwmac",
#ifdef CONFIG_PM_SLEEP
#if defined(CONFIG_ARCH_CV180X) || defined(CONFIG_ARCH_CV181X)
		.pm		= &cvi_eth_pm_ops,
#else
		.pm		= &stmmac_pltfr_pm_ops,
#endif
#endif
		.of_match_table = bm_dwmac_match,
	},
};
module_platform_driver(bm_dwmac_driver);

MODULE_AUTHOR("Wei Huang<wei.huang01@bitmain.com>");
MODULE_DESCRIPTION("Cvitek DWMAC specific glue layer");
MODULE_LICENSE("GPL");
