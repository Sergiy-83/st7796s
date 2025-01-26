/*
 * DRM driver for display panels connected to a Sitronix ST7796S (SKU:MSP4031)
 * display controller in SPI mode.
 *
 * Copyright 2025 Sergey Horoshevskiy <horoshevskiy_83@mail.ru>
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/property.h>
#include <linux/spi/spi.h>
#include <video/mipi_display.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fbdev_generic.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_gem_dma_helper.h>
#include <drm/drm_managed.h>
#include <drm/drm_mipi_dbi.h>

#define ST7796S_MY	BIT(7)
#define ST7796S_MX	BIT(6)
#define ST7796S_MV	BIT(5)
#define ST7796S_BGR	BIT(3)

static void st7796S_pipe_enable(struct drm_simple_display_pipe *pipe, struct drm_crtc_state *crtc_state, struct drm_plane_state *plane_state)
	{
	struct mipi_dbi_dev *dbidev = drm_to_mipi_dbi_dev(pipe->crtc.dev);
	struct mipi_dbi 	*dbi = &dbidev->dbi;
	int ret, idx;
	u8 addr_mode;
		
	printk("st7796S - enable start");
	
	if (!drm_dev_enter(pipe->crtc.dev, &idx))
		{
		printk("st7796S - enable return (!drm_dev_enter)");
		return;
		}

	ret = mipi_dbi_poweron_conditional_reset(dbidev);
	if (ret < 0)
		goto out_exit;
	if (ret == 1)
		goto out_enable;
	
	ret = mipi_dbi_poweron_reset(dbidev);
	if (ret)
		goto out_exit;
	
//	mipi_dbi_command(dbi,0x01); //Software reset
//	msleep(120);

//	mipi_dbi_command(dbi,0x11); //Sleep exit                                            
//	msleep(120);

//	msleep(120);

	mipi_dbi_command(dbi,0x01); //Software reset
	msleep(120);

	mipi_dbi_command(dbi,0x11); //Sleep exit                                            
	msleep(120);

	mipi_dbi_command(dbi,0xF0,0xC3); //Command Set control. Enable extension command 2 partI                                 
	mipi_dbi_command(dbi,0xF0,0x96); //Command Set control. Enable extension command 2 partII
	  
	mipi_dbi_command(dbi,0x36,0x48); //Memory Data Access Control MX, MY, RGB mode   X-Mirror, Top-Left to right-Buttom, RGB                                   
	mipi_dbi_command(dbi,0x3A,0x55); //Interface Pixel Format. Control interface color format set to 16 (RGB565)
	mipi_dbi_command(dbi,0xB4,0x01); //Column inversion.  1-dot inversion
	mipi_dbi_command(dbi,0xB6,0x80,0x02,0x3B); //Display Function Control
	//Bypass
	//Source Output Scan from S1 to S960, Gate Output scan from G1 to G480, scan cycle=2
	//LCD Drive Line=8*(59+1)

	mipi_dbi_command(dbi,0xE8,0x40,0x8A,0x00,0x00,0x29,0x19,0xA5,0x33); //Display Output Ctrl Adjust
	//Source eqaulizing period time= 22.5 us
	//Timing for "Gate start"=25 (Tclk)
	//Timing for "Gate End"=37 (Tclk), Gate driver EQ function ON
  
	mipi_dbi_command(dbi,0xC1,0x06); //Power control 2   	VAP(GVDD)=3.85+( vcom+vcom offset), VAN(GVCL)=-3.85+( vcom+vcom offset)
	mipi_dbi_command(dbi,0xC2,0xA7); //Power control 3 	Source driving current level=low, Gamma driving current level=High
	mipi_dbi_command(dbi,0xC5,0x18); //VCOM Control		VCOM=0.9
	
	//Убираем здесь 4
	msleep(120);
  
	//Gamma Sequence
	mipi_dbi_command(dbi,0xE0,0xF0,0x09,0x0b,0x06,0x04,0x15,0x2F,0x54,0x42,0x3C,0x17,0x14,0x18,0x1B); //Gamma"+"                                             
	mipi_dbi_command(dbi,0xE1,0xE0,0x09,0x0B,0x06,0x04,0x03,0x2B,0x43,0x42,0x3B,0x16,0x14,0x17,0x1B); //Gamma"-"                                             
	msleep(120);
  
	mipi_dbi_command(dbi,0xF0,0x3C); 
	mipi_dbi_command(dbi,0xF0,0x69);
  
out_enable:
	switch (dbidev->rotation) 
		{
		default:
			addr_mode = ST7796S_MX;
			break;
		case 90:
			addr_mode = ST7796S_MV;
			break;
		case 180:
			addr_mode = ST7796S_MY;
			break;
		case 270:
			addr_mode = ST7796S_MV | ST7796S_MY | ST7796S_MX;
			break;
		}
	addr_mode |= ST7796S_BGR;
	mipi_dbi_command(dbi, MIPI_DCS_SET_ADDRESS_MODE, addr_mode); //36h

	mipi_dbi_command(dbi,MIPI_DCS_ENTER_NORMAL_MODE);	//0x13
	mipi_dbi_command(dbi,MIPI_DCS_SET_DISPLAY_ON);		//0x29
	mipi_dbi_command(dbi,MIPI_DCS_WRITE_MEMORY_START);	//0x2C 
	
	mipi_dbi_enable_flush(dbidev, crtc_state, plane_state);
out_exit:
	drm_dev_exit(idx);
	printk("st7796S - enable end");
	return;
	}

static const struct drm_simple_display_pipe_funcs st7796S_pipe_funcs = 
	{
	DRM_MIPI_DBI_SIMPLE_DISPLAY_PIPE_FUNCS(st7796S_pipe_enable),
	};

static const struct drm_display_mode st7796s_mode = 
	{
	DRM_SIMPLE_MODE(320, 480, 37, 49),
	};

DEFINE_DRM_GEM_DMA_FOPS(st7796S_fops);

static const struct drm_driver st7796S_driver = 
	{
	.driver_features = DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC,
	.fops			= &st7796S_fops, DRM_GEM_DMA_DRIVER_OPS_VMAP,
	.debugfs_init	= mipi_dbi_debugfs_init,
	.name			= "st7796S",
	.desc			= "Sitronix ST7796S",
	.date			= "20250126",
	.major			= 1,
	.minor			= 0,
	};

static const struct of_device_id st7796S_of_match[] = 
	{
		{ .compatible = "sitronix,st7796s"},
		{ },
	};
MODULE_DEVICE_TABLE(of, st7796S_of_match);

static const struct spi_device_id st7796S_id[] = 
	{
		{ "st7796s", 0 },
		{ },
	};
MODULE_DEVICE_TABLE(spi, st7796S_id);

static int st7796S_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct mipi_dbi_dev *dbidev;
	struct drm_device *drm;
	struct mipi_dbi *dbi;
	struct gpio_desc *dc;
	u32 rotation = 0;
	int ret;

	printk("st7796S - probe start");

	dbidev = devm_drm_dev_alloc(dev, &st7796S_driver, struct mipi_dbi_dev, drm);
	if (IS_ERR(dbidev))
		return PTR_ERR(dbidev);
	
	dbi = &dbidev->dbi;
	drm = &dbidev->drm;

	dbi->reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(dbi->reset))
		return dev_err_probe(dev, PTR_ERR(dbi->reset), "Failed to get GPIO 'reset'\n");

	dc = devm_gpiod_get(dev, "dc", GPIOD_OUT_LOW);
	if (IS_ERR(dc))
		return dev_err_probe(dev, PTR_ERR(dc), "Failed to get GPIO 'dc'\n");

	dbidev->backlight = devm_of_find_backlight(dev);
	if (IS_ERR(dbidev->backlight))
		return PTR_ERR(dbidev->backlight);

	device_property_read_u32(dev, "rotation", &rotation);

	ret = mipi_dbi_spi_init(spi, dbi, dc);
	if (ret)
		return ret;

	dbi->read_commands = NULL;

	ret = mipi_dbi_dev_init(dbidev, &st7796S_pipe_funcs, &st7796s_mode, rotation);
	if (ret)
		return ret;

	drm_mode_config_reset(drm);

	ret = drm_dev_register(drm, 0);
	if (ret)
		return ret;

	spi_set_drvdata(spi, drm);

	drm_fbdev_generic_setup(drm, 0);
	printk("st7796S - probe end");
	return 0;
}

static void st7796S_remove(struct spi_device *spi)
	{
	struct drm_device *drm = spi_get_drvdata(spi);
	drm_dev_unplug(drm);
	drm_atomic_helper_shutdown(drm);
	}

static void st7796S_shutdown(struct spi_device *spi)
	{
	drm_atomic_helper_shutdown(spi_get_drvdata(spi));
	}

static struct spi_driver st7796S_spi_driver = 
	{
	.driver = {
		.name = "st7796s",
		.of_match_table = st7796S_of_match,
	},
	.id_table 	= st7796S_id,
	.probe 		= st7796S_probe,
	.remove 	= st7796S_remove,
	.shutdown 	= st7796S_shutdown,
	};
module_spi_driver(st7796S_spi_driver);

MODULE_DESCRIPTION("Sitronix ST7796S SPI DRM driver");
MODULE_AUTHOR("Sergey Horoshevskiy <horoshevskiy_83@mail.ru>");
MODULE_LICENSE("GPL");
