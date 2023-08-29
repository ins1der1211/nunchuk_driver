// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/input.h>

static char init_cmd_1[] = {0xf0, 0x55};
static char init_cmd_2[] = {0xfb, 0x00};
static char read_cmd[] = {0x00};

struct nunchuk_dev
{
    struct i2c_client *i2c_client;
};

static void nunchuk_poll(struct input_dev *dev)
{
    int res = 0;
    char registers[6] = {};
    int z_pressed = 0;
    int c_pressed = 0;

    struct nunchuk_dev *nunchuk = input_get_drvdata(dev);
    struct i2c_client *client = nunchuk->i2c_client;

    // init device
    res = i2c_master_send(client, init_cmd_1, ARRAY_SIZE(init_cmd_1));
    if (res < 0)
    {
        pr_err("init_cmd_1 failed\n");
        return;
    }
    udelay(1000);
    res = i2c_master_send(client, init_cmd_2, ARRAY_SIZE(init_cmd_2));
    if (res < 0)
    {
        pr_err("init_cmd_2 failed\n");
        return;
    }

    // read registers
    usleep_range(10000, 20000);
    res = i2c_master_send(client, read_cmd, 1);
    if (res < 0)
    {
        pr_err("read registers failed with code %d\n", res);
        return;
    }
    usleep_range(10000, 20000);
    res = i2c_master_recv(client, registers, ARRAY_SIZE(registers));
    if (res < 0)
    {
        pr_err("read registers failed with code %d\n", res);
        return;
    }

    z_pressed = registers[5] & 0x1;
    if (z_pressed == 0)
    {
        z_pressed = 1;
    }
    else
    {
        z_pressed = 0;
    }
    c_pressed = registers[5] >> 1 & 0x1;
    if (c_pressed == 0)
    {
        c_pressed = 1;
    }
    else
    {
        c_pressed = 0;
    }

    input_report_key(dev, BTN_Z, z_pressed);
    input_report_key(dev, BTN_C, c_pressed);
    input_sync(dev);
}

static int nunchuk_i2c_probe(struct i2c_client *client,
                             const struct i2c_device_id *id)
{
    int res = 0;
    struct input_dev *input;
    struct nunchuk_dev *nunchuk;

    input = devm_input_allocate_device(&client->dev);
    if (!input)
        return -ENOMEM;
    input->name = "Wii Nunchuk";
    input->id.bustype = BUS_I2C;
    set_bit(EV_KEY, input->evbit);
    set_bit(BTN_C, input->keybit);
    set_bit(BTN_Z, input->keybit);

    nunchuk = devm_kzalloc(&client->dev, sizeof(*nunchuk), GFP_KERNEL);
    if (!nunchuk)
        return -ENOMEM;
    nunchuk->i2c_client = client;
    input_set_drvdata(input, nunchuk);

    res = input_setup_polling(input, nunchuk_poll);
    if (res)
    {
        pr_err("input_setup_polling failed\n");
        return res;
    }

    res = input_register_device(input);
    if (res != 0)
    {
        pr_err("Device register failure\n");
        return res;
    }

    return 0;
}

static int nunchuk_i2c_remove(struct i2c_client *client)
{
    pr_info("nunchuk_i2c_remove get called\n");
    return 0;
}

static const struct i2c_device_id nunchuk_id[] = {
    {"nunchuk", 0},
    {}};

MODULE_DEVICE_TABLE(i2c, nunchuk_id);

static const struct of_device_id nunchuk_of_match[] = {
    {.compatible = "nintendo,nunchuk"},
    {}};

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