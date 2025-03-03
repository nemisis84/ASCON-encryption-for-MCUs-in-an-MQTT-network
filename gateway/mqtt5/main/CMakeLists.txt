idf_component_register(SRCS "main.c" "wifi_enterprise.c" "gattc_demo.c" "mqtt5c.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES ca.pem client.crt client.key)
