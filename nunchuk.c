// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/input.h>

struct nunchuk_dev
{
    struct i2c_client *i2c_client;
};

static s32 nunchuk_read_registers(struct i2c_client *client, u8 *buf, int buf_size)
{
    s32 status;
    mdelay(10);
    buf[0] = 0x00;
    status = i2c_master_send(client, buf, 1);
    if (status < 0)
    {
        return status;
    }
    mdelay(10);
    return i2c_master_recv(client, buf, buf_size);
}

static void nunchuk_poll(struct input_dev *dev)
{
    u8 buf[6];
    u8 joystick_x;
    u8 joystick_y;
    u8 c_pressed;
    u8 z_pressed;
    struct nunchuk_dev *nunchuk = input_get_drvdata(dev);
    struct i2c_client *client = nunchuk->i2c_client;

    s32 status;

    status = nunchuk_read_registers(client, buf, ARRAY_SIZE(buf));
    if (status < 0) {
        dev_err(&client->dev, "Can't read device registers -> %d\n", status);
        return;
    }
    joystick_x = buf[0];
    joystick_y = buf[1];
    c_pressed = !(buf[5] & 0x2);
    z_pressed = !(buf[5] & 0x1);

    input_report_abs(dev, ABS_X, joystick_x);
    input_report_abs(dev, ABS_Y, joystick_y);
    input_report_key(dev, BTN_C, c_pressed);
    input_report_key(dev, BTN_Z, z_pressed);

    input_sync(dev);
    
}

static int nunchuk_i2c_probe(struct i2c_client *client,
                             const struct i2c_device_id *id)
{
    u8 buf[2];
    s32 status;
    struct nunchuk_dev *nunchuk;

    struct input_dev *input = devm_input_allocate_device(&client->dev);
    if (!input)
    {
        pr_err("Can't allocate device, enomem\n");
        return -ENOMEM;
    }

    nunchuk = devm_kzalloc(&client->dev, sizeof(*nunchuk), GFP_KERNEL);
    if (!nunchuk)
    {
        return -ENOMEM;
    }
    nunchuk->i2c_client = client;
    input_set_drvdata(input, nunchuk);

    input->name = "Wii Nunchuk";
    input->id.bustype = BUS_I2C;

    set_bit(EV_KEY, input->evbit);
    set_bit(BTN_C, input->keybit);
    set_bit(BTN_Z, input->keybit);

    set_bit(EV_ABS, input->evbit);
    set_bit(ABS_X, input->absbit);
    set_bit(ABS_Y, input->absbit);
    input_set_abs_params(input, ABS_X, 30, 220, 4, 8);
    input_set_abs_params(input, ABS_Y, 40, 200, 4, 8);

    input_setup_polling(input, nunchuk_poll);
    input_set_poll_interval(input, 50);


    /* Nunchuk handshake */
    buf[0] = 0xf0;
    buf[1] = 0x55;
    status = i2c_master_send(client, buf, 2);
    if (status < 0)
    {
        dev_err(&client->dev, "nunchuk handshake failed #1 - %d\n", status);
        return status;
    }
    udelay(1);
    buf[0] = 0xfb;
    buf[1] = 0x00;
    status = i2c_master_send(client, buf, 2);
    if (status < 0)
    {
        dev_err(&client->dev, "nunchuk handshake failed #2 - %d\n", status);
        return status;
    }

    /* Register device */
    status = input_register_device(input);
    if (status < 0)
    {
        dev_err(&client->dev, "Can't register device - %d\n", status);
        return status;
    }

    return 0;
}

static int nunchuk_i2c_remove(struct i2c_client *client)
{
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