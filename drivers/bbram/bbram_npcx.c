/*
 * Copyright (c) 2021 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_bbram

#include <drivers/bbram.h>
#include <errono.h>
#include <sys/util.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(bbram, CONFIG_BBRAM_LOG_LEVEL);

/** Device config */
struct bbram_npcx_config {
	/** BBRAM base address */
	uintptr_t base_addr;
	/** BBRAM size (Unit:bytes) */
	int size;
	/** Status register base address */
	uintptr_t status_reg_addr;
};

#define NPCX_STATUS_IBBR BIT(7)
#define NPCX_STATUS_VSBY BIT(1)
#define NPCX_STATUS_VCC1 BIT(0)

#define DRV_CONFIG(dev) ((const struct bbram_npcx_config *)(dev)->config)
#define DRV_STATUS(dev)                                                        \
	(*((volatile uint8_t *)DRV_CONFIG(dev)->status_reg_addr))

static int get_bit_and_reset(const struct device *dev, int mask)
{
	int result = DRV_STATUS(dev) & mask;

	/* Clear the bit(s) */
	DRV_STATUS(dev) = mask;

	return result;
}

static int bbram_npcx_check_invalid(const struct device *dev)
{
	return get_bit_and_reset(dev, NPCX_STATUS_IBBR);
}

static int bbram_npcx_check_standby_power(const struct device *dev)
{
	return get_bit_and_reset(dev, NPCX_STATUS_VSBY);
}

static int bbram_npcx_check_power(const struct device *dev)
{
	return get_bit_and_reset(dev, NPCX_STATUS_VCC1);
}

static int bbram_npcx_get_size(const struct device *dev, size_t *size)
{
	const struct bbram_npcx_config *config = DRV_CONFIG(dev);

	*size = config->size;
	return 0;
}

static int bbram_npcx_read(const struct device *dev, size_t offset, size_t size,
			   uint8_t *data)
{
	const struct bbram_npcx_config *config = DRV_CONFIG(dev);

	if (size < 1 || offset + size > config->size || bbram_npcx_check_invalid(dev)) {
		return -EFAULT;
	}


	bytecpy(data, ((volatile uint8_t *)config->base_addr + offset), size);
	return 0;
}

static int bbram_npcx_write(const struct device *dev, size_t offset, size_t size,
			    const uint8_t *data)
{
	const struct bbram_npcx_config *config = DRV_CONFIG(dev);

	if (size < 1 || offset + size > config->size || bbram_npcx_check_invalid(dev)) {
		return -EFAULT;
	}

	bytecpy(((volatile uint8_t *)config->base_addr + offset), data, size);
	return 0;
}

static const struct bbram_driver_api bbram_npcx_driver_api = {
	.check_invalid = bbram_npcx_check_invalid,
	.check_standby_power = bbram_npcx_check_standby_power,
	.check_power = bbram_npcx_check_power,
	.get_size = bbram_npcx_get_size,
	.read = bbram_npcx_read,
	.write = bbram_npcx_write,
};

static int bbram_npcx_init(const struct device *dev)
{
	return 0;
}

#define BBRAM_INIT(inst)                                                                           \
	static struct {                                                                            \
	} bbram_data_##inst;                                                                       \
	static const struct bbram_npcx_config bbram_cfg_##inst = {                                 \
		.base_addr = DT_INST_REG_ADDR_BY_NAME(inst, memory),                               \
		.size = DT_INST_REG_SIZE_BY_NAME(inst, memory),                                    \
		.status_reg_addr = DT_INST_REG_ADDR_BY_NAME(inst, status),                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, bbram_npcx_init, NULL, &bbram_data_##inst, &bbram_cfg_##inst,  \
			      PRE_KERNEL_1, CONFIG_BBRAM_INIT_PRIORITY, &bbram_npcx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BBRAM_INIT);
