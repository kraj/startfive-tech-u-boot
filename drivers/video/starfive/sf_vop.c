// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017 Theobroma Systems Design und Consulting GmbH
 * Copyright (c) 2015 Google, Inc
 * Copyright 2014 Rockchip Inc.
 */

#include <common.h>
#include <display.h>
#include <dm.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>
#include <dm/lists.h>

#include <regmap.h>
#include <syscon.h>
#include <video.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/delay.h>

#include <power-domain-uclass.h>
#include <power-domain.h>
#include <clk.h>
#include <video_bridge.h>
#include <power/pmic.h>
#include <panel.h>

#include "sf_vop.h"

DECLARE_GLOBAL_DATA_PTR;

static int sf_vop_power(struct udevice *dev)
{
	struct udevice *dev_power;
	struct udevice *dev_pmic;
	struct power_domain_ops *ops;
	struct power_domain power_domain;
	int ret;
	if (!(gd->flags & GD_FLG_RELOC))
		 return 0;
	ret = uclass_find_first_device(UCLASS_POWER_DOMAIN, &dev_power);
	if (ret)
		return ret;

	ret = device_probe(dev_power);
	if (ret) {
		pr_err("%s: device '%s' display won't probe (ret=%d)\n",
		   __func__, dev_power->name, ret);
		return ret;
	}

	ops = (struct power_domain_ops *)dev_power->driver->ops;
	power_domain.dev = dev_power;
	power_domain.id = 4;
	ret = ops->request(&power_domain);
	if (ret) {
		pr_err("ops->request() failed: %d\n", ret);
		return ret;
	}

	ret = ops->on(&power_domain);
	if (ret) {
		pr_err("ops->on() failed: %d\n", ret);
		return ret;
	}

	ret = uclass_get_device_by_driver(UCLASS_PMIC,
			  DM_DRIVER_GET(pmic_starfive), &dev_pmic);
	if (ret) {
		pr_err("failed to find PMIC: %d\n", ret);
		return ret;
	}

	ret = pmic_clrsetbits(dev_pmic, POWER_SW_0_REG, 0x3f, 0x3f);
	if (ret) {
		pr_err("failed to update SD control register: %d", ret);
		return ret;
	}

	return 0;
}

static int vout_get_rst_clock_resources(struct udevice *dev)
{
	struct sf_vop_priv *priv = dev_get_priv(dev);
	int ret;

	ret = clk_get_by_name(dev, "disp_axi", &priv->disp_axi);
	if (ret) {
		pr_err("clk_get_by_name(noc_disp) failed: %d", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "vout_src", &priv->vout_src);
	if (ret) {
		pr_err("clk_get_by_name(vout_src) failed: %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "top_vout_axi", &priv->top_vout_axi);
	if (ret) {
		pr_err("clk_get_by_name(top_vout_axi) failed: %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "top_vout_ahb", &priv->top_vout_ahb);
	if (ret) {
		pr_err("clk_get_by_name(top_vout_ahb) failed: %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "dc_pix0", &priv->dc_pix0);
	if (ret) {
		pr_err("clk_get_by_name(dc_pix0) failed: %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "dc_pix1", &priv->dc_pix1);
	if (ret) {
		pr_err("clk_get_by_name(dc_pix0) failed: %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "dc_axi", &priv->dc_axi);
	if (ret) {
		pr_err("clk_get_by_name(dc_axi) failed: %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "dc_core", &priv->dc_core);
	if (ret) {
		pr_err("clk_get_by_name(dc_core) failed: %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "dc_ahb", &priv->dc_ahb);
	if (ret) {
		pr_err("clk_get_by_name(dc_ahb) failed: %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "top_vout_lcd", &priv->top_vout_lcd);
	if (ret) {
		pr_err("clk_get_by_name(top_vout_lcd) failed: %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "hdmitx0_pixelclk", &priv->hdmitx0_pixelclk);
	if (ret) {
		pr_err("clk_get_by_name(hdmitx0_pixelclk) failed: %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "dc8200_pix0", &priv->dc_pix_src);
	if (ret) {
		pr_err("clk_get_by_name(dc_pix_src) failed: %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "dc8200_pix0_out", &priv->dc_pix0_out);
	if (ret) {
		pr_err("clk_get_by_name(dc_pix0_out) failed: %d\n", ret);
		return ret;
	}

	ret = clk_get_by_name(dev, "dc8200_pix1_out", &priv->dc_pix1_out);
	if (ret) {
		pr_err("clk_get_by_name(dc_ahb) failed: %d\n", ret);
		return ret;
	}

	ret = reset_get_by_name(dev, "rst_vout_src", &priv->rst_vout_src);
	if (ret) {
		pr_err("failed to get rst_vout_src reset (ret=%d)\n", ret);
		return ret;
	}

	ret = reset_get_by_name(dev, "rst_axi", &priv->dc8200_rst_axi);
	if (ret) {
		pr_err("failed to get dc8200_rst_axi reset (ret=%d)\n", ret);
		return ret;
	}
	ret = reset_get_by_name(dev, "rst_ahb", &priv->dc8200_rst_ahb);
	if (ret) {
		pr_err("failed to get ahb reset (ret=%d)\n", ret);
		return ret;
	}
	ret = reset_get_by_name(dev, "rst_core", &priv->dc8200_rst_core);
	if (ret) {
		pr_err("failed to get dc8200_rst_core reset (ret=%d)\n", ret);
		return ret;
	}
	ret = reset_get_by_name(dev, "rst_noc_disp", &priv->noc_disp);
	if (ret) {
		pr_err("failed to get noc_disp reset (ret=%d)\n", ret);
		return ret;
	}

	debug("%s: OK\n", __func__);
	return 0;
}



int dc_hw_init(struct udevice *dev)
{
	struct sf_vop_priv *priv = dev_get_priv(dev);

	u32 revision = readl(priv->regs_hi+DC_HW_REVISION);
	u32 cid = readl(priv->regs_hi+DC_HW_CHIP_CID);

	debug("%s: revision = %08x\n", __func__,revision);
	debug("%s: cid = %08x\n", __func__,cid);

	return 0;
}

static int vout_probe_resources_jh7110(struct udevice *dev)
{
	struct sf_vop_priv *priv = dev_get_priv(dev);
	int ret;
	ret = vout_get_rst_clock_resources(dev);

	ret = clk_enable(&priv->disp_axi);
	if (ret < 0) {
		pr_err("clk_enable(noc_disp) failed: %d\n", ret);
		return ret;
	}
	ret = reset_deassert(&priv->noc_disp);
	if (ret) {
		pr_err("failed to deassert noc_disp reset (ret=%d)\n", ret);
		goto free_clock_vout_src;
	}
	ret = clk_enable(&priv->vout_src);
	if (ret < 0) {
		pr_err("clk_enable(vout_src) failed: %d\n", ret);
		goto free_clock_vout_src;
	}
	ret = clk_enable(&priv->top_vout_axi);
	if (ret < 0) {
		pr_err("clk_enable(top_vout_axi) failed: %d\n", ret);
		goto free_clock_top_vout_axi;
	}
	ret = clk_enable(&priv->top_vout_ahb);
	if (ret < 0) {
		pr_err("clk_enable(top_vout_ahb) failed: %d\n", ret);
		goto free_clock_top_vout_ahb;
	}

	ret = reset_deassert(&priv->rst_vout_src);
	if (ret) {
		pr_err("failed to deassert rst_vout_src reset (ret=%d)\n", ret);
		goto free_clock_dc_pix0;
	}
	ret = clk_enable(&priv->dc_pix0);
	if (ret < 0) {
		pr_err("clk_enable(dc_pix0) failed: %d\n", ret);
		goto free_clock_dc_pix0;
	}

	ret = clk_enable(&priv->dc_pix1);
	if (ret < 0) {
		pr_err("clk_enable(dc_pix1) failed: %d\n", ret);
		goto free_clock_dc_pix1;
	}

	ret = clk_enable(&priv->dc_axi);
	if (ret < 0) {
		pr_err("clk_enable(dc_axi) failed: %d\n", ret);
		goto free_clock_dc_axi;
	}

	ret = clk_enable(&priv->dc_core);
	if (ret < 0) {
		pr_err("clk_enable(dc_core) failed: %d\n", ret);
		goto free_clock_dc_core;
	}

	ret = clk_enable(&priv->dc_ahb);
	if (ret < 0) {
		pr_err("clk_enable(dc_ahb) failed: %d\n", ret);
		goto free_clock_dc_ahb;
	}

	ret = clk_enable(&priv->hdmitx0_pixelclk);
	if (ret < 0) {
		pr_err("clk_enable(hdmitx0_pixelclk) failed: %d\n", ret);
		goto free_clock_hdmitx0_pixelclk;
	}

	ret = reset_deassert(&priv->dc8200_rst_axi);
	if (ret) {
		pr_err("failed to deassert dc8200_rst_axi reset (ret=%d)\n", ret);
		goto free_reset_dc8200;
	}

	ret = reset_deassert(&priv->dc8200_rst_core);
	if (ret) {
		pr_err("failed to deassert dc8200_rst_axi reset (ret=%d)\n", ret);
		goto free_reset_dc8200;
	}

	ret = reset_deassert(&priv->dc8200_rst_ahb);
	if (ret) {
		pr_err("failed to deassert dc8200_rst_ahb reset (ret=%d)\n", ret);
		goto free_reset_dc8200;
	}

	debug("%s: OK\n", __func__);
	return 0;

free_reset_dc8200:
	clk_disable(&priv->hdmitx0_pixelclk);
free_clock_hdmitx0_pixelclk:
	clk_disable(&priv->dc_ahb);
free_clock_dc_ahb:
	clk_disable(&priv->dc_core);
free_clock_dc_core:
	clk_disable(&priv->dc_axi);
free_clock_dc_axi:
	clk_disable(&priv->dc_pix1);
free_clock_dc_pix1:
	clk_disable(&priv->dc_pix0);
free_clock_dc_pix0:
	clk_disable(&priv->top_vout_ahb);
free_clock_top_vout_ahb:
	clk_disable(&priv->top_vout_axi);
free_clock_top_vout_axi:
	clk_disable(&priv->vout_src);
free_clock_vout_src:
	clk_disable(&priv->disp_axi);

	return ret;

}

static int sf_display_init(struct udevice *dev, ulong fbbase, ofnode ep_node)
{
	struct video_uc_plat *uc_plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	struct sf_vop_priv *priv = dev_get_priv(dev);
	int vop_id, remote_vop_id;
	struct display_timing timing;
	struct udevice *disp;
	int ret;
	u32 remote_phandle;
	ofnode remote;
	const char *compat;

	struct udevice *panel = NULL;

	ret = ofnode_read_u32(ep_node, "remote-endpoint", &remote_phandle);
	if (ret)
		return ret;

	remote = ofnode_get_by_phandle(remote_phandle);
	if (!ofnode_valid(remote))
		return -EINVAL;
	remote_vop_id = ofnode_read_u32_default(remote, "reg", -1);
	uc_priv->bpix = VIDEO_BPP32;

	/*
	 * The remote-endpoint references into a subnode of the encoder
	 * (i.e. HDMI, MIPI, etc.) with the DTS looking something like
	 * the following (assume 'hdmi_in_vopl' to be referenced):
	 *
	 * hdmi: hdmi@ff940000 {
	 *   ports {
	 *     hdmi_in: port {
	 *       hdmi_in_vopb: endpoint@0 { ... };
	 *       hdmi_in_vopl: endpoint@1 { ... };
	 *     }
	 *   }
	 * }
	 *
	 * The original code had 3 steps of "walking the parent", but
	 * a much better (as in: less likely to break if the DTS
	 * changes) way of doing this is to "find the enclosing device
	 * of UCLASS_DISPLAY".
	 */
	while (ofnode_valid(remote)) {
		remote = ofnode_get_parent(remote);
		if (!ofnode_valid(remote)) {
			debug("%s(%s): no UCLASS_DISPLAY for remote-endpoint\n",
			      __func__, dev_read_name(dev));
			return -EINVAL;
		}

		uclass_find_device_by_ofnode(UCLASS_VIDEO_BRIDGE, remote, &disp);
		if (disp)
			break;
	};
	compat = ofnode_get_property(remote, "compatible", NULL);
	if (!compat) {
		printf("%s(%s): Failed to find compatible property\n",
		      __func__, dev_read_name(dev));
		return -EINVAL;
	}
	if (strstr(compat, "edp")) {
		vop_id = VOP_MODE_EDP;
	} else if (strstr(compat, "mipi")) {
		vop_id = VOP_MODE_MIPI;
	} else if (strstr(compat, "hdmi")) {
		vop_id = VOP_MODE_HDMI;
	} else if (strstr(compat, "cdn-dp")) {
		vop_id = VOP_MODE_DP;
	} else if (strstr(compat, "lvds")) {
		vop_id = VOP_MODE_LVDS;
	} else {
		printf("%s(%s): Failed to find vop mode for %s\n",
		      __func__, dev_read_name(dev), compat);
		return -EINVAL;
	}

	ret = device_probe(disp);
	if (ret) {
		printf("%s: device '%s' display won't probe (ret=%d)\n",
		      __func__, dev->name, ret);
		return ret;
	}
	debug("%s,vop_id = %d\n", __func__,vop_id);

	if(vop_id == VOP_MODE_MIPI)
	{
		ret = video_bridge_attach(disp);
		if (ret) {
			printf("fail to attach bridge\n");
			return ret;
		}

		ret = video_bridge_set_backlight(disp, 80);
		if (ret) {
			debug("dp: failed to set backlight\n");
			return ret;
		}

		ret = uclass_first_device_err(UCLASS_PANEL, &panel);
		if (ret) {
			if (ret != -ENODEV)
				printf("panel device error %d\n", ret);
			return ret;
		}

		ret = panel_get_display_timing(panel, &timing);
		if (ret) {
			ret = ofnode_decode_display_timing(dev_ofnode(panel),
							   0, &timing);
			if (ret) {
				printf("decode display timing error %d\n", ret);
				return ret;
			}
		}

		int err = clk_set_parent(&priv->dc_pix0, &priv->dc_pix_src);
		if (err) {
			printf("failed to set %s clock as %s's parent\n",
				priv->dc_pix_src.dev->name, priv->dc_pix0.dev->name);
			return err;
		}

		ulong new_rate = clk_set_rate(&priv->dc_pix_src, timing.pixelclock.typ);
		debug("new_rate  %ld\n", new_rate);

		dc_hw_init(dev);

		uc_priv->xsize = timing.hactive.typ;
		uc_priv->ysize = timing.vactive.typ;

		if (IS_ENABLED(CONFIG_VIDEO_COPY))
			uc_plat->copy_base = uc_plat->base - uc_plat->size / 2;

		writel(0xc0001fff, priv->regs_hi+0x00000014); //csr_reg
		writel(0x000000e8, priv->regs_hi+0x00001a38); //csr_reg
		writel(0x00002000, priv->regs_hi+0x00001cc0); //csr_reg
		writel(0x00000000, priv->regs_hi+0x000024d8); //csr_reg
		writel(0x03c00438, priv->regs_hi+0x000024e0); //csr_reg
		writel(0x03c00438, priv->regs_hi+0x00001810); //csr_reg
		writel(uc_plat->base, priv->regs_hi+0x00001400);
		writel(0x000010e0, priv->regs_hi+0x00001408); //csr_reg
		writel(0x000fb00b, priv->regs_hi+0x00001ce8); //csr_reg
		writel(0x0000a9a3, priv->regs_hi+0x00002510); //csr_reg
		writel(0x2c4e6f06, priv->regs_hi+0x00002508); //csr_reg
		writel(0xe6daec4f, priv->regs_hi+0x00002500); //csr_reg
		writel(0x18220000, priv->regs_hi+0x00001518); //csr_reg
		writel(0x00003000, priv->regs_hi+0x00001cc0); //csr_reg
		writel(0x00030000, priv->regs_hi+0x00001cc4); //csr_reg
		writel(0x00030000, priv->regs_hi+0x00001cc4); //csr_reg
		writel(0x00050c1a, priv->regs_hi+0x00001540); //csr_reg
		writel(0x00000001, priv->regs_hi+0x00002540); //csr_reg
		writel(0x00050c1a, priv->regs_hi+0x00001540); //csr_reg
		writel(0x4016120c, priv->regs_hi+0x00001544); //csr_reg
		writel(0x00000002, priv->regs_hi+0x00002544); //csr_reg
		writel(0x4016120c, priv->regs_hi+0x00001544); //csr_reg
		writel(0x001b1208, priv->regs_hi+0x00001548); //csr_reg
		writel(0x00000004, priv->regs_hi+0x00002548); //csr_reg
		writel(0x001b1208, priv->regs_hi+0x00001548); //csr_reg
		writel(0x0016110e, priv->regs_hi+0x0000154c); //csr_reg
		writel(0x00000005, priv->regs_hi+0x0000254c); //csr_reg
		writel(0x0016110e, priv->regs_hi+0x0000154c); //csr_reg
		writel(0x00000001, priv->regs_hi+0x00002518); //csr_reg
		writel(0x00000000, priv->regs_hi+0x00001a28); //csr_reg
		writel(0x03840320, priv->regs_hi+0x00001430); //csr_reg, hsize, htotal
		writel(0xc1bf837a, priv->regs_hi+0x00001438); //csr_reg, hsize blanking
		writel(0x022601e0, priv->regs_hi+0x00001440); //csr_reg, vsize
		writel(0xc110021c, priv->regs_hi+0x00001448); //csr_reg, vsize blanking
		writel(0x00000000, priv->regs_hi+0x000014b0); //csr_reg
		writel(0x000000e2, priv->regs_hi+0x00001cd0); //csr_reg
		writel(0x000000af, priv->regs_hi+0x000014d0); //csr_reg
		writel(0x00000005, priv->regs_hi+0x000014b8); //csr_reg
		writel(0x8dd0b774, priv->regs_hi+0x00001528); //csr_reg
		writel(0x00001111, priv->regs_hi+0x00001418); //csr_reg
		writel(0x00000000, priv->regs_hi+0x00001410); //csr_reg
		writel(0x00000000, priv->regs_hi+0x00002518); //csr_reg
		writel(0x00000006, priv->regs_hi+0x00001468); //csr_reg
		writel(0x00000000, priv->regs_hi+0x00001484); //csr_reg
		writel(0x00000006, priv->regs_hi+0x00001468); //csr_reg
		writel(0x00011b25, priv->regs_hi+0x000024e8); //csr_reg
		writel(0x00000000, priv->regs_hi+0x000024fc); //csr_reg
		writel(0x00011b25, priv->regs_hi+0x000024e8); //csr_reg
		writel(0x00000001, priv->regs_hi+0x00001ccc); //csr_reg
	}

	return 0;
}

static int sf_vop_probe(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct sf_vop_priv *priv = dev_get_priv(dev);

	ofnode port, node;
	int ret;

	sf_vop_power(dev);
	mdelay(50);

	priv->regs_hi = dev_remap_addr_name(dev, "hi");
	if (!priv->regs_hi)
		return -EINVAL;

	priv->regs_low = dev_remap_addr_name(dev, "low");
	if (!priv->regs_low)
		return -EINVAL;

	vout_probe_resources_jh7110(dev);

	/*
	 * Try all the ports until we find one that works. In practice this
	 * tries EDP first if available, then HDMI.
	 *
	 * Note that rockchip_vop_set_clk() always uses NPLL as the source
	 * clock so it is currently not possible to use more than one display
	 * device simultaneously.
	 */
	port = dev_read_subnode(dev, "port");
	if (!ofnode_valid(port)) {
		debug("%s(%s): 'port' subnode not found\n",
		      __func__, dev_read_name(dev));
		return -EINVAL;
	}

	for (node = ofnode_first_subnode(port);
	     ofnode_valid(node);
	     node = dev_read_next_subnode(node)) {
		ret = sf_display_init(dev, plat->base, node);
		if (ret)
			debug("Device failed: ret=%d\n", ret);
		if (!ret)
			break;
	}

	video_set_flush_dcache(dev, 1);

	return 0;
}

static int sf_vop_remove(struct udevice *dev)
{
	return 0;
}

struct rkvop_driverdata rk3288_driverdata = {
	.features = VOP_FEATURE_OUTPUT_10BIT,
};

static const struct udevice_id sf_dc_ids[] = {
	{ .compatible = "starfive,sf-dc8200",
	  .data = (ulong)&rk3288_driverdata },
	{ }
};

static const struct video_ops sf_vop_ops = {
};

int sf_vop_bind(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);

	plat->size = 4 * (CONFIG_VIDEO_STARFIVE_MAX_XRES *
			  CONFIG_VIDEO_STARFIVE_MAX_YRES);
	printf("%s,%d,plat->size = %d\n",__func__,__LINE__,plat->size);

	return 0;
}

U_BOOT_DRIVER(starfive_dc8200) = {
	.name	= "starfive_dc8200",
	.id	= UCLASS_VIDEO,
	.of_match = sf_dc_ids,
	.ops	= &sf_vop_ops,
	.bind	= sf_vop_bind,
	.probe	= sf_vop_probe,
    .remove = sf_vop_remove,
	.priv_auto	= sizeof(struct sf_vop_priv),
};