import argparse
import dataclasses
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

    def __init__(self, latency_s: float, bandwidth_bps: float, realtime: bool = False):
        self.latency_s = latency_s
        self.bandwidth_bps = bandwidth_bps
        self.realtime = realtime
        self.total_time = 0.0

    def transmit(self, data_bytes: bytes, direction: str) -> None:
        t = self.latency_s + 8 * len(data_bytes) / self.bandwidth_bps
        logger.debug('COMMS_: %s length %d delay %.3f (ms)', direction, len(data_bytes), t * 1000)
        self.total_time += t
        if self.realtime:
            time.sleep(t)


class Server:
    def __init__(self, seek_rate_byte_per_s: float, read_rate_byte_per_s: float, realtime: bool = False):
        self.seek_rate_byte_per_s = seek_rate_byte_per_s
        self.read_rate_byte_per_s = read_rate_byte_per_s
        self.realtime = realtime
        self.file_position = 0
        self.total_time = 0.0

    def get(self, file_position: int, length: int) -> bytes:
        t = abs(file_position - self.file_position) / self.seek_rate_byte_per_s
        t += length / self.read_rate_byte_per_s
        self.total_time += t
        logger.debug('SERVER: fpos %d length %d delay %.3f (ms)', file_position, length, t * 1000)
        if self.realtime:
            time.sleep(t)
        self.file_position = file_position
        return b' ' * length


@dataclasses.dataclass
class RunResult:
    has_hits: int
    has_misses: int
    minimal_bytes: int
    num_bytes: int
    sizeof: int
    time_exec: float


class Client:
    def __init__(self, comms: Communications, server: Server):
        self.comms = comms
        self.server = server

    def run(self, seek_reads: typing.Tuple[typing.Tuple[int, int], ...], greedy_length: int) -> RunResult:
        time_start = time.perf_counter()
        svf = svfs.cSVF('ID')
        time_svf = 0.0
        has_hits = has_misses = 0
        minimal_bytes = 0
        for fpos_demand, length_demand in seek_reads:
            minimal_bytes += length_demand
            blocks = [f'({fpos:,d} : {length:,d} : {fpos + length:,d})' for fpos, length in svf.blocks()]
            logger.debug('CLIENT:  blocks was: %s', blocks)
            logger.debug(
                f'CLIENT: demands fpos {fpos_demand:16,d} length {length_demand:6,d} ({fpos_demand + length_demand:16,d})')
            time_svf_start = time.perf_counter()
            has_data = svf.has_data(fpos_demand, length_demand)
            time_svf += time.perf_counter() - time_svf_start
            if not has_data:
                time_svf_start = time.perf_counter()
                need = svf.need(fpos_demand, length_demand, greedy_length)
                time_svf += time.perf_counter() - time_svf_start
                logger.debug(f'CLIENT:    need {need}')
                for fpos, length in need:
                    logger.debug(f'CLIENT:    need fpos {fpos:16,d} length {length:6,d} ({fpos + length:16,d})')
                    # Crude simulation of a GET request.
                    client_server_message = f'GET File position {fpos} length {length}'.encode('ascii')
                    self.comms.transmit(client_server_message, 'Client->Server')
                    result = self.server.get(fpos, length)
                    self.comms.transmit(result, 'Server->Client')
                    time_svf_start = time.perf_counter()
                    svf.write(fpos, result)
                    time_svf += time.perf_counter() - time_svf_start
                    logger.debug(
                        f'CLIENT:   wrote fpos {fpos:16,d} length {len(result):6,d} ({fpos + len(result):16,d})')
                if not svf.has_data(fpos_demand, length_demand):
                    logger.error(
                        f'CLIENT: demands fpos {fpos_demand:16,d} length {length_demand:6,d} ({fpos_demand + length_demand:16,d})'
                    )
                    blocks = [f'({fpos:,d} : {length:,d} : {fpos + length:,d})' for fpos, length in svf.blocks()]
                    logger.error('CLIENT:  blocks now: %s', blocks)
                    assert 0
                has_misses += 1
            else:
                logger.debug(
                    f'CLIENT:     has fpos {fpos_demand:16,d} length {length_demand:6,d} ({fpos_demand + length_demand:16,d})'
                )
                has_hits += 1

            time_svf_start = time.perf_counter()
            svf.read(fpos_demand, length_demand)
            time_svf += time.perf_counter() - time_svf_start
        time_exec = time.perf_counter() - time_start
        time_exec = self.comms.total_time + self.server.total_time + time_svf
        logger.info('has(): hits: %d misses: %d', has_hits, has_misses)
        logger.info(
            'Blocks: %d bytes: %d sizeof: %d overhead: %d', svf.num_blocks(), svf.num_bytes(), svf.size_of(),
            svf.size_of() - svf.num_bytes()
        )
        percent_str = '+' * int(0.5 + 50 * self.comms.total_time / time_exec)
        logger.info(
            f'Comms time : {self.comms.total_time * 1000:10.3f} (ms) ({self.comms.total_time / time_exec:6.1%})'
            f' {percent_str}'
        )
        percent_str = '+' * int(0.5 + 50 * self.server.total_time / time_exec)
        logger.info(
            f'Server time: {self.server.total_time * 1000:10.3f} (ms) ({self.server.total_time / time_exec:6.1%})'
            f' {percent_str}'
        )
        percent_str = '+' * int(0.5 + 50 * time_svf / time_exec)
        logger.info(
            f'SVF time   : {time_svf * 1000:10.3f} (ms) ({time_svf / time_exec:6.1%})'
            f' {percent_str}'
        )
        # time_residual = time_exec - self.comms.total_time - self.server.total_time - time_svf
        # percent_str = '+' * int(0.5 + 50 * time_residual / time_exec)
        # logger.info(
        #     f'Residual   : {time_residual * 1000:10.3f} (ms) ({time_residual / time_exec:6.1%})'
        #     f' {percent_str}'
        # )
        logger.info(f'Total      : {time_exec * 1000:10.3f} (ms) ({time_exec / time_exec:6.1%})')
        logger.info('SVF contents: %s Execution time: %.3f (s) %.3f (Mb/s)',
                    svf.num_bytes(), time_exec, svf.num_bytes() / time_exec / 1024 ** 2
                    )
        return RunResult(has_hits, has_misses, minimal_bytes, svf.num_bytes(), svf.size_of(),
                         self.comms.total_time + self.server.total_time + time_svf
                         )


def run(
        events: typing.Tuple[typing.Tuple[int, int], ...],
        greedy_length: int,
        latency_s: float, bandwidth_bit_ps: float,
        seek_rate_byte_per_s: float, read_rate_byte_per_s: float, realtime: bool
) -> RunResult:
    comms = Communications(latency_s, bandwidth_bit_ps, realtime=realtime)
    server = Server(seek_rate_byte_per_s, read_rate_byte_per_s, realtime=realtime)
    client = Client(comms, server)
    return client.run(events, greedy_length)


def main():
    time_start = time.perf_counter()
    result = 0
    parser = argparse.ArgumentParser(description='Simulate reading into a SVF.', prog=__file__)
    parser.add_argument('-l', '--log-level', dest='log_level', type=int, default=20, help='Log level.')
    parser.add_argument('--latency', type=float, default=10,
                        help='Communications channel latency (one way) in ms. [default: %(default)d]')
    parser.add_argument('--bandwidth', type=float, default=50,
                        help='Communications channel bandwidth in million bits per second. [default: %(default)d]')
    parser.add_argument('--seek-rate', type=float, default=10000,
                        help='Server seek rate in million bytes per second. [default: %(default)d]')
    parser.add_argument('--read-rate', type=float, default=50,
                        help='Server read rate in million bytes per second. [default: %(default)d]')
    parser.add_argument('--greedy-length', type=int, default=-1,
                        help=(
                            'The greedy length to read fragments from the server.'
                            ' Zero means read every fragment.'
                            ' Default is to run through a range of greedy lengths and report the performance.'
                            ' [default: %(default)d]'
                        )
                        )
    parser.add_argument('--realtime', action="store_true", default=False,
                        help='Run in realtime (may be slow). [default: %(default)d]')
    args = parser.parse_args()
    # print('Args:', args)
    logging.basicConfig(level=args.log_level, format=LOG_FORMAT_NO_PROCESS, stream=sys.stdout)

    results_time: typing.Dict[str, typing.List[typing.Tuple[int, RunResult]]] = {}
    print('Simulator setup:')
    print(f'Network latency {args.latency:.3f} (ms) bandwidth {args.bandwidth:.3f} (M bits/s)')
    print(f'Server seek rate {args.seek_rate:.3f} (M bytes/s) read rate {args.read_rate:.3f} (M bytes/s)')
    # for name in ('EXAMPLE_FILE_POSITIONS_LENGTHS_TIFF_CMU_1',):
    # for name in ('EXAMPLE_FILE_POSITIONS_LENGTHS_SYNTHETIC',):
    t_start = time.perf_counter()
    for name in sim_examples.EXAMPLE_FILE_POSITIONS_LENGTHS:
        if args.greedy_length == -1:  # Default greedy-length, use a range
            greedy_length = 0
            # for greedy_length in (1024,):
            # for greedy_length in range(0, 1024 + 32, 32):
            while greedy_length <= 2048 * 4 * 4 * 4:
                logger.info('Running %s with %d file actions and greedy_length %d', name,
                            len(sim_examples.EXAMPLE_FILE_POSITIONS_LENGTHS[name]), greedy_length)
                result = run(
                    sim_examples.EXAMPLE_FILE_POSITIONS_LENGTHS[name], greedy_length,
                    args.latency / 1000, args.bandwidth * 1e6, args.seek_rate * 1e6, args.read_rate * 1e6, args.realtime
                )
                if name not in results_time:
                    results_time[name] = []
                results_time[name].append((greedy_length, result))
                if greedy_length == 0:
                    greedy_length = 1
                elif greedy_length == 1:
                    greedy_length = 16
                else:
                    greedy_length *= 2
        else:
            logger.info('Running %s with %d file actions and greedy_length %d', name,
                        len(sim_examples.EXAMPLE_FILE_POSITIONS_LENGTHS[name]), args.greedy_length)
            result = run(
                sim_examples.EXAMPLE_FILE_POSITIONS_LENGTHS[name], args.greedy_length,
                args.latency / 1000, args.bandwidth * 1e6, args.seek_rate * 1e6, args.read_rate * 1e6, args.realtime
            )
            if name not in results_time:
                results_time[name] = []
            results_time[name].append((args.greedy_length, result))
    for key in results_time:
        print(f'{key}:')
        print(
            f'{"greedy_length":>14} {"Time(ms)":>10}'
            f' {"Hits":>8} {"Miss":>8} {"Hits%":>8}'
            f' {"Min. Bytes":>12} {"Act. Bytes":>12} {"Act. / Min.":>12}'
            f' {"sizeof":>10} {"Overhead":>8} {"sizeof / Act.":>14}'
        )
        for greedy_length, result in results_time[key]:
            print(
                f'{greedy_length:14} {result.time_exec * 1000 :10.1f} {result.has_hits:8d} {result.has_misses:8d}'
                f' {result.has_hits / (result.has_hits + result.has_misses):8.3%}'
                f' {result.minimal_bytes:12d} {result.num_bytes:12d} {result.num_bytes / result.minimal_bytes:12.3%}'
                f' {result.sizeof:10d} {result.sizeof - result.num_bytes:+8d} {result.sizeof / result.num_bytes:14.3%}'
            )
    print(f'Execution time: {time.perf_counter() - time_start:10.3f} (s)')
    return 0


if __name__ == '__main__':
    exit(main())
