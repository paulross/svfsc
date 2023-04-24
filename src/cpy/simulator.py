import argparse
import logging
import pprint
import sys
import time
import typing

import svfs

import src.cpy.sim_examples as sim_examples

logger = logging.getLogger(__file__)

LOG_FORMAT_VERBOSE = (
    '%(asctime)s - %(filename)24s#%(lineno)-4d - %(process)5d - (%(threadName)-10s) - %(levelname)-8s - %(message)s'
)

LOG_FORMAT_NO_THREAD = (
    '%(asctime)s - %(filename)24s#%(lineno)-4d - %(process)5d - %(levelname)-8s - %(message)s'
)

LOG_FORMAT_NO_PROCESS = (
    '%(asctime)s - %(filename)24s#%(lineno)-4d - %(levelname)-8s - %(message)s'
)


class Communications:
    """Represents the delay over a communication line."""

    def __init__(self, latency_s: float, bandwidth_bps: float):
        self.latency_s = latency_s
        self.bandwidth_bps = bandwidth_bps
        self.total_time = 0.0

    def transmit(self, data_bytes: bytes, direction: str) -> None:
        t = self.latency_s + 8 * len(data_bytes) / self.bandwidth_bps
        logger.debug('COMMS_: %s length %d delay %.3f (ms)', direction, len(data_bytes), t * 1000)
        self.total_time += t
        time.sleep(t)


class Server:
    def __init__(self, seek_rate_byte_per_s: float, read_rate_byte_per_s: float):
        self.seek_rate_byte_per_s = seek_rate_byte_per_s
        self.read_rate_byte_per_s = read_rate_byte_per_s
        self.file_position = 0
        self.total_time = 0.0

    def get(self, file_position: int, length: int) -> bytes:
        t = abs(file_position - self.file_position) / self.seek_rate_byte_per_s
        t += length / self.read_rate_byte_per_s
        self.total_time += t
        logger.debug('SERVER: fpos %d length %d delay %.3f (ms)', file_position, length, t * 1000)
        time.sleep(t)
        self.file_position = file_position
        return b' ' * length


class Client:
    def __init__(self, comms: Communications, server: Server):
        self.comms = comms
        self.server = server

    def run(self, seek_reads: typing.Tuple[typing.Tuple[int, int], ...], greedy_length: int):
        time_start = time.perf_counter()
        svf = svfs.cSVF('ID')
        time_svf = 0.0
        for fpos_demand, length_demand in seek_reads:
            logger.debug('CLIENT: demands fpos %d length %d', fpos_demand, length_demand)
            if not svf.has_data(fpos_demand, length_demand):
                for fpos, length in svf.need(fpos_demand, length_demand, greedy_length):
                    logger.debug('CLIENT: fpos %d length %d', fpos, length)
                    # Crude simulation of a GET request.
                    client_server_message = f'GET File position {fpos} length {length}'.encode('ascii')
                    self.comms.transmit(client_server_message, 'Client->Server')
                    result = self.server.get(fpos, length)
                    self.comms.transmit(result, 'Server->Client')
                    time_svf_start = time.perf_counter()
                    svf.write(fpos_demand, result)
                    time_svf += time.perf_counter() - time_svf_start
                    logger.debug('CLIENT: wrote result length %d', len(result))
        time_exec = time.perf_counter() - time_start
        time_residual = time_exec - self.comms.total_time - self.server.total_time - time_svf
        logger.info(
            f'Comms time : {self.comms.total_time * 1000:10.3f} (ms) ({self.comms.total_time / time_exec:6.1%})')
        logger.info(
            f'Server time: {self.server.total_time * 1000:10.3f} (ms) ({self.server.total_time / time_exec:6.1%})')
        logger.info(f'SVF time   : {time_svf * 1000:10.3f} (ms) ({time_svf / time_exec:6.1%})')
        logger.info(f'Residual   : {time_residual * 1000:10.3f} (ms) ({time_residual / time_exec:6.1%})', )
        logger.info(f'Total      : {time_exec * 1000:10.3f} (ms) ({time_exec / time_exec:6.1%})', )
        logger.info('SVF contents: %s Execution time: %.3f (s) %.3f (Mb/s)',
                    svf.num_bytes(), time_exec, svf.num_bytes() / time_exec / 1024 ** 2
                    )


def run(
        events: typing.Tuple[typing.Tuple[int, int], ...],
        greedy_length: int,
        latency_s: float, bandwidth_bps: float,
        seek_rate_byte_per_s: float, read_rate_byte_per_s: float,
):
    comms = Communications(latency_s, bandwidth_bps)
    server = Server(seek_rate_byte_per_s, read_rate_byte_per_s)
    client = Client(comms, server)
    client.run(events, greedy_length)


def main():
    time_start = time.perf_counter()
    result = 0
    parser = argparse.ArgumentParser(description='Simulate reading into a SVF.', prog=__file__)
    parser.add_argument('-l', '--log-level', dest='log_level', type=int, default=20, help='Log level.')
    parser.add_argument('--latency', type=float, default=0.005,
                        help='Communications channel latency in seconds. [default: %(default)d]')
    parser.add_argument('--bandwidth', type=float, default=50e6,
                        help='Communications channel bandwidth in bits per second. [default: %(default)d]')
    parser.add_argument('--seek-rate', type=float, default=1e9,
                        help='Server seek rate in bytes per second. [default: %(default)d]')
    parser.add_argument('--read-rate', type=float, default=500e6,
                        help='Server read rate in bytes per second. [default: %(default)d]')
    parser.add_argument('--greedy-length', type=int, default=0,
                        help='The greedy length to read fragments from the server. Zero means read every fragment. [default: %(default)d]')
    args = parser.parse_args()
    # print('Args:', args)
    logging.basicConfig(level=args.log_level, format=LOG_FORMAT_NO_PROCESS, stream=sys.stdout)

    results = {}
    for name in sim_examples.EXAMPLE_FILE_POSITIONS_LENGTHS:
        for greedy_length in range(0, 1024 * 2, 64):
            logger.info('Running %s with greedy_length %d', name, greedy_length)
            t_start = time.perf_counter()
            run(
                sim_examples.EXAMPLE_FILE_POSITIONS_LENGTHS[name], greedy_length, args.latency, args.bandwidth, args.seek_rate,
                args.read_rate
            )
            if name not in results:
                results[name] = []
            results[name].append((greedy_length, time.perf_counter() - t_start))
    for key in results:
        print(f'{key}:')
        for greedy_length, tim in results[key]:
            print(f'    {greedy_length:6} : {tim * 1000 :10.1f} (ms)')
    print(f'Execution time: {time.perf_counter() - time_start:10.3f} (s)')
    return result


if __name__ == '__main__':
    exit(main())
