import json
import logging
import os
import sys

from src.cpy import client_lib, server
from src.cpy.client_lib import scan_file_json_index

logger = logging.getLogger(__file__)

EXAMPLE_540_KB = '/Users/engun/Documents/workspace/TotalDepth/example_data/RP66V1/data/206_05a-_3_DWL_DWL_WIRE_258276498.DLIS'


def client(file_path: str, the_server: server.Server):
    logger.info(f'CLIENT: Opening {file_path}')
    with open(file_path, 'rb') as file_object:
        logger.info('CLIENT: Scanning file.')
        json_bytes = scan_file_json_index(file_object)
        logger.info('CLIENT: Scanning complete, adding file to server.')
        json_bytes = the_server.add_file(file_path, os.stat(file_path).st_mtime_ns / 1e9, json_bytes)
        logger.info(f'CLIENT: File added to server. Recieved {len(json_bytes)} JSON bytes.')
        while json_bytes:
            logger.info(f'CLIENT: Recieved {len(json_bytes)} JSON bytes.')
            data = []
            for seek, read in json.loads(json_bytes):
                file_object.seek(seek)
                data.append((seek, file_object.read()))
            json_bytes = json.dumps(data)
            logger.info(f'CLIENT: Seek/read done. Sending {len(json_bytes)} JSON bytes.')
            json_bytes = the_server.add_data(file_path, os.stat(file_path).st_mtime_ns / 1e9, json_bytes)


DEFAULT_OPT_LOG_FORMAT_VERBOSE = '%(asctime)s - %(filename)-16s - %(process)5d - (%(threadName)-10s) - %(levelname)-8s - %(message)s'


def main() -> int:
    logging.basicConfig(level=logging.INFO, format=DEFAULT_OPT_LOG_FORMAT_VERBOSE, stream=sys.stdout)
    # Write test plots
    logger.info(f'Writing test plots...')
    a_server = server.Server()
    client(EXAMPLE_540_KB, a_server)
    return 0


if __name__ == '__main__':
    sys.exit(main())


