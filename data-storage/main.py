import os
import pyascon.ascon as ascon
import paho.mqtt.client as mqtt
import time
from datetime import datetime
import pandas as pd
import struct
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
import sys


class SecureMQTTClient:

    def __init__(self,
                 broker,
                 topic,
                 port=1883,
                 scenario=1,
                 send_back=True,
                 crypto_algorithm_tag="ASCON"):
        """Initialize the MQTT client and Ascon encryption parameters."""
        self.broker = broker
        self.port = port
        self.topic = topic

        # Crypto encryption parameters

        self.crypto_algorithm = "ASCON" if crypto_algorithm_tag.endswith("ASCON") else crypto_algorithm_tag
        self.key = bytes.fromhex("000102030405060708090A0B0C0D0E0F")
        self.devices = {
            "TEMP-1": bytes.fromhex("000102030405060708090A0B0C0D0E0F")
        }
        self.nonce_size = 16 if self.crypto_algorithm == "ASCON" else 12 if self.crypto_algorithm == "AES-GCM" else 0

        # Initialize MQTT client
        self.client = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            protocol=mqtt.MQTTProtocolVersion.MQTTv5)
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.send_back = send_back

        # Store data
        self.receive_data_mode = False
        self.receiving_data_type = ""
        self.received_bytes = b""
        self.encryption_log = pd.DataFrame(columns=["Start_Time", "End_Time"])
        self.decryption_log = pd.DataFrame(columns=["Start_Time", "End_Time"])
        self.stored = False  # Keeps track of if the datastorage data has been stored

        base_dir = os.path.join(
            "results", crypto_algorithm_tag + "_scen" + scenario)
        self.results_dir = base_dir
        counter = 1
        while os.path.exists(self.results_dir):
            self.results_dir = f"{base_dir}_{counter}"
            counter += 1
        os.makedirs(self.results_dir, exist_ok=True)

        print(f"MQTT Protocol Version: {self.client._protocol}")

    def _on_connect(self, client, userdata, flags, reason_code, properties):
        """Callback when the client connects to the broker."""
        print(
            f"Connected to MQTT Broker {self.broker} with result code {reason_code}"
        )
        client.subscribe(self.topic)

    def _on_message(self, client, userdata, msg):
        """Callback when a message is received."""
        if not self.receive_data_mode:
            try:
                payload = msg.payload
                print(f"\nMessage received:{payload.hex()}")
                if self._check_if_data_is_incoming(payload):
                    return
                if self.crypto_algorithm == "NONE" and self.send_back:
                    print("No encryption, sending back the message")
                    self.publish(payload, "/ascon-e2e/PICO")
                    return
                else:
                    ciphertext, nonce, associated_data = self._parse_encrypted_message(
                        payload)
                    decrypted_msg = self._decrypt_message(
                        ciphertext, nonce, associated_data)
                    print(f"Decrypted message: {decrypted_msg}")

                if self.send_back:

                    byte_length = (decrypted_msg.bit_length() +
                                   7) // 8  # Calculate required byte size
                    encoded_bytes = decrypted_msg.to_bytes(byte_length,
                                                           byteorder='little')
                    encrypted_message, nonce = self._encrypt_message(
                        encoded_bytes, associated_data)

                    message = encrypted_message + nonce + associated_data

                    self.publish(message, "/ascon-e2e/PICO")

                    print(f"Encrypted message sent back: {message}")

            except Exception as e:
                print("❌ Error decrypting message:")
                print(e)
        else:
            try:
                payload = msg.payload
                print(f"Recived data: {payload.hex()}")
                sequence_number = payload[0]
                # The rest of the bytes are the payload
                self.received_bytes += payload[1:]

                if sequence_number == 0:
                    self._export_data()

            except Exception as e:
                print(e)

    def _export_data(self):
        """
        Parses self.received_bytes (binary data) into structured RTT entries
        and exports the results as a CSV file inside `/results`, with a timestamped filename.
        """

        if not self.received_bytes:
            print("❌ No data to export.")
            return

        # Define the RTT_Entry struct format (uint16_t, uint64_t, uint64_t)
        RTT_ENTRY_FORMAT = "HQQ"  # H = uint16_t (2 bytes), Q = uint64_t (8 bytes), Q = uint64_t (8 bytes)
        ENTRY_SIZE = struct.calcsize(
            RTT_ENTRY_FORMAT)  # Total struct size (18 bytes per entry)

        # Parse the binary data
        rtt_entries = []
        for i in range(0, len(self.received_bytes), ENTRY_SIZE):
            if i + ENTRY_SIZE > len(self.received_bytes):
                print(f"⚠️ Incomplete RTT Entry at index {i}, skipping.")
                break  # Skip incomplete entries

            entry = struct.unpack(RTT_ENTRY_FORMAT,
                                  self.received_bytes[i:i + ENTRY_SIZE])
            rtt_entries.append(entry)

        # Convert to Pandas DataFrame
        df = pd.DataFrame(rtt_entries,
                          columns=["Seq_Num", "Start_Time", "End_Time"])

        if not self.stored:
            self.encryption_log.to_csv(self.results_dir + "/DS_ENC.csv",
                                       index=False)
            self.decryption_log.to_csv(self.results_dir + "/DS_DEC.csv",
                                       index=False)
            self.stored = True

        # Generate a timestamped filename

        file_path = os.path.join(self.results_dir,
                                 f"{self.receiving_data_type}.csv")

        # Save DataFrame to CSV
        df.to_csv(file_path, index=False)

        print(f"✅ Data successfully exported to {file_path}")
        self.received_bytes = b""  # Reset the received bytes
        self.receive_data_mode = False

        if self.receiving_data_type == "DEC":
            sys.exit(0)  # Exit if the data type is DEC


    def _encrypt_message(
            self,
            message: bytes,
            associated_data: bytes = b"BLE-Temp") -> tuple[bytes, bytes]:
        """Encrypts a message using Ascon or AES-GCM-128."""
        nonce = os.urandom(self.nonce_size)
        start_time = time.perf_counter_ns()  # ⏱️ Start time

        if self.crypto_algorithm == "ASCON":
            ciphertext = ascon.ascon_encrypt(self.key,
                                             nonce,
                                             associated_data,
                                             message,
                                             variant="Ascon-128a")
        elif self.crypto_algorithm == "AES-GCM":
            aesgcm = AESGCM(self.key)
            ciphertext = aesgcm.encrypt(nonce, message, associated_data)
        else:
            raise ValueError("Unsupported crypto_algorithm")

        end_time = time.perf_counter_ns()
        self.encryption_log.loc[len(self.encryption_log)] = {
            "Start_Time": start_time,
            "End_Time": end_time,
        }

        return ciphertext, nonce

    def _check_if_data_is_incoming(self, payload):
        # Convert payload to string for easier processing
        payload_str = payload.decode("utf-8", errors="ignore")

        # Define the allowed data types
        data_types = {"RTT", "ENC", "DEC"}

        if "|" in payload_str:
            main_data, suffix = payload_str.rsplit("|", 1)

            if suffix in data_types:
                print(
                    f"Entering Receive Data Mode: Detected Data Type '{suffix}'"
                )
                self.receive_data_mode = True
                self.receiving_data_type = suffix
                return True

            return False

        print("Invalid Payload Format")
        return False  # Invalid payload case

    def _parse_encrypted_message(self, payload: bytes):
        """Parses a raw byte-encoded encrypted message."""

        if self._check_if_data_is_incoming(payload):
            return None, None, None

        if len(payload) < self.nonce_size:
            print("Error: Payload too short to contain a valid nonce.")
            return None, None, None

        ad_start_index = payload.rfind(b"|TEMP-")  # Locate the start of AD

        if ad_start_index == -1:
            print("❌ Error: Associated Data not found in payload.")
            return None, None, None

        associated_data = payload[ad_start_index:]  # AD starts from this index
        nonce = payload[ad_start_index - self.nonce_size:ad_start_index]
        ciphertext = payload[:ad_start_index - self.nonce_size]
        return ciphertext, nonce, associated_data

    def _decrypt_message(self, ciphertext: bytes, nonce: bytes,
                         associated_data: bytes):
        associated_str = associated_data.decode()[1:]
        device_id = associated_str.split("|")[0]
        key = self.devices[device_id]
        start_time = time.perf_counter_ns()

        if self.crypto_algorithm == "ASCON":
            plaintext = ascon.ascon_decrypt(key,
                                            nonce,
                                            associated_data,
                                            ciphertext,
                                            variant="Ascon-128a")
        elif self.crypto_algorithm == "AES-GCM":
            aesgcm = AESGCM(key)
            plaintext = aesgcm.decrypt(nonce, ciphertext, associated_data)
        else:
            raise ValueError("Unsupported crypto_algorithm")

        end_time = time.perf_counter_ns()
        self.decryption_log.loc[len(self.decryption_log)] = {
            "Start_Time": start_time,
            "End_Time": end_time,
        }

        decoded_value = int.from_bytes(plaintext, byteorder='little')
        return decoded_value

    def connect(self):
        """Connect to the MQTT broker."""
        self.client.connect(self.broker, self.port, 60)

    def publish(self, message, topic=None):
        """Encrypt and publish a message to the MQTT topic."""
        if not topic:
            topic = self.topic
        self.client.publish(topic, message)

    def listen(self):
        """Keep listening for incoming messages."""
        print("Listening for messages...")
        self.client.loop_forever()


if __name__ == "__main__":
    if len(sys.argv) < 3 or not sys.argv[1].isdigit():
        print("Usage: python main.py <scenario_number> <crypto_algorithm>")
        sys.exit(1)
    if len(sys.argv[1]) > 1:
        print("Scenario number should be a single digit.")
        sys.exit(1)
    if sys.argv[2] not in ["ASCON", "masked_ASCON", "AES-GCM", "NONE"]:
        print("Usage: python main.py <scenario_number> <crypto_algorithm>")
        sys.exit(1)
    scenario = sys.argv[1]
    crypto_algorithm_tag = sys.argv[2]
    broker = "mqtt20.iik.ntnu.no"
    topic = "/ascon-e2e/data-storage"
    client = SecureMQTTClient(broker,
                              topic,
                              scenario=scenario,
                              crypto_algorithm_tag=crypto_algorithm_tag)
    # Connect to the broker
    client.connect()
    # Start listening for encrypted messages
    client.listen()
