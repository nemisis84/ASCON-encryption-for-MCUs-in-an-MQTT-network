idf_component_register(SRCS "main.c" "wifi_enterprise.c" "gatt_client.c" "mqtt5c.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES ca.pem client.crt client.key)
