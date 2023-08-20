// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>

static int nunchuk_read_registers(struct i2c_client *client, char *buf, int buf_size) {
    int res = 0;
    char read_cmd[] = { 0x00 };
    
    usleep_range(10000, 20000);
    res = i2c_master_send(client, read_cmd, 1);
    if (res < 0) {
        pr_err("nunchuk_read_registers failed with code %d\n", res);
        return -1;
    }
    usleep_range(10000, 20000);
    res = i2c_master_recv(client, buf, buf_size);
    if (res < 0) {
        pr_err("nunchuk_read_registers failed with code %d\n", res);
        return -1;
    }
    return 0;
}

static int nunchuk_i2c_probe(struct i2c_client *client,
                             const struct i2c_device_id *id)
{
    char init_cmd_1[] = { 0xf0, 0x55 };
    char init_cmd_2[] = { 0xfb, 0x00 };
    char registers[6] = {};
    int z_pressed = 0;
    int c_pressed = 0;
    int res = 0;

    pr_info("nunchuk_i2c_probe get called\n");

    res = i2c_master_send(client, init_cmd_1, ARRAY_SIZE(init_cmd_1));
    if (res < 0) {
        pr_err("init_cmd_1 failed\n");
        return res;
    }
    udelay(1000);
    res = i2c_master_send(client, init_cmd_2, ARRAY_SIZE(init_cmd_2));
    if (res < 0) {
        pr_err("init_cmd_2 failed\n");
        return res;
    }
    nunchuk_read_registers(client, registers, ARRAY_SIZE(registers));
    res = nunchuk_read_registers(client, registers, ARRAY_SIZE(registers));
    if (res < 0) {
        pr_err("Failed to read registers\n");
        return -1;
    }
    z_pressed = registers[5] & 0x1;
    if (z_pressed == 0) {
        z_pressed = 1;
    } else {
        z_pressed = 0;
    }
    c_pressed = registers[5] >> 1 & 0x1;
    if (c_pressed == 0) {
        c_pressed = 1;
    } else {
        c_pressed = 0;
    }

    pr_info("z pressed -> %d, c pressed -> %d\n", z_pressed, c_pressed);

    return 0;
}

static int nunchuk_i2c_remove(struct i2c_client *client)
{
    pr_info("nunchuk_i2c_remove get called\n");
    return 0;
}

static const struct i2c_device_id nunchuk_id[] = {
    {"nunchuk", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, nunchuk_id);

static const struct of_device_id nunchuk_of_match[] = {
    {.compatible = "nintendo,nunchuk"},
    {}
};

MODULE_DEVICE_TABLE(of, nunchuk_of_match);

static struct i2c_driver nunchuk_i2c_driver = {
    .driver = {
        .name = "nunchuk_i2c",
        .of_match_table = nunchuk_of_match,
    },
    .probe = nunchuk_i2c_probe,
    .remove = nunchuk_i2c_remove,
    .id_table = nunchuk_id,
};

module_i2c_driver(nunchuk_i2c_driver);

MODULE_AUTHOR("Nikita Yurchenko <insider.skiy@gmail.com>");
MODULE_DESCRIPTION("Wii Nunchuk controller I2C driver");
MODULE_LICENSE("GPL");