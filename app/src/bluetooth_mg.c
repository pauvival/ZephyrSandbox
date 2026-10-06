#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/addr.h>
#include "bluetooth_mg.h"

//BLE

//Obtain BLE device name from prj.conf
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

//Opaque type representing a connection to a remote device
struct bt_conn *my_conn = NULL;

//A structure used to submit work
//All the members are internal and should not be accessed directly
static struct k_work adv_work;


//----------------------------------------------------------------------------------
//GATT LAYER INFO
//Base UUID                     a743XXXX-0941-4322-8610-858882512683
//Custom LED service            a7431102-0941-4322-8610-858882512683
//LED characteristic            a7431103-0941-4322-8610-858882512683


// Define the Service UUID
#define BT_UUID_LED_SERVICE_VAL     BT_UUID_128_ENCODE(0xa7431102, 0x0941, 0x4322, 0x8610, 0x858882512683)
#define BT_UUID_LED_SERVICE         BT_UUID_DECLARE_128(BT_UUID_LED_SERVICE_VAL)

// Define the Characteristic UUID
#define BT_UUID_LED_CHAR_VAL    BT_UUID_128_ENCODE(0xa7431103, 0x0941, 0x4322, 0x8610, 0x858882512683)
#define BT_UUID_LED_CHAR        BT_UUID_DECLARE_128(BT_UUID_LED_CHAR_VAL)


uint16_t gatt_led_value;

static ssize_t write_led (struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    printk("Attribute write, handle: %u, conn: %p\n", attr->handle, (void *)conn);

	if (len != sizeof(uint16_t)) {
		printk("Write led: Incorrect data length\n");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (offset != 0) {
		printk("Write led: Incorrect data offset\n");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	//Read the received value
    //Assumes central device send data in little-endian format (the standard way)
	uint16_t val = *((uint16_t *)buf);
    gatt_led_value = val;
    printk("Received value %d", val);

	return len;
}


//Create the custom LED service 
BT_GATT_SERVICE_DEFINE(my_led_svc, 
    BT_GATT_PRIMARY_SERVICE(BT_UUID_LED_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_LED_CHAR, BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_WRITE_ENCRYPT, NULL, write_led, &gatt_led_value),


);

//--------------------------------------------------------------------------------------
//GAP LAYER
//Struct for advertising packet data
static const struct bt_data ad[] = {
	//Set the advertising flags (no normal Bluetooth supported, connection supported)
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),

    //Device service UUID (so that python script finds it )
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LED_SERVICE_VAL),

};

//Struct for scan response packet data (most probably the scan request will be dropped in the future)
static const struct bt_data sd[] = {
	//Set the device name 
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

//GAP layer events callabacks


//Callback called when a central connects
void connected_cb(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("Connection error %d\n", err);
        return;
    }
    printk("Connected");
    my_conn = bt_conn_ref(conn);

    err = bt_conn_set_security(conn, BT_SECURITY_L2); //request just works security
    if (err) {
        printk("Failed to set security (err %d)\n", err);
    }

	struct bt_conn_info info;
	err = bt_conn_get_info(conn, &info);
	if (err) {
		printk("bt_conn_get_info() returned %d\n", err);
		return;
	}

	double connection_interval = BT_GAP_US_TO_CONN_INTERVAL(info.le.interval_us) *1.25; // in ms
	uint16_t supervision_timeout = info.le.timeout*10; // in ms
	printk("Connection parameters: interval %.2f ms, latency %d intervals, timeout %d ms\n", connection_interval, info.le.latency, supervision_timeout);

}

void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected. Reason %d\n", reason);
    bt_conn_unref(my_conn);
}

//Handler to resume advertising after a disconnection
static void adv_work_handler(struct k_work *work)
{
	//A custom advertising interval with a parameter could be used instead of BT_LE_ADV_CONN_FAST_1
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

//Submit resume advertising work structure to system work queue 
static void advertising_start(void)
{
	k_work_submit(&adv_work);
}

//Callback for when the device is prepared for a new connection
static void recycled_cb(void)
{
	printk("Connection object available from previous conn. Disconnect is complete!\n");
	advertising_start();
}

//Updated connection parameters callback
void le_param_updated_cb(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
    double connection_interval = interval*1.25;         // in ms
    uint16_t supervision_timeout = timeout*10;          // in ms
    printk("Connection parameters updated: interval %.2f ms, latency %d intervals, timeout %d ms\n", connection_interval, latency, supervision_timeout);
}

//Updated PHY parameters callback
void le_phy_updated_cb(struct bt_conn *conn, struct bt_conn_le_phy_info *param)
{
    // PHY Updated
    if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_1M) {
        printk("PHY updated. New PHY: 1M\n");
    }
    else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_2M) {
        printk("PHY updated. New PHY: 2M\n");
    }
    else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_CODED_S8) {
        printk("PHY updated. New PHY: Long Range\n");
    }
}

//Updated MTU configuration callback
void le_data_len_updated_cb(struct bt_conn *conn, struct bt_conn_le_data_len_info *info)
{
    uint16_t tx_len     = info->tx_max_len; 
    uint16_t tx_time    = info->tx_max_time;
    uint16_t rx_len     = info->rx_max_len;
    uint16_t rx_time    = info->rx_max_time;
    printk("Data length updated. Length %d/%d bytes, time %d/%d us\n", tx_len, rx_len, tx_time, rx_time);
}


static void security_changed_cb(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d\n", addr, level,
			err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected 	= connected_cb,
	.disconnected = disconnected_cb,
	.recycled 	= recycled_cb,
    .le_param_updated   = le_param_updated_cb,
    //.le_phy_updated     = le_phy_updated_cb, //requires prj.conf enables, we will not change it for the time being
    //.le_data_len_updated    = le_data_len_updated_cb, //requires prj.conf enables, we will not change it for the time being
    .security_changed = security_changed_cb,
};

//----------------------------------------------------------------------------------

void init_BLE(void)
{
    int err;

    //Create a random static address
    bt_addr_le_t addr;
    err = bt_addr_le_from_str("FF:EE:DD:CC:BB:AA", "random", &addr);
    if (err) {
        printk("Invalid BT address (err %d)\n", err);
    }

    err = bt_id_create(&addr, NULL);
    if (err < 0) {
        printk("Creating new ID failed (err %d)\n", err);
    }

	//Enable BLE
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	printk("BLE initialized\n");

	//Start BLE advertising
	//Now it is used through the system workqueue, that way the callback for recycled is reused

    //Associates the handler adv_work_handler with the work structure adv_work 
    //This is only done once unless the associated work handler is to be changed
    //This is done before submiting the work structure first time 
    //This is done at run time. Could be done at compile time with K_WORK_DEFINE. We shall see what fits the project better.
	k_work_init(&adv_work, adv_work_handler);
	advertising_start();

}