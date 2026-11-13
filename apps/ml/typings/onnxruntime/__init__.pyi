from collections.abc import Mapping, Sequence
from os import PathLike
from typing import Any

from numpy.typing import NDArray

class SessionOptions:
    intra_op_num_threads: int
    inter_op_num_threads: int
    enable_cpu_mem_arena: bool
    enable_mem_pattern: bool
    log_severity_level: int

class NodeArg:
    @property
    def name(self) -> str: ...
    @property
    def shape(self) -> list[Any]: ...

class InferenceSession:
    def __init__(
        self,
        path_or_bytes: str | PathLike[str],
        sess_options: SessionOptions | None = None,
        providers: Sequence[str] | None = None,
    ) -> None: ...
    def run(
        self, output_names: Sequence[str] | None, input_feed: Mapping[str, NDArray[Any]]
    ) -> list[NDArray[Any]]: ...
    def get_inputs(self) -> list[NodeArg]: ...
    def get_outputs(self) -> list[NodeArg]: ...
