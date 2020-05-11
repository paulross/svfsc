import json
import logging
import os
import sys
import typing

from src.cpy import client_lib, server, common
from src.cpy.client_lib import scan_file_json_index

logger = logging.getLogger(__file__)


BASIC_FILE = '/Users/engun/Documents/workspace/TotalDepth/example_data/RP66V1/data/BASIC_FILE.dlis'
EXAMPLE_540_KB = '/Users/engun/Documents/workspace/TotalDepth/example_data/RP66V1/data/206_05a-_3_DWL_DWL_WIRE_258276498.DLIS'
EXAMPLE_24_MB = '/Users/engun/Documents/workspace/TotalDepth/data/by_type/RP66V1/WAPIMS/2010-2015/W003353/S1R2/UNGANI-2_S1R2_XRMI&WSTT_DLIS_MAIN.dlis'
EXAMPLE_120_MB = '/Users/engun/Documents/workspace/TotalDepth/data/by_type/RP66V1/WAPIMS/2010-2015/W004434/Wireline/CVX_F24B_S1R1_IBC-CBL_Repeat_031PUC.DLIS'
EXAMPLE_256_MB = '/Users/engun/Documents/workspace/TotalDepth/data/by_type/RP66V1-de-tiffed/WAPIMS/2006-2008/W002846/Amulet_1_Log_Data/PROCESSED/Amulet1_S1R4_ProcessedData.dlis'
EXAMPLE_1_GB = '/Users/engun/Documents/workspace/TotalDepth/data/by_type/RP66V1-de-tiffed/WAPIMS/2006-2008/W001596/WIRELINE/BAMBRA-3 LOGGING BYPASS_RDT_S1R1.dlis'
EXAMPLE_4_GB = '/Users/engun/Documents/workspace/TotalDepth/data/by_type/RP66V1/WAPIMS/2010-2015/W003353/Ungani_2_Log_Data_C/Buru_Ungani-2_RM_VISION+geoVISION_25-2800mMD.dlis'


def _init_file(file_path: str, file_object: typing.BinaryIO, the_server: server.Server) -> bool:
    logger.info(f'CLIENT: Scanning file "{os.path.basename(file_path)}" {os.path.getsize(file_path):,d} bytes.')
    timer = common.Timer()
    total_timer = common.Timer()
    json_to_server = scan_file_json_index(file_object)
    logger.info(f'CLIENT: scan_file_json_index() time {timer.ms():.3f} (ms).')
    logger.info(f'CLIENT: Scanning complete {len(json_to_server)} bytes. Adding file to server.')
    timer = common.Timer()
    json_from_server = the_server.add_file(file_path, client_lib.file_mod_time(file_path), json_to_server)
    logger.info(f'CLIENT: the_server.add_file() time {timer.ms():.3f} (ms).')
    timer = common.Timer()
    if json_from_server:
        logger.info(f'CLIENT: the_server.add_file() got[{len(json_from_server)}] bytes')#= {json_from_server!r}')
        seek_read_data = []
        seek_read = common.SeekRead.from_json(json_from_server)
        bytes_read = 0
        for seek, read in seek_read.seek_read:
            file_object.seek(seek)
            seek_read_data.append((seek, common.encode_bytes(file_object.read(read))))
            bytes_read += read
        logger.info(f'CLIENT: reading [{len(seek_read_data)}] blocks {bytes_read} bytes.')
        json_to_server = json.dumps(seek_read_data)
        logger.info(f'CLIENT: sending[{len(json_to_server)}] JSON bytes to the server.')#' {json_to_server!r}')
        json_from_server = the_server.add_data(file_path, client_lib.file_mod_time(file_path), json_to_server)
        result = json.loads(json_from_server)
        logger.info(f'CLIENT: result {result!r}.')
        # if result['response']:
        #     break
    logger.info(f'CLIENT: the_server.add_data() time {timer.ms():.3f} (ms).')
    logger.info(f'CLIENT: Total time {total_timer.ms():.3f} (ms).')
    return True


def client(file_path: str, the_server: server.Server):
    """
    This acts like a client (or user) that submits a file then wants to read parts of it.

    Basic sequence:

        - Client wants to do something (read EFLR, plot curves etc.).
        - Asks server if it needs data to satisfy the request.
        - If so, seek+reads from the file and sends data from file to server.
        - Asks server for the HTML/SVG corresponding to the original; request.

    :param file_path:
    :param the_server:
    :return:
    """
    logger.info(f'CLIENT: Opening {file_path}')
    timer_overall = common.Timer()
    with open(file_path, 'rb') as file_object:
        result = _init_file(file_path, file_object, the_server)
        logger.info(f'CLIENT: File added to server, result {result}.')
    logger.info(f'CLIENT: execution time {timer_overall.ms():.3f} (ms).')



        # while json_bytes:
        #     logger.info(f'CLIENT: Recieved {len(json_bytes)} JSON bytes.')
        #     data = []
        #     for seek, read in json.loads(json_bytes):
        #         file_object.seek(seek)
        #         data.append((seek, file_object.read()))
        #     json_bytes = json.dumps(data)
        #     logger.info(f'CLIENT: Seek/read done. Sending {len(json_bytes)} JSON bytes.')
        #     json_bytes = the_server.add_data(file_path, os.stat(file_path).st_mtime_ns / 1e9, json_bytes)


def run_with_new_server(file_path: str) -> None:
    a_server = server.Server()
    client(file_path, a_server)
    logger.info('')


DEFAULT_OPT_LOG_FORMAT_VERBOSE = '%(asctime)s - %(filename)-16s - %(process)5d - (%(threadName)-10s) - %(levelname)-8s - %(message)s'


def main() -> int:
    logging.basicConfig(level=logging.INFO, format=DEFAULT_OPT_LOG_FORMAT_VERBOSE, stream=sys.stdout)
    # Write test plots
    logger.info(f'Writing test plots...')
    # a_server = server.Server()
    run_with_new_server(BASIC_FILE)
    run_with_new_server(EXAMPLE_540_KB)
    run_with_new_server(EXAMPLE_24_MB)
    run_with_new_server(EXAMPLE_120_MB)
    run_with_new_server(EXAMPLE_256_MB)
    run_with_new_server(EXAMPLE_1_GB)
    run_with_new_server(EXAMPLE_4_GB)
    return 0


if __name__ == '__main__':
    sys.exit(main())


