import typing

from TotalDepth.RP66V1.core import LogPass, RepCode, XAxis
from TotalDepth.RP66V1.core.LogicalRecord import EFLR


class LogicalFileIndex:
    # NOTE: Similar to TotalDepth.RP66V1.core.LogicalFile.LogicalFile

    ALWAYS_CACHE = {b'FILE-HEADER', b'ORIGIN', b'WELL-REFERENCE', b'CHANNEL' b'FRAME'}
    ORIGIN_SETS = {b'ORIGIN', b'WELL-REFERENCE'}

    def __init__(self, fpos: int, fh_eflr: EFLR.ExplicitlyFormattedLogicalRecord, cache_eflrs: bool = True):
        """
        Create a Logical File entry with the file position of the LSRH 'FILE-HEADER' EFLR.
        If cache_eflrs is True then all given EFLRs will be cached.
        If False only essential EFLRs are cached. These are FILE-HEADER and ORIGIN permanently, CHANNEL and FRAME EFLRs
        will then be cached just so long as a LogPass can be constructed then they will be discarded from the cache.
        """
        assert cache_eflrs, 'Premature optimisation.'
        if fh_eflr.lr_type != 0:
            raise ValueError(
                f'LogicalFileIndex needs a FILE-HEADER EFLR (type 0) not {fh_eflr.set.type} of type {fh_eflr.lr_type}'
            )
        self.cache_eflrs = cache_eflrs
        # List of EFLRs as an integer entry in the low_level_index
        # First must be 'FILE-HEADER', second must be 'ORIGIN' or 'WELL-REFERENCE'
        self.eflr_positions: typing.List[int] = [fpos]
        # Cache of fully formed EFLRs, we always cache FILE-HEADER, ORIGIN, CHANNEL, FRAME EFLRs
        # Cache other EFLRs depending on cache_eflrs flag.
        self.eflr_cache: typing.Dict[int, EFLR.ExplicitlyFormattedLogicalRecord] = {
            fpos: fh_eflr,
        }
        # The file position of CHANNEL and FRAME EFLRs we haven't seen yet, these make a LogPass
        self.channel_fpos = 0
        self.frame_fpos = 0
        self.log_pass: typing.Union[None, LogPass.LogPass] = None
        # TODO: IFLRs
        self.iflr_position_map: typing.Dict[RepCode.ObjectName, XAxis.XAxis] = {}

    def __str__(self) -> str:
        ret = [
            f' EFLR refs: {len(self.eflr_positions)}',
            f', cached: {len(self.eflr_cache)}',
            f' LogPass? {self.log_pass is not None}'
        ]
        return ' '.join(ret)

    def long_str(self) -> str:
        ret = [
            f' EFLR refs: {len(self.eflr_positions)}',
            f', cached: {len(self.eflr_cache)}',
        ]
        if self.log_pass is not None:
            ret.append(str(self.log_pass))
        return ' '.join(ret)

    @staticmethod
    def can_add_eflr(eflr: EFLR.ExplicitlyFormattedLogicalRecord) -> bool:
        return eflr.lr_type != 0

    def add_eflr(self, fpos: int, eflr: EFLR.ExplicitlyFormattedLogicalRecord) -> None:
        """
        Adds an EFLR.

        NOTE: Similar to TotalDepth.RP66V1.core.LogicalFile.LogicalFile#add_eflr
        """
        assert self.can_add_eflr(eflr)

        if len(self.eflr_positions) == 1:
            # Expect an ORIGIN or WELL-REFERENCE
            if eflr.set.type not in self.ORIGIN_SETS:
                raise ValueError('TODO')
        self.eflr_positions.append(fpos)
        self._cache_eflr(fpos, eflr)
        if eflr.set.type == b'CHANNEL':
            self.channel_fpos = fpos
        if eflr.set.type == b'FRAME':
            self.frame_fpos = fpos
        if self.channel_fpos and self.frame_fpos:
            # TODO: self.log_pass not None?
            self.log_pass = LogPass.log_pass_from_RP66V1(
                self.eflr_cache[self.frame_fpos],
                self.eflr_cache[self.channel_fpos],
            )

    def _cache_eflr(self, fpos: int, eflr: EFLR.ExplicitlyFormattedLogicalRecord) -> None:
        if self.cache_eflrs or eflr.set in self.ALWAYS_CACHE:
            assert fpos not in self.eflr_cache
            self.eflr_cache[fpos] = eflr


class MidLevelIndex:
    def __init__(self, cache_eflrs: bool = True):
        assert cache_eflrs, 'Premature optimisation.'
        self.cache_eflrs = cache_eflrs
        self.logical_files: typing.List[LogicalFileIndex] = []

    def add_eflr(self, fpos: int, eflr: EFLR.ExplicitlyFormattedLogicalRecord):
        if len(self.logical_files) == 0 or not self.logical_files[-1].can_add_eflr(eflr):
            self.logical_files.append(LogicalFileIndex(fpos, eflr, self.cache_eflrs))
        else:
            self.logical_files[-1].add_eflr(fpos, eflr)

    def __str__(self) -> str:
        return f'MidLevelIndex with {len(self.logical_files)} Logical Files'

    def long_str(self) -> str:
        return '\n'.join(
            [str(self)] + [lf.long_str() for lf in self.logical_files]
        )
