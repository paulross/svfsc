import json
import logging
import os
import sys
import typing

from src.cpy import client_lib, connection, common
from src.cpy.client_lib import scan_file_json_index

logger = logging.getLogger(__file__)


BASIC_FILE = '/Users/engun/Documents/workspace/TotalDepth/example_data/RP66V1/data/BASIC_FILE.dlis'
EXAMPLE_540_KB = '/Users/engun/Documents/workspace/TotalDepth/example_data/RP66V1/data/206_05a-_3_DWL_DWL_WIRE_258276498.DLIS'
EXAMPLE_24_MB = '/Users/engun/Documents/workspace/TotalDepth/data/by_type/RP66V1/WAPIMS/2010-2015/W003353/S1R2/UNGANI-2_S1R2_XRMI&WSTT_DLIS_MAIN.dlis'
EXAMPLE_120_MB = '/Users/engun/Documents/workspace/TotalDepth/data/by_type/RP66V1/WAPIMS/2010-2015/W004434/Wireline/CVX_F24B_S1R1_IBC-CBL_Repeat_031PUC.DLIS'
EXAMPLE_256_MB = '/Users/engun/Documents/workspace/TotalDepth/data/by_type/RP66V1-de-tiffed/WAPIMS/2006-2008/W002846/Amulet_1_Log_Data/PROCESSED/Amulet1_S1R4_ProcessedData.dlis'
EXAMPLE_1_GB = '/Users/engun/Documents/workspace/TotalDepth/data/by_type/RP66V1-de-tiffed/WAPIMS/2006-2008/W001596/WIRELINE/BAMBRA-3 LOGGING BYPASS_RDT_S1R1.dlis'
EXAMPLE_4_GB = '/Users/engun/Documents/workspace/TotalDepth/data/by_type/RP66V1/WAPIMS/2010-2015/W003353/Ungani_2_Log_Data_C/Buru_Ungani-2_RM_VISION+geoVISION_25-2800mMD.dlis'


class Client:
    LOGGER_PREFIX = 'CLIENT'

    def __init__(self):
        self.connection = connection.Connection()

    def _init_file(self, file_path: str, file_object: typing.BinaryIO) -> bool:
        logger.info(
            f'{self.LOGGER_PREFIX}: Scanning file "{os.path.basename(file_path)}"'
            f' {os.path.getsize(file_path):,d} bytes.'
        )
        timer = common.Timer()
        total_timer = common.Timer()
        json_to_server = scan_file_json_index(file_object)
        logger.info(f'{self.LOGGER_PREFIX}: scan_file_json_index() time {timer.ms():.3f} (ms).')
        logger.info(f'{self.LOGGER_PREFIX}: Scanning complete {len(json_to_server)} bytes. Adding file to server.')
        timer = common.Timer()
        json_from_server = self.connection.add_file(file_path, client_lib.file_mod_time(file_path), json_to_server)
        logger.info(f'{self.LOGGER_PREFIX}: the_server.add_file() time {timer.ms():.3f} (ms).')
        timer = common.Timer()
        if json_from_server:
            logger.info(f'{self.LOGGER_PREFIX}: the_server.add_file() got[{len(json_from_server)}] bytes')
            seek_read_data = []
            seek_read = common.SeekRead.from_json(json_from_server)
            bytes_read = 0
            for seek, read in seek_read.seek_read:
                file_object.seek(seek)
                seek_read_data.append((seek, common.encode_bytes(file_object.read(read))))
                bytes_read += read
            logger.info(f'{self.LOGGER_PREFIX}: reading [{len(seek_read_data)}] blocks {bytes_read} bytes.')
            json_to_server = json.dumps(seek_read_data)
            logger.info(f'{self.LOGGER_PREFIX}: sending[{len(json_to_server)}] JSON bytes to the server.')
            json_from_server = self.connection.add_data(file_path, client_lib.file_mod_time(file_path), json_to_server)
            result = json.loads(json_from_server)
            logger.info(f'{self.LOGGER_PREFIX}: result {result!r}.')
        logger.info(f'{self.LOGGER_PREFIX}: the_server.add_data() time {timer.ms():.3f} (ms).')
        logger.info(f'{self.LOGGER_PREFIX}: Total time {total_timer.ms():.3f} (ms).')
        return True

    def add_file(self, file_path: str):
        logger.info(f'{self.LOGGER_PREFIX}: Opening {file_path}')
        timer_overall = common.Timer()
        with open(file_path, 'rb') as file_object:
            result = self._init_file(file_path, file_object)
            logger.info(f'{self.LOGGER_PREFIX}: File added to server, result {result}.')
            # TODO: Read IFLRs

        logger.info(f'{self.LOGGER_PREFIX}: execution time {timer_overall.ms():.3f} (ms).')


def run_with_new_connection(file_path: str) -> None:
    client = Client()
    client.add_file(file_path)
    logger.info('')


DEFAULT_OPT_LOG_FORMAT_VERBOSE = '%(asctime)s - %(filename)-16s - %(process)5d - (%(threadName)-10s) - %(levelname)-8s - %(message)s'


def main() -> int:
    logging.basicConfig(level=logging.INFO, format=DEFAULT_OPT_LOG_FORMAT_VERBOSE, stream=sys.stdout)
    # Write test plots
    logger.info(f'Test simulation...')
    run_with_new_connection(BASIC_FILE)
    run_with_new_connection(EXAMPLE_540_KB)
    run_with_new_connection(EXAMPLE_24_MB)
    run_with_new_connection(EXAMPLE_120_MB)
    run_with_new_connection(EXAMPLE_256_MB)
    run_with_new_connection(EXAMPLE_1_GB)
    run_with_new_connection(EXAMPLE_4_GB)
    return 0


if __name__ == '__main__':
    sys.exit(main())


