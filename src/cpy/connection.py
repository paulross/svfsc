"""
Some characteristics of our imaginary connection.
"""

# Bandwidth in bits/second
import logging
import time
import typing

from src.cpy import server


logger = logging.getLogger(__file__)


BITS_PER_BYTE = 8


class Connection:
    """Simulates a network connection between a client and server."""
    BANDWIDTH_BPS = 50e6
    # Latency in seconds
    LATENCY_S = 0.015
    # The total payloaad divided by the useful payload. For example 10 bits for every byte of data is 1.25
    PAYLOAD_OVERHEAD = 1.25

    LOGGER_PREFIX = 'NETWRK'

    def __init__(self):
        logger.info(f'{self.LOGGER_PREFIX}: __init__()')
        self.server = server.Server()
        self.bytes_client_to_server: typing.List[int] = []
        self.bytes_server_to_client: typing.List[int] = []

    def delay(self, json_bytes: str, to_server: bool) -> None:
        sleep_time = self.LATENCY_S + \
                     self.PAYLOAD_OVERHEAD * len(json_bytes) * BITS_PER_BYTE / self.BANDWIDTH_BPS
        logger.info(f'{self.LOGGER_PREFIX}: delay len={len(json_bytes):12,d} sleep={sleep_time * 1000:.3f} (ms)')
        if to_server:
            self.bytes_client_to_server.append(len(json_bytes))
        else:
            self.bytes_server_to_client.append(len(json_bytes))
        time.sleep(sleep_time)

    # Mirror server.Server() methods with time delay
    def add_file(self, file_id: str, mod_time: float, json_bytes: str) -> str:
        self.delay(json_bytes, to_server=True)
        to_client = self.server.add_file(file_id, mod_time, json_bytes)
        self.delay(to_client, to_server=False)
        return to_client

    def add_data(self, file_id: str, mod_time: float, json_bytes: str) -> str:
        self.delay(json_bytes, to_server=True)
        to_client = self.server.add_data(file_id, mod_time, json_bytes)
        self.delay(to_client, to_server=False)
        return to_client
