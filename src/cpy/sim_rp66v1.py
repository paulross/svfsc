import logging
import os
import sys

from TotalDepth.util.bin_file_type import binary_file_type_from_path

from src.cpy import client, common

logger = logging.getLogger(__file__)

if os.path.exists(os.path.expanduser('~/PycharmProjects/TotalDepth')):
    EXAMPLE_DATA_PATH = os.path.expanduser('~/PycharmProjects/TotalDepth/example_data')
    ARCHIVE_DATA_PATH = os.path.expanduser('~/PycharmProjects/TotalDepth/data/by_type')
else:
    EXAMPLE_DATA_PATH = os.path.expanduser('~/Documents/workspace/TotalDepth/example_data')
    ARCHIVE_DATA_PATH = os.path.expanduser('~/Documents/workspace/TotalDepth/data/by_type')



BASIC_FILE = os.path.join(EXAMPLE_DATA_PATH, 'RP66V1/data/BASIC_FILE.dlis')
EXAMPLE_540_KB = os.path.join(EXAMPLE_DATA_PATH, 'RP66V1/data/206_05a-_3_DWL_DWL_WIRE_258276498.DLIS')
EXAMPLE_24_MB = os.path.join(ARCHIVE_DATA_PATH, 'RP66V1/WAPIMS/2010-2015/W003353/S1R2/UNGANI-2_S1R2_XRMI&WSTT_DLIS_MAIN.dlis')
EXAMPLE_120_MB = os.path.join(ARCHIVE_DATA_PATH, 'RP66V1/WAPIMS/2010-2015/W004434/Wireline/CVX_F24B_S1R1_IBC-CBL_Repeat_031PUC.DLIS')
EXAMPLE_256_MB = os.path.join(ARCHIVE_DATA_PATH, 'RP66V1/WAPIMS/2006-2008/W002846/Amulet_1_Log_Data/PROCESSED/Amulet1_S1R4_ProcessedData.dlis')
EXAMPLE_481_MB = os.path.join(ARCHIVE_DATA_PATH, 'RP66V1/WAPIMS/2006-2008/W002854/WIRELINE/GNU-1_RDT/GNU-1_S4R3_RDT.dlis')
EXAMPLE_1_GB = os.path.join(ARCHIVE_DATA_PATH, 'RP66V1/WAPIMS/2006-2008/W001596/WIRELINE/BAMBRA-3 LOGGING BYPASS_RDT_S1R1.dlis')
EXAMPLE_4_GB = os.path.join(ARCHIVE_DATA_PATH, 'RP66V1/WAPIMS/2010-2015/W003353/Ungani_2_Log_Data_C/Buru_Ungani-2_RM_VISION+geoVISION_25-2800mMD.dlis')


LOGGER_PREFIX = 'SIMULA'


def run_with_new_connection(file_path: str) -> None:
    logger.info(f'{LOGGER_PREFIX}: Creating new client for {file_path}')

    timer = common.Timer()
    a_client = client.Client()
    a_client.add_file(file_path)

    b_c_to_s = sum(a_client.connection.bytes_client_to_server)
    b_s_to_c = sum(a_client.connection.bytes_server_to_client)
    logger.info(
        f'{LOGGER_PREFIX}: simulating file of size {os.path.getsize(file_path)}'
        f' bytes: to server={b_c_to_s} to client={b_s_to_c}'
        f' took {timer.ms():.3f} (ms).'
    )

    # logger.info(
    #     f'{LOGGER_PREFIX}: Bytes to server: {a_client.connection.bytes_client_to_server}'
    #     f' ({sum(a_client.connection.bytes_client_to_server)})'
    # )
    # logger.info(
    #     f'{LOGGER_PREFIX}: Bytes to client: {a_client.connection.bytes_server_to_client}'
    #     f' ({sum(a_client.connection.bytes_server_to_client)})'
    # )
    logger.info('')


DEFAULT_OPT_LOG_FORMAT_VERBOSE = '%(asctime)s - %(filename)16s#%(lineno)5d - %(process)5d - (%(threadName)-10s)' \
                                 ' - %(levelname)-8s - %(message)s'


def process_some_example_files():
    # run_with_new_connection(BASIC_FILE)
    # run_with_new_connection(EXAMPLE_540_KB)
    # run_with_new_connection(EXAMPLE_24_MB)
    # run_with_new_connection(EXAMPLE_120_MB)
    # run_with_new_connection(EXAMPLE_256_MB)
    run_with_new_connection(EXAMPLE_481_MB)
    # run_with_new_connection(EXAMPLE_1_GB)
    # run_with_new_connection(EXAMPLE_4_GB)


def process_directory(path):
    for dirpath, dirnames, filenames in os.walk(path):
        for aName in filenames:
            path_in = os.path.join(dirpath, aName)
            bin_file_type = binary_file_type_from_path(path_in)
            if bin_file_type == 'RP66V1':
                try:
                    run_with_new_connection(path_in)
                except Exception as _err:
                    logger.exception(f'process_directory({path_in})')


def main() -> int:
    logging.basicConfig(level=logging.INFO, format=DEFAULT_OPT_LOG_FORMAT_VERBOSE, stream=sys.stdout)
    logger.info(f'Test simulation...')

    process_some_example_files()
    # process_directory(os.path.join(ARCHIVE_DATA_PATH, 'RP66V1/WAPIMS/2006-2008/W002846'))
    # process_directory(os.path.join(ARCHIVE_DATA_PATH, 'RP66V1/WAPIMS/2006-2008'))

    return 0


if __name__ == '__main__':
    sys.exit(main())
