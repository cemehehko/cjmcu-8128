#include "BMP280.h"
#include "CCS811.h"
#include "SI7021.h"

#include <cmath>
#include <iomanip>
#include <MQTTClient.h>
#include <jsoncpp/json//json.h>

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "ExampleClientPub"
#define TOPIC       "cjmcu"
#define QOS         1
#define TIMEOUT     10000L

int main() {
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    CCS811 ccs811("/dev/i2c-1", 0x5a);
    SI7021 si7021("/dev/i2c-1", 0x40);
    BMP280 bmp280("/dev/i2c-1", 0x76);

    while (true) {
        bmp280.measure();

        float t_si7021 = si7021.measure_temperature();
        double t_bmp20 = bmp280.get_temperature();
        float relative_humidity = si7021.measure_humidity();

        ccs811.set_env_data(relative_humidity, (t_si7021 + t_bmp20) / 2);
        ccs811.read_sensors();

        float mm = bmp280.get_pressure()*0.750063755419211;

        Json::Value json;
        json["T1"] = round(t_si7021*10)/10;
        json["T2"] = round(t_bmp20*10)/10;
        json["H"] = round(relative_humidity*10)/10;
        json["CO2"] = ccs811.get_co2();
        json["TVOC"] = ccs811.get_tvoc();
        json["P"] = round(mm*10)/10;

        std::ostringstream stream;
        Json::StreamWriterBuilder builder;
        builder.settings_["indentation"] = "";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        writer->write(json, &stream);

        std::string str = stream.str();
        pubmsg.payload = (void *)str.c_str();
        pubmsg.payloadlen = (int)str.size();

        MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
        rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
        printf("Message %d delivered\n", token);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
}
