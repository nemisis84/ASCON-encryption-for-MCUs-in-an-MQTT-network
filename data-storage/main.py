import os
import pyascon.ascon as ascon
import paho.mqtt.client as mqtt


class SecureMQTTClient:

    def __init__(self, broker, topic, port=1883):
        """Initialize the MQTT client and Ascon encryption parameters."""
        self.broker = broker
        self.port = port
        self.topic = topic

        # Ascon encryption parameters
        self.key = bytes.fromhex("000102030405060708090A0B0C0D0E0F")
        self.devices = {"TEMP-1": bytes.fromhex("000102030405060708090A0B0C0D0E0F")}
        self.nonce_size = 16

        # Initialize MQTT client
        self.client = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            protocol=mqtt.MQTTProtocolVersion.MQTTv5)
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message

        print(f"MQTT Protocol Version: {self.client._protocol}")

    def _on_connect(self, client, userdata, flags, reason_code, properties):
        """Callback when the client connects to the broker."""
        print(
            f"Connected to MQTT Broker {self.broker} with result code {reason_code}"
        )
        client.subscribe(self.topic)

    def _on_message(self, client, userdata, msg):
        """Callback when a message is received."""
        try:
            payload = msg.payload
            print(f"\nEncrypted message received: {payload}")
            ciphertext, nonce, associated_data = self._parse_encrypted_message(
                payload)
            decrypted_msg = self._decrypt_message(ciphertext, nonce,
                                                    associated_data)
            print(f"Decrypted message: {decrypted_msg}")
        except Exception as e:
            print("❌ Error decrypting message:")
            print(e)

    def _encrypt_message(self,
                         message: str,
                         associated_data: str = "BLE-Temp"):
        """Encrypts a message using Ascon."""
        nonce = os.urandom(self.nonce_size)
        ciphertext = ascon.ascon_encrypt(self.key,
                                         nonce,
                                         associated_data.encode(),
                                         message.encode(),
                                         variant="Ascon-128a")

        return ciphertext, nonce

    def _parse_encrypted_message(self, payload: bytes):
        """Parses a raw byte-encoded encrypted message."""

        if len(payload) < self.nonce_size:
            print("Error: Payload too short to contain a valid nonce.")
            return None, None, None

        ad_start_index = payload.rfind(b"|TEMP-")  # Locate the start of AD

        if ad_start_index == -1:
            print("❌ Error: Associated Data not found in payload.")
            return None, None, None
        
        associated_data = payload[ad_start_index:]  # AD starts from this index
        nonce = payload[ad_start_index-self.nonce_size:ad_start_index]
        ciphertext = payload[:ad_start_index-self.nonce_size]
        return ciphertext, nonce, associated_data

    def _decrypt_message(self, ciphertext: bytes, nonce: bytes,
                         associated_data: bytes):
        """Decrypts a received message using Ascon."""
        associated_str = associated_data.decode()[1:]
        key = self.devices[associated_str]
        print(f"Nonce: {len(nonce)}, ciphertext: {len(ciphertext)}")
        plaintext = ascon.ascon_decrypt(key,
                                        nonce,
                                        associated_data,
                                        ciphertext,
                                        variant="Ascon-128a")
        decoded_value = int.from_bytes(plaintext, byteorder='little')
        return decoded_value

    def connect(self):
        """Connect to the MQTT broker."""
        self.client.connect(self.broker, self.port, 60)


    def publish(self, message: str, topic=None):
        """Encrypt and publish a message to the MQTT topic."""
        if not topic:
            topic = self.topic

        ciphertext, nonce = self._encrypt_message(message)
        encrypted_payload = ciphertext + nonce
        self.client.publish(topic, encrypted_payload)

    def listen(self):
        """Keep listening for incoming messages."""
        print("Listening for messages...")
        self.client.loop_forever()


if __name__ == "__main__":
    broker = "mqtt20.iik.ntnu.no"
    topic = "/ascon-e2e/data-storage"
    client = SecureMQTTClient(broker, topic)
    # Connect to the broker
    client.connect()
    # Publish an encrypted message
    client.publish("Hei!", topic="/ascon-e2e/PICO")
    # Start listening for encrypted messages
    client.listen()
